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

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "lib/LibDLS/Dir.h"
#include "proto/dls.pb.h"

class ProcMother;

/*****************************************************************************/

/** Incoming network connection.
 */
class Connection
{
public:
    Connection(ProcMother *, int);
    ~Connection();

    int start_thread();
    int thread_finished();
    int return_code() const { return _ret; }

private:
    ProcMother * const _parent_proc;
    const int _fd;
    google::protobuf::io::FileInputStream _fis;
    google::protobuf::io::FileOutputStream _fos;
    pthread_t _thread;
    int _ret; /**< Return value. */
    bool _running;
    LibDLS::Directory _dir;

    static void *_run_static(void *);
    void *_run();
    void _send(const google::protobuf::Message &);
    void _send_hello();
    void _receive();
    void _process(const std::string &);
    void _process_dir_info(const DlsProto::DirInfoRequest &);
};

/*****************************************************************************/

#endif


