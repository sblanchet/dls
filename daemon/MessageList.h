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

#ifndef MessageListH
#define MessageListH

/*****************************************************************************/

#include <list>
using namespace std;

#include <pdcom/Process.h>

#include "lib/LibDLS/Exception.h"
#include "lib/LibDLS/Time.h"

/*****************************************************************************/

class Job;
class Message;

/*****************************************************************************/

/** Message list.
 */
class MessageList
{
public:
    MessageList(Job *);
    ~MessageList();

    void import();
    void subscribe(PdCom::Process *);
    void store_message(LibDLS::Time, const std::string &, const std::string &);

private:
    Job * const _parent_job; /**< Parent job. */
    list<Message *> _messages; /**< Messages. */

    void _clear();
};

/*****************************************************************************/

#endif
