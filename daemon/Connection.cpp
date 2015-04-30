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

#include "config.h"
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
    int ret;

    _send_hello();

    while (_running) {
        FD_ZERO(&rfds);
        FD_SET(_fd, &rfds);

        FD_ZERO(&wfds);
        if (!_ostream.eof()) {
            // something to write
            FD_SET(_fd, &wfds);
        }

        ret = select(_fd + 1, &rfds, &wfds, NULL, NULL);
        if (ret == -1) {
            if (errno != EINTR) {
                char buf[1024], *str = strerror_r(errno, buf, sizeof(buf));
                cerr << "select() failed: " << str << endl;
                _running = false;
            }
        }

        if (ret > 0) { // file descriptors ready
            if (FD_ISSET(_fd, &rfds)) {
                _receive();
            }
            if (FD_ISSET(_fd, &wfds)) {
                _send();
            }
        }
    }

    close(_fd);
    return (void *) 88;
}

/*****************************************************************************/

void Connection::_send_hello()
{
    DlsProto::Hello msg;
    msg.set_version(PACKAGE_VERSION);
    msg.set_revision(REVISION);
    msg.set_protocol_version(1);
    msg.SerializeToOstream(&_ostream);
}

/*****************************************************************************/

void Connection::_send()
{
    char buf[1024];

    _ostream.read(buf, sizeof(buf));

    int to_write = _ostream.gcount();
    int ret = write(_fd, buf, to_write);
    if (ret == -1) {
        if (errno != EINTR) {
            char buf[1024], *str = strerror_r(errno, buf, sizeof(buf));
            cerr << "write() failed: " << str << endl;
            _running = false;
        }
    }

    // put unsent bytes back in the stream
    int written = max(ret, 0);
    for (int i = to_write - 1; i >= written; i--) {
        _ostream.putback(buf[i]);
    }
}

/*****************************************************************************/

void Connection::_receive()
{
    char buf[1024];

    int ret = read(_fd, buf, sizeof(buf));
    if (ret < 0) {
        if (errno != EINTR) {
            char buf[1024], *str = strerror_r(errno, buf, sizeof(buf));
            cerr << "read() failed: " << str << endl;
            _running = false;
        }
    }
    else if (ret == 0) {
        // connection closed by foreign host
        cerr << "Closed by remote host." << endl;
        _running = false;
    }
    else { // ret > 0
        _process(string(buf, ret));
    }
}

/*****************************************************************************/

void Connection::_process(const string &rec)
{
    cout << "+" << rec << "-" << endl;
}

/*****************************************************************************/
