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

#include <pdcom/Variable.h>

#include "Message.h"
#include "MessageList.h"
#include "globals.h"

/****************************************************************************/

/** Constructor.
 */
Message::Message(
        MessageList *list,
        xmlNode *node
        ):
    _parent_list(list),
    _type(Information),
    _var(NULL),
    _value(0.0),
    _data_present(false)
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
Message::~Message()
{
    if (_var) {
        _var->unsubscribe(this);
    }
}

/****************************************************************************/

/** Subscribe variable.
 */
void Message::subscribe(PdCom::Process *process)
{
    if (_var) {
        _var->unsubscribe(this);
    }

    _var = process->findVariable(_path);

    if (!_var) {
        msg() << "Message variable " << _path << " not found!";
        log(::Error);
        return;
    }

    _data_present = false;

    try {
        _var->subscribe(this, 0.0);
        _var->poll(this);
    }
    catch (PdCom::Exception &e) {
        msg() << "Message variable subscription failed!";
        log(::Error);
        _var->unsubscribe(this);
        _var = NULL;
    }
}

/****************************************************************************/

/** Converts a message type string to the appropriate #Type.
 */
Message::Type Message::_typeFromString(const std::string &str)
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

void Message::notify(PdCom::Variable *var)
{
    if (var != _var) {
        return;
    }

    double new_value;
    var->getValue(&new_value);

    if (_data_present && _value == 0.0 && new_value != 0.0) {
        double t = var->getMTime();
        LibDLS::Time time;
        time.from_dbl_time(t);
        string storeType;

        switch (_type) {
            case Information:
                storeType = "info";
                break;
            case Warning:
                storeType = "warn";
                break;
            case Error:
                storeType = "error";
                break;
            case Critical:
                storeType = "error";
                break;
            default:
                storeType = "info";
                break;
        }

        _parent_list->store_message(time, storeType, _path);
    }

    _value = new_value;
    _data_present = true;
}

/****************************************************************************/

void Message::notifyDelete(PdCom::Variable *var)
{
    if (var == _var) {
        _var = NULL;
        _data_present = false;
    }
}

/****************************************************************************/
