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
#include <iostream>
using namespace std;

#include "BaseMessage.h"
#include "BaseMessageList.h"
using namespace LibDLS;

/****************************************************************************/

/** Constructor.
 */
BaseMessage::BaseMessage(
        xmlNode *node
        ):
    _type(Information)
{
    char *data;
    string str;

    data = (char *) xmlGetProp(node, (const xmlChar *) "type");
    if (!data) {
        throw Exception("Missing type attribute!");
    }
    str = data;
    xmlFree(data);

    _type = _typeFromString(str);

    data = (char *) xmlGetProp(node, (const xmlChar *) "variable");
    if (!data) {
        throw Exception("Missing variable attribute!");
    }
    _path = data;
    xmlFree(data);
}

/****************************************************************************/

/** Destructor.
 */
BaseMessage::~BaseMessage()
{
}

/****************************************************************************/

/** Converts a message type string to the appropriate #Type.
 */
BaseMessage::Type BaseMessage::_typeFromString(const std::string &str)
{
    if (str == "Information") {
        return Information;
    }
    if (str == "Warning") {
        return Warning;
    }
    if (str == "Error") {
        return Error;
    }
    if (str == "Critical") {
        return Critical;
    }

    stringstream err;
    err << "Invalid message type " << str;
    throw Exception(err.str());
}

/****************************************************************************/
