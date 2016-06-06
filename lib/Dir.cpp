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

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <unistd.h>
#include <errno.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <uriparser/Uri.h>

#include "config.h"
#include "LibDLS/Dir.h"
#include "proto/dls.pb.h"

using namespace std;
using namespace LibDLS;

#ifdef _WIN32
#define DLS_INVALID_SOCKET INVALID_SOCKET
#define DLS_CLOSE_SOCKET closesocket
#else
#define DLS_INVALID_SOCKET -1
#define DLS_CLOSE_SOCKET ::close
#endif

//#define DEBUG

/*****************************************************************************/

class LibDLS::SocketInputStream:
    public google::protobuf::io::CopyingInputStream
{
    public:
        SocketInputStream(
#ifdef _WIN32
                SOCKET sock
#else
                int sock
#endif
        ): _sock(sock), _errno(0) {}

        int Read(void *buffer, int size) {
#ifdef DEBUG
            stringstream msg;
            msg << __func__ << "(" << size << ")";
#endif

            ssize_t bytes_read;
#ifdef _WIN32
            bytes_read = recv(_sock, (char *) buffer, size, 0);
#else
            bytes_read = read(_sock, buffer, size);
#endif

#ifdef DEBUG
            msg << ": " << bytes_read;
            log(msg.str());
#endif

            if (bytes_read >= 0) {
                return bytes_read;
            } else {
#ifdef _WIN32
                _errno = WSAGetLastError();
#else
                _errno = errno;
#endif
                return -1;
            }
        }

        int lastError() const { return _errno; }

    private:
#ifdef _WIN32
        SOCKET _sock;
#else
        int _sock;
#endif
        int _errno;
};

/*****************************************************************************/

Directory::Directory(const std::string &uri_text):
    _access(Unknown),
    _sock(DLS_INVALID_SOCKET),
    _sis(NULL),
    _cisa(NULL),
    _fos(NULL)
{
    set_uri(uri_text);

#ifdef _WIN32
    WSADATA wsaData;
    int err = WSAStartup(0x0202, &wsaData);
    if (err != 0) {
        stringstream err;
        err << "WSAStartup() failed with error " << err << ".";
        _error_msg = err.str();
        log(err.str());
        throw DirectoryException(err.str());
    } else {
        stringstream msg;
        msg << "WSAStartup() initialized with version "
            << hex << wsaData.wVersion << ".";
        log(msg.str());
    }
#endif
}

/*****************************************************************************/

