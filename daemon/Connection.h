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

#ifndef DLSConnectionHpp
#define DLSConnectionHpp

/*****************************************************************************/

#include <pthread.h>
#include <sstream>

/*****************************************************************************/

/** Incoming network connection.
 */
class Connection
{
public:
    Connection(int);
    ~Connection();

    int start_thread();
    int thread_finished();
    int return_code() const { return _ret; }

private:
    int _fd;
    pthread_t _thread;
    int _ret; /**< Return value. */
    bool _running;
    std::stringstream _ostream;

    static void *_run_static(void *);
    void *_run();
    int _send_hello();
};

/*****************************************************************************/

#endif


