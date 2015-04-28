/*****************************************************************************
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
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sstream>
using namespace std;

#include "BaseMessageList.h"
#include "BaseMessage.h"

using namespace LibDLS;

/****************************************************************************/

/** Constructor.
 */
BaseMessageList::BaseMessageList()
{
}

/****************************************************************************/

/** Destructor.
 */
BaseMessageList::~BaseMessageList()
{
    _clear();
}

/****************************************************************************/

/** Return message file path.
 */
std::string BaseMessageList::path(const string &job_path) const
{
	return job_path + "/plainmessages.xml";
}

/****************************************************************************/

/** Check, if message file exists.
 */
bool BaseMessageList::exists(const string &job_path) const
{
	struct stat buf;
	int ret = stat(path(job_path).c_str(), &buf);
	if (ret == 0) {
		return true;
	}
	else if (errno == ENOENT) {
		return false;
	}
	else {
		stringstream err;
		err << "stat() failed: " << strerror(errno);
		throw Exception(err.str());
	}
}

/****************************************************************************/

/** Import XML file.
 */
void BaseMessageList::import(const string &job_path)
{
    _clear();

    xmlDocPtr doc = xmlParseFile(path(job_path).c_str());

    if (!doc) {
		stringstream err;

		err << "Failed to import messages";
		xmlErrorPtr e = xmlGetLastError();
		if (e) {
			err << ": " << e->message;
		}
		else {
			err << ".";
		}

		throw Exception(err.str());
    }

    xmlNode *rootNode = xmlDocGetRootElement(doc);

    for (xmlNode *curNode = rootNode->children;
            curNode; curNode = curNode->next) {
        if (curNode->type == XML_ELEMENT_NODE
                && string((const char *) curNode->name) == "Message") {
            BaseMessage *message;

            try {
                message = newMessage(curNode);
            } catch (BaseMessage::Exception &e) {
                _clear();
				xmlFreeDoc(doc);
                throw Exception("Failed to import message: " + e.msg);
            }

            _messages.push_back(message);
        }
    }

    xmlFreeDoc(doc);
}

/****************************************************************************/

/** Count the messages.
 */
unsigned int BaseMessageList::count() const
{
	unsigned int c = 0;

    for (list<BaseMessage *>::const_iterator i = _messages.begin();
            i != _messages.end(); i++) {
        c++;
    }

	return c;
}

/****************************************************************************/

/** Clear the messages.
 */
void BaseMessageList::_clear()
{
    for (list<BaseMessage *>::iterator i = _messages.begin();
            i != _messages.end(); i++) {
        delete *i;
    }

    _messages.clear();
}

/****************************************************************************/