Directory::~Directory()
{
    _disconnect();

    list<Job *>::iterator job_i;
    for (job_i = _jobs.begin(); job_i != _jobs.end(); job_i++) {
        delete *job_i;
    }

#ifdef _WIN32
    WSACleanup();
#endif
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

void Directory::set_uri(const string &uri_text)
{
    _uri_text = uri_text;

#if 0
    cerr << __func__ << "(" << uri_text << ")" << endl;
#endif

    UriUriA uri;
    UriParserStateA state;
    state.uri = &uri;
    if (uriParseUriA(&state, _uri_text.c_str()) != URI_SUCCESS) {
        stringstream err;
        err << "Failed to parse URI \"" << _uri_text << "\"!";
        _error_msg = err.str();
        log(err.str());
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

    _host = uriTextRange(uri.hostText);
    _port = uriTextRange(uri.portText);
    if (_port == "") {
        _port = "53584";
    }
    _path = uriPathSegment(uri.pathHead);
    if (uri.absolutePath) {
        _path = "/" + _path;
    }

#if 0
    cerr << "host=" << _host << " path=" << _path << endl;
#endif

    uriFreeUriMembersA(&uri);

    if (scheme == "" || scheme == "file") {
        _access = Local;
    }
    else if (scheme == "dls") {
        _access = Network;
    }
    else {
        _access = Unknown;
        stringstream err;
        err << "Unsupported URI scheme \"" << scheme << "\"!";
        _error_msg = err.str();
        log(err.str());
        throw DirectoryException(err.str());
    }
}

/*****************************************************************************/

void Directory::import()
{
    _jobs.clear();

    if (_access == Local) {
        _importLocal();
    }
    else if (_access == Network) {
        _importNetwork();
    }

    // Nach Job-ID sortieren
    _jobs.sort(compare_job_id);

    _notify_observers();
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

void Directory::set_dir_info(DlsProto::DirInfo *dir_info) const
{
    //dir_info->set_path(_path);

    for (list<LibDLS::Job *>::const_iterator job_i = _jobs.begin();
            job_i != _jobs.end(); job_i++) {
        (*job_i)->set_job_info(dir_info->add_job());
    }
}

/*****************************************************************************/

void Directory::attach_observer(Observer *o)
{
    _observers.insert(o);
}

/*****************************************************************************/

void Directory::remove_observer(Observer *o)
{
    _observers.erase(o);
}

/*****************************************************************************/

void Directory::_importLocal()
{
    stringstream str;
    DIR *dir;
    struct dirent *dir_ent;
    Job *job;
    string dir_name;
    unsigned int job_id;

    str.exceptions(ios::failbit | ios::badbit);

    if (!(dir = opendir(_path.c_str()))) {
        stringstream err;
        err << "Failed to open DLS directory \"" << _path << "\"!";
        _error_msg = err.str();
        log(err.str());
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

        job = new Job(this);

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

void Directory::_importNetwork()
{
    DlsProto::Request req;
    DlsProto::Response res;

    DlsProto::DirInfoRequest *dir_req = req.mutable_dir_info();
    dir_req->set_path(_path);

    _send_message(req);
    _receive_message(res);

    if (res.has_error()) {
        _error_msg = res.error().message();
        log(_error_msg);
        throw DirectoryException(res.error().message());
    }

    const DlsProto::DirInfo &dir_info = res.dir_info();

    _jobs.clear();

    //_path = dir_info.path();

    google::protobuf::RepeatedPtrField<DlsProto::JobInfo>::const_iterator
        job_i;
    for (job_i = dir_info.job().begin();
            job_i != dir_info.job().end(); job_i++) {
        _jobs.push_back(new Job(this, *job_i));
    }
}

/*****************************************************************************/

void Directory::_connect()
{
    if (connected()) {
        return;
    }

    {
        stringstream msg;
        msg << "Connecting to " << _host << " on port " << _port << ".";
        log(msg.str());
    }

    /* Obtain address(es) matching host/port */

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */

    struct addrinfo *result;
    int ret = getaddrinfo(_host.c_str(), _port.c_str(), &hints, &result);
    if (ret) {
        stringstream err;
        err << "Failed to get address info: " << gai_strerror(ret);
        _error_msg = err.str();
        log(_error_msg);
        throw DirectoryException(err.str());
    }

    struct addrinfo *rp;
    int sock;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == DLS_INVALID_SOCKET) {
            continue;
        }

#ifdef _WIN32
        unsigned long arg = 0;
        int ret = ioctlsocket(sock, FIONBIO, &arg);
        if (ret) {
            _error_msg = "Failed to set socket to blocking mode!";
            log(_error_msg);
            DLS_CLOSE_SOCKET(sock);
            throw DirectoryException(_error_msg);
        }
#endif

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        DLS_CLOSE_SOCKET(sock);
    }

    freeaddrinfo(result);

    if (!rp) {
        _error_msg = "Connection failed!";
        log(_error_msg);
        goto out_throw;
    }

    try {
        _sis = new SocketInputStream(sock);
    }
    catch (...) {
        _error_msg = "Failed to create socket input stream!";
        log(_error_msg);
        goto out_sock;
    }

    try {
        _cisa = new google::protobuf::io::CopyingInputStreamAdaptor(_sis);
    }
    catch (...) {
        _error_msg = "Failed to create socket stream adapter!";
        log(_error_msg);
        goto out_sis;
    }

    try {
        _fos = new google::protobuf::io::FileOutputStream(sock);
    }
    catch (...) {
        _error_msg = "Failed to create file output stream!";
        log(_error_msg);
        goto out_cisa;
    }

    _sock = sock;

    {
        stringstream msg;
        msg << "Connected.";
        log(msg.str());
    }

    /* read hello message */
    _receive_hello();
    return;

out_cisa:
    delete _cisa;
out_sis:
    delete _sis;
out_sock:
    DLS_CLOSE_SOCKET(sock);
out_throw:
    throw DirectoryException(_error_msg);
}

/*****************************************************************************/

void Directory::_disconnect()
{
    if (!connected()) {
        return;
    }

    {
        stringstream msg;
        msg << "Disconnecting.";
        log(msg.str());
    }

    delete _cisa;
    delete _sis;
    delete _fos;
    DLS_CLOSE_SOCKET(_sock);
    _sock = DLS_INVALID_SOCKET;
}

/*****************************************************************************/

void Directory::_send_message(const DlsProto::Request &req)
{
    _connect();

#ifdef DLS_PROTO_DEBUG
    cerr << "Sending message with " << req.ByteSize() << " bytes:" << endl;
    cerr << req.DebugString() << endl;
#endif

    {
        string str;
        req.SerializeToString(&str);
        google::protobuf::io::CodedOutputStream os(_fos);
        os.WriteVarint32(req.ByteSize());
        if (os.HadError()) {
            stringstream err;
            err << "os.WriteVarint32() failed!";
            throw DirectoryException(err.str());
        }

        os.WriteString(str);
        if (os.HadError()) {
            stringstream err;
            err << "os.WriteString() failed!";
            throw DirectoryException(err.str());
        }
    }

    _fos->Flush();
}

/*****************************************************************************/

void Directory::_receive_message(
        google::protobuf::Message &msg,
        bool debug
        )
{
    google::protobuf::io::CodedInputStream ci(_cisa);

    uint32_t size;
    bool success = ci.ReadVarint32(&size);
    if (!success) {
        stringstream err;
        err << "ReadVarint32() failed: " << strerror(_sis->lastError())
            << " (" << _sis->lastError() << ").";
        _error_msg = err.str();
        log(_error_msg);
        _disconnect();
        throw DirectoryException(err.str());
    }

    string rec;
    success = ci.ReadString(&rec, size);
    if (!success) {
        stringstream err;
        err << "ReadString() failed!";
        _error_msg = err.str();
        log(_error_msg);
        _disconnect();
        throw DirectoryException(err.str());
    }

    success = msg.ParseFromString(rec);
    if (!success) {
        stringstream err;
        err << "ParseFromString() failed!";
        _error_msg = err.str();
        log(_error_msg);
        _disconnect();
        throw DirectoryException(err.str());
    }

#ifdef DLS_PROTO_DEBUG
    if (debug) {
        cerr << "Received message with " << size << " bytes:" << endl;
        cerr << msg.DebugString() << endl;
    }
#endif
}

/*****************************************************************************/

void Directory::_receive_hello()
{
    DlsProto::Hello hello;

    _receive_message(hello);

    stringstream str;
    str << "Received hello from DLS " << hello.version()
        << " " << hello.revision() << " protocol version "
        << hello.protocol_version() << ".";
    log(str.str());
}

/*****************************************************************************/

void Directory::_notify_observers()
{
    std::set<Observer *>::iterator o;
    for (o = _observers.begin(); o != _observers.end(); o++) {
        (*o)->update();
    }
}

/*****************************************************************************/
