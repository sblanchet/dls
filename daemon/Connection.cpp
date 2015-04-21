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

#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>

#include "Connection.h"
#include "proto/dls.pb.h"

#include <iostream>
using namespace std;

/*****************************************************************************/

Connection::Connection(int fd):
    _fd(fd),
    _ret(0),
    _running(true)
{
}

/*****************************************************************************/

Connection::~Connection()
{
}

/*****************************************************************************/

int Connection::start_thread()
{
    int ret;

    ret = pthread_create(&_thread, NULL, _run_static, this);
    if (ret) {
        return ret;
    }

    return 0;
}

/*****************************************************************************/

int Connection::thread_finished()
{
    void *res;

    int ret = pthread_tryjoin_np(_thread, &res);

    if (ret == EBUSY) {
        return 0;
    }
    if (ret) {
        return ret;
    }
    else {
        _ret = (uint64_t) res;
        return 1;
    }
}

/*****************************************************************************/

void *Connection::_run_static(void *arg)
{
    Connection *conn = (Connection *) arg;
    return conn->_run();
}

/*****************************************************************************/

void *Connection::_run()
{
    fd_set rfds, wfds;
    int max_fd = -1, ret;
    char buf[1024];

    _send_hello();

    while (_running) {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_SET(_fd, &rfds);
        max_fd = _fd;

        if (!_ostream.eof()) {
            FD_SET(_fd, &wfds);
        }

        ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret == -1) {
            if (errno != EINTR) {
                char str[1024];
                strerror_r(errno, str, sizeof(str));
                cerr << "select() failed: " << str << endl;
                _running = false;
            }
        }

        if (ret > 0 && FD_ISSET(_fd, &rfds)) {
            ret = read(_fd, buf, sizeof(buf));
            if (ret < 0) {
                if (errno != EINTR) {
                    char str[1024];
                    strerror_r(errno, str, sizeof(str));
                    cerr << "select() failed: " << str << endl;
                    _running = false;
                }
            }
            else if (ret == 0) {
                // connection closed by foreign host
                _running = false;
            }
            else { // ret > 0
                cerr << string(buf, ret) << endl;
            }
        }

        if (ret > 0 && FD_ISSET(_fd, &wfds)) {
            _ostream.read(buf, sizeof(buf));
            int to_write = _ostream.gcount();
            int written = 0;
            ret = write(_fd, buf, to_write);
            if (ret == -1) {
                if (errno != EINTR) {
                    char str[1024];
                    strerror_r(errno, str, sizeof(str));
                    cerr << "write() failed: " << str << endl;
                    _running = false;
                }
            }
            if (ret >= 0) {
                written = ret;
            }
            for (int i = to_write - 1; i >= written; i--) {
                _ostream.putback(buf[i]);
            }
        }
    }

    close(_fd);
    return (void *) 88;
}

/*****************************************************************************/

int Connection::_send_hello()
{
    DlsProto::Hello msg;
    msg.set_revision(REVISION);
    msg.set_protocol_version(1);
    msg.SerializeToOstream(&_ostream);
}

/*****************************************************************************/
