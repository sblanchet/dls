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

#ifndef MessageH
#define MessageH

/*****************************************************************************/

#include <libxml/parser.h>

#include <pdcom/Process.h>
#include <pdcom/Subscriber.h>

#include "lib/LibDLS/Exception.h"

class MessageList;

/*****************************************************************************/

/** Message
 */
class Message:
    public PdCom::Subscriber
{
public:
    Message(MessageList *, xmlNode *);
    ~Message();

    void subscribe(PdCom::Process *);

    /** Message type.
     */
    enum Type {
        Information, /**< Non-critical information. */
        Warning, /**< Warning, that does not influence
                   the process flow. */
        Error, /**< Error, that influences the process flow. */
        Critical /**< Critical error, that makes the process
                   unable to run. */
    };

    /** Exception.
     */
    class Exception:
        public LibDLS::Exception
    {
        public:
            Exception(string pmsg):
                LibDLS::Exception(pmsg) {};
    };

private:
    MessageList * const _parent_list;
    Type _type;
    std::string _path;
    PdCom::Variable *_var;
    double _value;
    bool _data_present;

    static Type _typeFromString(const std::string &);

    virtual void notify(PdCom::Variable *);
    virtual void notifyDelete(PdCom::Variable *);
};

/*****************************************************************************/

#endif


