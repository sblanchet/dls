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

#include <google/protobuf/io/coded_stream.h>

#include "config.h"
#include "Connection.h"
#include "proto/dls.pb.h"
#include "ProcMother.h"

#include <iostream>
using namespace std;

/*****************************************************************************/

Connection::Connection(ProcMother *parent_proc, int fd):
    _parent_proc(parent_proc),
    _fd(fd),
    _fis(fd),
    _fos(fd),
    _ret(0),
    _running(true)
{
}

/*****************************************************************************/

Connection::~Connection()
{
    close(_fd);
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
    fd_set rfds;
    int ret;

    _send_hello();

    while (_running) {
        FD_ZERO(&rfds);
        FD_SET(_fd, &rfds);

        ret = select(_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret == -1) {
            if (errno != EINTR) {
                char ebuf[1024], *str = strerror_r(errno, ebuf, sizeof(ebuf));
                cerr << "select() failed: " << str << endl;
                _running = false;
            }
        }

        if (ret > 0) { // file descriptors ready
            if (FD_ISSET(_fd, &rfds)) {
                _receive();
            }
        }
    }

    return (void *) 0;
}

/*****************************************************************************/

void Connection::_send(
        const google::protobuf::Message &msg
#ifdef DLS_PROTO_DEBUG
        , bool debug
#endif
        )
{
    string str;
    msg.SerializeToString(&str);

#ifdef DLS_PROTO_DEBUG
    if (debug) {
        cerr << "Sending message with "
            << msg.ByteSize() << " bytes: " << endl
            << msg.DebugString() << endl;
    }
#endif

    {
        google::protobuf::io::CodedOutputStream os(&_fos);
        os.WriteVarint32(msg.ByteSize());
        if (os.HadError()) {
            cerr << "os.WriteVarint32() failed!" << endl;
            _running = false;
            return;
        }

        os.WriteString(str);
        if (os.HadError()) {
            cerr << "os.WriteString() failed!" << endl;
            _running = false;
            return;
        }
    }

    _fos.Flush();
}

/*****************************************************************************/

void Connection::_send_hello()
{
    DlsProto::Hello msg;
    msg.set_version(PACKAGE_VERSION);
    msg.set_revision(REVISION);
    msg.set_protocol_version(1);
    _send(msg);
}

/*****************************************************************************/

void Connection::_receive()
{
    google::protobuf::io::CodedInputStream ci(&_fis);

    uint32_t size;
    bool success = ci.ReadVarint32(&size);
    if (!success) {
        cerr << "ReadVarint32() failed!" << endl;
        _running = false;
        return;
    }

    string str;
    success = ci.ReadString(&str, size);
    if (!success) {
        cerr << "ReadString() failed!" << endl;
        _running = false;
        return;
    }

    _process(str);
}

/*****************************************************************************/

void Connection::_process(const string &rec)
{
    DlsProto::Request req;
    req.ParseFromString(rec);

#ifdef DLS_PROTO_DEBUG
    cerr << "Received request with " << rec.size() << " bytes: " << endl;
    cerr << req.DebugString() << endl;
#endif

    if (req.has_dir_info()) {
        _process_dir_info(req.dir_info());
    }

    if (req.has_job_request()) {
        _process_job_request(req.job_request());
    }
}

/*****************************************************************************/

void Connection::_process_dir_info(const DlsProto::DirInfoRequest &req)
{
    string path;
    if (req.path() != "") {
        path = req.path();
    }
    else {
        path = _parent_proc->dls_dir();
    }

    try {
        _dir.import(path);
    }
    catch (LibDLS::DirectoryException &e) {
        DlsProto::Response res;
        DlsProto::Error *err = res.mutable_error();
        err->set_message(e.msg);
        _send(res);
        return;
    }

    DlsProto::Response res;
    _dir.set_dir_info(res.mutable_dir_info());
    _send(res);
}

/*****************************************************************************/

void Connection::_process_job_request(const DlsProto::JobRequest &req)
{
    LibDLS::Job *job = _dir.find_job(req.id());

    if (!job) {
        DlsProto::Response res;
        stringstream str;
        str << "Job " << req.id() << " not found!";
        DlsProto::Error *err = res.mutable_error();
        err->set_message(str.str());
        _send(res);
        return;
    }

    if (req.fetch_channels()) {
        job->fetch_channels();
        DlsProto::Response res;
        DlsProto::DirInfo *dir_info = res.mutable_dir_info();
        DlsProto::JobInfo *job_info = dir_info->add_job();
        job->set_job_info(job_info, false);
        _send(res);
    }

    if (req.has_channel_request()) {
        _process_channel_request(job, req.channel_request());
    }
}

/*****************************************************************************/

void Connection::_process_channel_request(
        LibDLS::Job *job,
        const DlsProto::ChannelRequest &req
        )
{
    LibDLS::Channel *channel = job->find_channel(req.id());

    if (!channel) {
        stringstream str;
        str << "Channel " << req.id()
            << " not found in job " << job->id() << "!";
        DlsProto::Response res;
        DlsProto::Error *err = res.mutable_error();
        err->set_message(str.str());
        _send(res);
        return;
    }

    if (req.fetch_chunks()) {
        channel->fetch_chunks();
        DlsProto::Response res;
        DlsProto::DirInfo *dir_info = res.mutable_dir_info();
        DlsProto::JobInfo *job_info = dir_info->add_job();
        DlsProto::ChannelInfo *channel_info = job_info->add_channel();
        channel_info->set_id(channel->dir_index());
        channel->set_chunk_info(channel_info);
        _send(res);
    }

    if (req.has_data_request()) {
        const DlsProto::DataRequest &data_req = req.data_request();
        unsigned int min_values = 0;
        if (data_req.has_min_values()) {
            min_values = data_req.min_values();
        }
        unsigned int decimation = 0;
        if (data_req.has_decimation()) {
            decimation = data_req.decimation();
        }
        channel->fetch_data(LibDLS::Time(data_req.start()),
                LibDLS::Time(data_req.end()), min_values,
                _static_data_callback, this, decimation);

        DlsProto::Response res;
        res.set_end_of_response(true);
        _send(res);
    }
}

/*****************************************************************************/

int Connection::_static_data_callback(LibDLS::Data *data, void *cb_data)
{
    Connection *c = (Connection *) cb_data;
    c->_data_callback(data);
    return 0;
}

/*****************************************************************************/

void Connection::_data_callback(LibDLS::Data *data)
{
    DlsProto::Response res;
    DlsProto::Data *data_res = res.mutable_data();
    data_res->set_start_time(data->start_time().to_int64());
    data_res->set_time_per_value(data->time_per_value().to_int64());
    data_res->set_meta_type((DlsProto::MetaType) data->meta_type());
    data_res->set_meta_level(data->meta_level());

    for (size_t i = 0; i < data->size(); i++) {
        data_res->add_value(data->value(i));
    }

    _send(res
#ifdef DLS_PROTO_DEBUG
            , 0
#endif
            );
}

/*****************************************************************************/
