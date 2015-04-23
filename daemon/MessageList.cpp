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

#include <sstream>
using namespace std;

#include <libxml/parser.h>

#include "MessageList.h"
#include "Message.h"
#include "Job.h"

/****************************************************************************/

/** Constructor.
 */
MessageList::MessageList(
        Job *job
        ):
    _parent_job(job)
{
}

/****************************************************************************/

/** Destructor.
 */
MessageList::~MessageList()
{
    _clear();
}

/****************************************************************************/

/** Import XML file.
 */
void MessageList::import()
{
    stringstream file_name;

    _clear();

    // Dateinamen konstruieren
    file_name << _parent_job->dir() << "/plainmessages.xml";

    xmlDocPtr doc = xmlParseFile(file_name.str().c_str());

    if (!doc) {
        // no message file found.
        return;
    }

    xmlNode *rootNode = xmlDocGetRootElement(doc);

    for (xmlNode *curNode = rootNode->children;
            curNode; curNode = curNode->next) {
        if (curNode->type == XML_ELEMENT_NODE
                && string((const char *) curNode->name) == "Message") {
            Message *message;

            try {
                message = new Message(this, curNode);
            } catch (Message::Exception &e) {
                msg() << "Failed to import messages: " << e.msg;
                log(Error);
                _clear();
                break;
            }

            _messages.push_back(message);
        }
    }

    xmlFreeDoc(doc);
}

/****************************************************************************/

/** Subscribe variables.
 */
void MessageList::subscribe(PdCom::Process *process)
{
    for (list<Message *>::iterator i = _messages.begin();
            i != _messages.end(); i++) {
        (*i)->subscribe(process);
    }
}

/****************************************************************************/

/** Store a message.
 */
void MessageList::store_message(LibDLS::Time time, const std::string &type,
        const std::string &msg)
{
    _parent_job->message(time, type, msg);
}

/****************************************************************************/

/** Clear the messages.
 */
void MessageList::_clear()
{
    for (list<Message *>::iterator i = _messages.begin();
            i != _messages.end(); i++) {
        delete *i;
    }

    _messages.clear();
}

/****************************************************************************/
