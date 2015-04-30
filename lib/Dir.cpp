/******************************************************************************
 *
 *  $Id$
 *
 *  This file is part of the Data Logging Service (DLS).
 *
 *  DLS is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  DLS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with DLS. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include <iostream>
#include <sstream>
#include <algorithm>

#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <uriparser/Uri.h>

#include "LibDLS/Dir.h"

using namespace std;
using namespace LibDLS;

/*****************************************************************************/

Directory::Directory()
{
}

/*****************************************************************************/

Directory::~Directory()
{
    list<Job *>::iterator job_i;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
        delete *job_i;
    }
}

/*****************************************************************************/

bool compare_job_id(Job *first, Job *second)
{
	return first->id() < second->id();
}

/*****************************************************************************/

std::string uriTextRange(const UriTextRangeA &range)
{
    if (range.first) {
        return string(range.first, range.afterLast - range.first);
    }

    return string();
}

/*****************************************************************************/

std::string uriPathSegment(const UriPathSegmentA *p)
{
    stringstream str;
    bool first = true;

    while (p) {
        if (first) {
            first = false;
        }
        else {
            str << "/";
        }
        str << uriTextRange(p->text);
        p = p->next;
    }
    return str.str();
}

/*****************************************************************************/

#if 0
std::string debugHost(const UriHostDataA &h) {
    stringstream str;
    if (h.ip4) {
        str << "ip4";
    }
    if (h.ip6) {
        str << "ip6";
    }
    str << uriTextRange(h.ipFuture);
    return str.str();
}
#endif

/*****************************************************************************/

void Directory::import(const string &uriText)
{
    UriParserStateA state;
    UriUriA uri;

    state.uri = &uri;
    if (uriParseUriA(&state, uriText.c_str()) != URI_SUCCESS) {
        stringstream err;
        err << "Failed to parse URI \"" << uriText << "\"!";
        throw DirectoryException(err.str());
    }

#if 0
    cerr << "scheme " << uriTextRange(uri.scheme) << endl;
    cerr << "userinfo " << uriTextRange(uri.userInfo) << endl;
    cerr << "hostText " << uriTextRange(uri.hostText) << endl;
    cerr << "hostData " << debugHost(uri.hostData) << endl;
    cerr << "portText " << uriTextRange(uri.portText) << endl;
    cerr << "pathHead " << uriPathSegment(uri.pathHead) << endl;
    cerr << "pathTail " << uriPathSegment(uri.pathTail) << endl;
    cerr << "abs " << (uri.absolutePath ? "yes" : "no") << endl;
#endif

    string scheme = uriTextRange(uri.scheme);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

    string path = uriPathSegment(uri.pathHead);
    if (uri.absolutePath) {
        path = "/" + path;
    }

    string host = uriTextRange(uri.hostText);
    string port = uriTextRange(uri.portText);

    uriFreeUriMembersA(&uri);

    if (scheme == "" || scheme == "file") {
        _importLocal(path);
    }
    else if (scheme == "dls") {
        _importNetwork(host, port);
    }
    else {
        stringstream err;
        err << "Unsupported URI scheme \"" << scheme << "\"!";
        throw DirectoryException(err.str());
    }

    // Nach Job-ID sortieren
    _jobs.sort(compare_job_id);
}

/*****************************************************************************/

Job *Directory::job(unsigned int index)
{
    list<Job *>::iterator job_i;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++, index--) {
        if (!index) {
			return *job_i;
		}
    }

    return NULL;
}

/*****************************************************************************/

Job *Directory::find_job(unsigned int job_id)
{
    list<Job *>::iterator job_i;

    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
        if ((*job_i)->id() == job_id) {
			return *job_i;
		}
    }

    return NULL;
}

/*****************************************************************************/

void Directory::_importLocal(const string &path)
{
    stringstream str;
    DIR *dir;
    struct dirent *dir_ent;
    Job *job;
    string dir_name;
    unsigned int job_id;

    str.exceptions(ios::failbit | ios::badbit);

    _path = path;
    _jobs.clear();

    if (!(dir = opendir(_path.c_str()))) {
        stringstream err;
        err << "Failed to open DLS directory \"" << _path << "\"!";
        throw DirectoryException(err.str());
    }

    while ((dir_ent = readdir(dir))) {
        dir_name = dir_ent->d_name;
        if (dir_name.find("job")) continue;

        str.str("");
        str.clear();
        str << dir_name.substr(3);

        try {
            str >> job_id;
        }
        catch (...) {
            continue;
        }

		job = new Job();

        try {
            job->import(_path, job_id);
        }
        catch (JobException &e) {
			stringstream err;
            err << "WARNING: Failed to import job "
                 << job_id << ": " << e.msg;
			log(err.str());
			delete job;
            continue;
        }

        _jobs.push_back(job);
    }

    // Verzeichnis schliessen
    closedir(dir);
}

/*****************************************************************************/

void Directory::_importNetwork(const string &host, string port)
{
    if (port == "") {
        port = "53584";
    }

    cerr << "Connecting to " << host << " on port " << port << endl;

    /* Obtain address(es) matching host/port */

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */

    struct addrinfo *result;
    int ret = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (ret) {
        stringstream err;
        err << "Faield to get address info: " << gai_strerror(ret);
        throw DirectoryException(err.str());
    }

    struct addrinfo *rp;
    int fd;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        ::close(fd);
    }

    freeaddrinfo(result);

    if (!rp) {
        throw DirectoryException("Connection failed!");
    }

    cerr << "connected" << endl;
    close(fd);
}

/*****************************************************************************/
