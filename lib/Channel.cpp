/******************************************************************************
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

#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h> // fchmod()
#include <unistd.h> // close()

#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

/*****************************************************************************/

#include "LibDLS/globals.h"
#include "LibDLS/Chunk.h"
#include "LibDLS/Job.h"
#include "LibDLS/Channel.h"
#include "LibDLS/Dir.h"

#include "proto/dls.pb.h"

#include "IndexT.h"
#include "XmlParser.h"
#include "IndexT.h"
using namespace LibDLS;

#ifdef DEBUG_TIMING
#include <iomanip>
#endif

/*****************************************************************************/

/**
   Constructor.
*/

Channel::Channel(Job *job):
    _job(job),
    _dir_index(0)
{
}

/*****************************************************************************/

Channel::Channel(Job *job, const DlsProto::ChannelInfo &info):
    _job(job),
    _dir_index(info.id()),
    _name(info.name()),
    _unit(info.unit()),
    _type((ChannelType) info.type())
{
}

/*****************************************************************************/

/**
   Destructor.
*/

Channel::~Channel()
{
    _chunks.clear();
}

/*****************************************************************************/

/**
   Imports channel information.
*/

void Channel::import(
        const string &channel_path, /**< channel directory path */
        unsigned int dir_index /**< index of the channel directory */
        )
{
    string file_name;
    fstream file;
    XmlParser xml;

    _path = channel_path;
    _dir_index = dir_index;

    file_name = _path + "/channel.xml";
    file.open(file_name.c_str(), ios::in);

    if (!file.is_open()) {
        stringstream err;
        err << "Failed to open channel file \"" << file_name << "\".";
        throw ChannelException(err.str());
    }

    try
    {
        xml.parse(&file, "dlschannel", dxttBegin);
        xml.parse(&file, "channel", dxttSingle);

        _name = xml.tag()->att("name")->to_str();
        _unit = xml.tag()->att("unit")->to_str();

        if ((_type = str_to_channel_type(xml.tag()->att("type")->to_str()))
            == TUNKNOWN) {
            stringstream err;
            file.close();
            err << "Unknown channel type \""
                << xml.tag()->att("type")->to_str() << "\".";
            throw ChannelException(err.str());
        }

        xml.parse(&file, "dlschannel", dxttEnd);
    }
    catch (EXmlParser &e) {
        stringstream err;
        file.close();
        err << "Parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }
    catch (EXmlParserEOF &e) {
        stringstream err;
        file.close();
        err << "Parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }
    catch (EXmlTag &e) {
        stringstream err;
        file.close();
        err << "XML tag parse error in " << file_name << ": " << e.msg;
        throw ChannelException(err.str());
    }

    file.close();
}

/*****************************************************************************/

std::pair<std::set<Chunk *>, std::set<int64_t> > Channel::fetch_chunks()
{
    if (_job->dir()->access() == Directory::Local) {
        return _fetch_chunks_local();
    }
    else {
        return _fetch_chunks_network();
    }
}

/*****************************************************************************/

/**
   Loads data values of the specified time range and resolution.

   If \a min_values is non-zero, the smallest resolution is chosen, that
   contains at least \a min_values for the requested range.

   Otherwise, all values (the maximum resolution) is loaded.

   The data are passed via the callback function.
*/

void Channel::fetch_data(
        Time start, /**< start of requested time range */
        Time end, /**< end of requested time range */
        unsigned int min_values, /**< minimal number */
        DataCallback cb, /**< callback */
        void *cb_data, /**< arbitrary callback parameter */
        unsigned int decimation /**< Decimation. */
        )
{
    if (_job->dir()->access() == Directory::Local) {
        _fetch_data_local(start, end, min_values, cb, cb_data, decimation);
    }
    else {
        _fetch_data_network(start, end, min_values, cb, cb_data, decimation);
    }
}

/*****************************************************************************/

/**
   Returns true, if this channel has exactly the same chunk times
   as the other channel.
   \return true, if channels have the same chunks.
*/

bool Channel::has_same_chunks_as(const Channel &other) const
{
    return _chunks == other._chunks;
}

/*****************************************************************************/

void Channel::set_channel_info(DlsProto::ChannelInfo *channel_info) const
{
    channel_info->set_id(_dir_index);
    channel_info->set_name(_name);
    channel_info->set_unit(_unit);
    channel_info->set_type((DlsProto::ChannelType) _type);
}

/*****************************************************************************/

void Channel::set_chunk_info(DlsProto::ChannelInfo *channel_info) const
{
    for (ChunkMap::const_iterator chunk_i = _chunks.begin();
            chunk_i != _chunks.end(); chunk_i++) {
        chunk_i->second.set_chunk_info(channel_info->add_chunk());
    }
}

/*****************************************************************************/

void Channel::update_index()
{
    if (_job->dir()->access() == Directory::Local) {
        return _update_index_local();
    }
    else {
        stringstream err;
        err << "Updating remote indices not implemented yet!";
        throw ChannelException(err.str());
    }
}

/*****************************************************************************/

#ifdef DEBUG_TIMING
#define TRACE_TIMING(TIME) \
    do { \
        t_now.set_now(); \
        (TIME) += t_now - t_prev; \
        t_prev = t_now; \
    } while(0)
#else
#define TRACE_TIMING(time)
#endif

/**
   Lädt die Liste der Chunks und importiert deren Eigenschaften

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

std::pair<std::set<Chunk *>, std::set<int64_t> >
Channel::_fetch_chunks_local()
{
    DIR *dir;
    struct dirent *dir_ent;
    Chunk new_chunk, *chunk;
    bool first = true;
    int64_t dir_time;
    set<int64_t> dir_chunks;
    std::pair<std::set<Chunk *>, std::set<int64_t> > ret;

#ifdef DEBUG_TIMING
    {
        stringstream msg;
        msg << __func__ << "(" << name() << ")";
        log(msg.str());
    }

    Time ts, te, t_now, t_prev, t_index_open, t_index_load, t_opendir,
         t_readdir, t_find, t_import, t_insert, t_range, t_close, t_removed;
    ts.set_now();
    t_prev = ts;
    Chunk::reset_timing();
    unsigned int imported = 0, existing = 0, removed = 0, from_index = 0;
#endif

    _range_start.set_null();
    _range_end.set_null();

    if (_chunks.empty()) {
        // try to open channel index file
        stringstream indexPath;
        indexPath << path() << "/channel.idx";
        IndexT<ChannelIndexRecord> index;

        try {
            index.open_read(indexPath.str());
        }
        catch (EIndexT &e) {
            cerr << "Failed to open index: " << e.msg << endl;
        }

        TRACE_TIMING(t_index_open);

#ifdef DEBUG_TIMING
        {
            stringstream msg;
            msg << "Using channel index with " << index.record_count()
                << " records.";
            log(msg.str());
        }
#endif

        for (unsigned int i = 0; i < index.record_count(); i++) {
            ChannelIndexRecord rec(index[i]);

            ChunkMap::iterator chunk_i = _chunks.find(rec.start_time);
            stringstream chunk_path;
            chunk_path << path() << "/chunk" << rec.start_time;
            if (chunk_i == _chunks.end()) {
                // chunk not existing yet
                try {
                    new_chunk.preload(chunk_path.str(), _type,
                            rec.start_time, rec.end_time);
                }
                catch (ChunkException &e) {
                    stringstream err;
                    err << "WARNING: Failed to preload chunk: " << e.msg;
                    log(err.str());
                    continue;
                }

                pair<int64_t, Chunk> val(rec.start_time, new_chunk);
                pair<ChunkMap::iterator, bool> ins_ret = _chunks.insert(val);
                chunk = &ins_ret.first->second;
                ret.first.insert(chunk);
#ifdef DEBUG_TIMING
                from_index++;
#endif
            }
        }
    }

#ifdef DEBUG_TIMING
    {
        stringstream msg;
        msg << "Finished loading index.";
        log(msg.str());
    }
#endif

    TRACE_TIMING(t_index_load);

    // now read chunks from directory

    if (!(dir = opendir(path().c_str()))) {
        stringstream err;
        err << "Failed to open \"" << path() << "\".";
        throw ChannelException(err.str());
    }

    TRACE_TIMING(t_opendir);

    while ((dir_ent = readdir(dir))) {
        string dir_ent_name;

        TRACE_TIMING(t_readdir);

        dir_ent_name = dir_ent->d_name;
        if (dir_ent_name.find("chunk") != 0) {
            continue;
        }

        // get chunk time from directory name for map indexing
        stringstream str;
        str << dir_ent_name.substr(5);
        str >> dir_time;

        // remember chunk time for later removal check
        dir_chunks.insert(dir_time);

        ChunkMap::iterator chunk_i = _chunks.find(dir_time);
        TRACE_TIMING(t_find);
        if (chunk_i == _chunks.end()) {
            // chunk not existing yet
#ifdef DEBUG_TIMING
            {
                stringstream msg;
                msg << "Importing " << dir_ent_name;
                log(msg.str());
            }
#endif
            stringstream chunk_path;
            chunk_path << path() << "/" << dir_ent_name;
            try {
                new_chunk.import(chunk_path.str(), _type);
            }
            catch (ChunkException &e) {
                TRACE_TIMING(t_import);
                stringstream err;
                err << "WARNING: Failed to import " << chunk_path.str()
                    << ": " << e.msg;
                log(err.str());
                continue;
            }
            TRACE_TIMING(t_import);

            pair<int64_t, Chunk> val(dir_time, new_chunk);
            pair<ChunkMap::iterator, bool> ins_ret = _chunks.insert(val);
            chunk = &ins_ret.first->second;
            ret.first.insert(chunk);
            TRACE_TIMING(t_insert);
#ifdef DEBUG_TIMING
            imported++;
#endif
        }
        else {
            // chunk existing
            chunk = &chunk_i->second;
#ifdef DEBUG_TIMING
            existing++;
#endif
        }

        if (chunk->incomplete()) {
            // chunk is still logging, fetch current end time
#ifdef DEBUG_TIMING
            {
                stringstream msg;
                msg << "Fetching range of " << chunk->start().to_int64();
                log(msg.str());
            }
#endif
            try {
                chunk->fetch_range();
            }
            catch (ChunkException &e) {
                TRACE_TIMING(t_range);
                stringstream err;
                err << "WARNING: Failed to fetch chunk range: " << e.msg;
                log(err.str());
                continue;
            }
            TRACE_TIMING(t_range);
            ret.first.insert(chunk);
        }

        if (first) {
            _range_start = chunk->start();
            _range_end = chunk->end();
            first = false;
        }
        else {
            if (chunk->start() < _range_start) {
                _range_start = chunk->start();
            }
            if (chunk->end() > _range_end) {
                _range_end = chunk->end();
            }
        }
    }

    closedir(dir);

    TRACE_TIMING(t_close);

    // check for removed chunks and erase them from map
    ChunkMap::iterator ch_i = _chunks.begin();
    while (ch_i != _chunks.end()) {
        ChunkMap::iterator cur = ch_i++;
        if (dir_chunks.find(cur->first) == dir_chunks.end()) {
            ret.second.insert(cur->first);
            _chunks.erase(cur);
#ifdef DEBUG_TIMING
            {
                stringstream msg;
                msg << "Removing " << cur->second.start().to_int64();
                log(msg.str());
            }
            removed++;
#endif
        }
    }

    TRACE_TIMING(t_removed);

#ifdef DEBUG_TIMING
    te.set_now();
    {
        stringstream msg;
        msg << __func__ << "() " << ts.diff_str_to(te) << endl
            << " imported " << imported
            << " / existing " << existing
            << " / from index " << from_index
            << " / removed " << removed << endl
            << " ret.first " << ret.first.size()
            << " / ret.second " << ret.second.size() << endl
            << fixed << setprecision(0) << setw(8)
            << " index open: " << t_index_open.to_dbl() << endl
            << " index load: " << t_index_load.to_dbl() << endl
            << "       open: " << t_opendir.to_dbl() << endl
            << "    readdir: " << t_readdir.to_dbl() << endl
            << "       find: " << t_find.to_dbl() << endl
            << "     import: " << t_import.to_dbl() << endl
            << "     insert: " << t_insert.to_dbl() << endl
            << "      range: " << t_range.to_dbl() << endl
            << "      close: " << t_close.to_dbl() << endl
            << "     remove: " << t_removed.to_dbl();
        log(msg.str());
        Chunk::output_timing();
    }
#endif

    return ret;
}

/*****************************************************************************/

std::pair<std::set<Chunk *>, std::set<int64_t> >
Channel::_fetch_chunks_network()
{
    DlsProto::Request req;
    DlsProto::Response res;
    Chunk new_chunk;
    Chunk *chunk;
    std::pair<std::set<Chunk *>, std::set<int64_t> > ret;
#ifdef DEBUG_TIMING
    {
        stringstream msg;
        msg << __func__ << "(" << name() << ")";
        log(msg.str());
    }
    Time ts;
    ts.set_now();
#endif

    DlsProto::JobRequest *job_req = req.mutable_job_request();
    job_req->set_id(_job->id());
    DlsProto::ChannelRequest *ch_req = job_req->mutable_channel_request();
    ch_req->set_id(_dir_index);
    ch_req->set_fetch_chunks(true);

    try {
        _job->dir()->_send_message(req);
    }
    catch (DirectoryException &e) {
        stringstream err;
        err << "Failed to request chunks: " << e.msg;
        log(err.str());
        return ret;
    }

    try {
        _job->dir()->_receive_message(res);
    }
    catch (DirectoryException &e) {
        stringstream err;
        err << "Failed to receive chunks: " << e.msg;
        log(err.str());
        return ret;
    }

    if (res.has_error()) {
        stringstream err;
        err << "Error response: " << res.error().message();
        log(err.str());
        return ret;
    }

    const DlsProto::DirInfo &dir_info = res.dir_info();

    if (dir_info.job_size() != 1) {
        stringstream err;
        err << "Job size is " << dir_info.job_size();
        log(err.str());
        return ret;
    }

    const DlsProto::JobInfo &job_info = dir_info.job(0);

    if (job_info.channel_size() != 1) {
        stringstream err;
        err << "Channel size is " << job_info.channel_size();
        log(err.str());
        return ret;
    }

    const DlsProto::ChannelInfo &ch_info = job_info.channel(0);

    google::protobuf::RepeatedPtrField<DlsProto::ChunkInfo>::const_iterator
        ch_info_i;
    for (ch_info_i = ch_info.chunk().begin();
            ch_info_i != ch_info.chunk().end(); ch_info_i++) {
        uint64_t start = ch_info_i->start();
        ChunkMap::iterator chunk_i = _chunks.find(start);
        if (chunk_i == _chunks.end()) {
            new_chunk = Chunk(*ch_info_i, _type);
            pair<int64_t, Chunk> val(start, new_chunk);
            pair<ChunkMap::iterator, bool> ins_ret = _chunks.insert(val);
            chunk = &ins_ret.first->second;
        } else {
            // chunk existing
            chunk_i->second.update_from_chunk_info(*ch_info_i);
            chunk = &chunk_i->second;
        }
        ret.first.insert(chunk);
    }

    // update time range
    ChunkMap::const_iterator ch_i = _chunks.begin();
    if (ch_i != _chunks.end()) {
        _range_start = ch_i->second.start();
        _range_end = ch_i->second.end();
        ch_i++;

        for (; ch_i != _chunks.end(); ch_i++) {
            if (ch_i->second.start() < _range_start) {
                _range_start = ch_i->second.start();
            }
            if (ch_i->second.end() > _range_end) {
                _range_end = ch_i->second.end();
            }
        }
    }
    else {
        _range_start.set_null();
        _range_end.set_null();
    }

    for (google::protobuf::RepeatedField<uint64_t>::const_iterator rem_i =
            ch_info.removed_chunks().begin();
            rem_i != ch_info.removed_chunks().end(); rem_i++) {
        ret.second.insert(*rem_i);
        _chunks.erase(*rem_i);
    }

#ifdef DEBUG_TIMING
    Time te;
    te.set_now();
    stringstream msg;
    msg << __func__ << "() " << setw(20) << ts.diff_str_to(te);
    if (res.has_response_time()) {
        Time r(res.response_time());
        Time z;
        msg << " server: " << setw(20) << z.diff_str_to(r);
    }
    log(msg.str());
    cerr << msg.str() << endl;
#endif
    return ret;
}

/*****************************************************************************/

void Channel::_fetch_data_local(
        Time start, /**< start of requested time range */
        Time end, /**< end of requested time range */
        unsigned int min_values, /**< minimal number */
        DataCallback cb, /**< callback */
        void *cb_data, /**< arbitrary callback parameter */
        unsigned int decimation /**< Decimation. */
        )
{
#ifdef DEBUG_TIMING
    {
        stringstream msg;
        msg << __func__ << "(" << name()
            << ", " << start.diff_str_to(end)
            << ", " << min_values << ")";
        log(msg.str());
    }
    Time ts, te;
    Chunk::reset_timing();
    ts.set_now();
#endif

    ChunkMap::iterator chunk_i;

    if (start < end) {
        try {
            for (chunk_i = _chunks.begin(); chunk_i != _chunks.end();
                    chunk_i++) {
                chunk_i->second.fetch_data(start, end,
                        min_values, cb, cb_data, decimation);
            }
        } catch (ChunkException &e) {
            stringstream err;
            err << "Failed to fetch data from chunk: " << e.msg;
            throw ChannelException(err.str());
        }
    }

#ifdef DEBUG_TIMING
    te.set_now();
    stringstream msg;
    msg << __func__ << "() took " << ts.diff_str_to(te);
    log(msg.str());
    Chunk::output_timing();
#endif
}

/*****************************************************************************/

void Channel::_fetch_data_network(
        Time start, /**< start of requested time range */
        Time end, /**< end of requested time range */
        unsigned int min_values, /**< minimal number */
        DataCallback cb, /**< callback */
        void *cb_data, /**< arbitrary callback parameter */
        unsigned int decimation /**< Decimation. */
        ) const
{
    DlsProto::Request req;
    DlsProto::Response res;
#ifdef DEBUG_TIMING
    {
        stringstream msg;
        msg << __func__ << "(" << name()
            << ", " << start.diff_str_to(end)
            << ", " << min_values << ")";
        log(msg.str());
    }
    Time ts;
    ts.set_now();
#endif

    DlsProto::JobRequest *job_req = req.mutable_job_request();
    job_req->set_id(_job->id());
    DlsProto::ChannelRequest *ch_req = job_req->mutable_channel_request();
    ch_req->set_id(_dir_index);
    DlsProto::DataRequest *data_req = ch_req->mutable_data_request();
    data_req->set_start(start.to_uint64());
    data_req->set_end(end.to_uint64());
    data_req->set_min_values(min_values);
    data_req->set_decimation(decimation);

    try {
        _job->dir()->_send_message(req);
    }
    catch (DirectoryException &e) {
        stringstream err;
        err << "Failed to request data: " << e.msg;
        log(err.str());
        return;
    }

    while(1) {
        try {
            _job->dir()->_receive_message(res, 0);
        }
        catch (DirectoryException &e) {
            stringstream err;
            err << "Failed to receive data: " << e.msg;
            log(err.str());
            break;
        }

        if (res.has_error()) {
            stringstream err;
            err << "Error response: " << res.error().message();
            log(err.str());
            continue;
        }

        if (res.has_end_of_response() && res.end_of_response()) {
            break;
        }

        if (!res.has_data()) {
            stringstream err;
            err << "Error: Expected data!";
            log(err.str());
            continue;
        }

        const DlsProto::Data &data_res = res.data();
        Data *d = new Data(data_res);
        int adopted = cb(d, cb_data);
        if (!adopted) {
            delete d;
        }
    }

#ifdef DEBUG_TIMING
    Time te;
    te.set_now();
    stringstream msg;
    msg << __func__ << "() " << setw(22) << ts.diff_str_to(te);
    if (res.has_response_time()) {
        Time r(res.response_time());
        Time z;
        msg << " server: " << setw(20) << z.diff_str_to(r);
    }
    log(msg.str());
    cerr << msg.str() << endl;
#endif
}

/*****************************************************************************/

void Channel::_update_index_local()
{
    {
        stringstream msg;
        msg << "Updating index of channel (" << _dir_index << ") " << _name;
        log(msg.str());
    }

    fetch_chunks();

    IndexT<ChannelIndexRecord> index;

    stringstream path;
    path << _path << "/channel.idx";
    string index_path(path.str());

    stringstream tmp;
    tmp << _path << "/.channel.idx.XXXXXX";
    string tmp_path(tmp.str());

    int tmp_fd = mkstemp((char *) tmp_path.c_str());
    if (tmp_fd == -1) {
        stringstream err;
        err << "Failed to create " << tmp_path << ": " << strerror(errno);
        throw ChannelException(err.str());
    }

    int ret = fchmod(tmp_fd, 0644);
    if (ret == -1) {
        stringstream err;
        err << "Failed to set temporary file mode of " << tmp_path
            << ": " << strerror(errno);
        close(tmp_fd);
        unlink(tmp_path.c_str());
        throw ChannelException(err.str());
    }

    try {
        index.open_read_append(tmp_path);
    }
    catch (EIndexT &e) {
        stringstream err;
        err << "Failed to open index: " << e.msg;
        close(tmp_fd);
        unlink(tmp_path.c_str());
        throw ChannelException(err.str());
    }

    close(tmp_fd);

    unsigned int record_count(0);
    unsigned int incomplete(0);

    for (Channel::ChunkMap::const_iterator chunk_i =
            _chunks.begin(); chunk_i != _chunks.end(); chunk_i++) {
        const Chunk *c = &chunk_i->second;
        ChannelIndexRecord rec;
        rec.start_time = c->start().to_uint64();
        if (c->incomplete()) {
            rec.end_time = 0ULL;
            incomplete++;
        }
        else {
            rec.end_time = c->end().to_uint64();
        }
        index.append_record(&rec);
        record_count++;
    }

    index.close();

    if (rename(tmp_path.c_str(), index_path.c_str()) == -1) {
        stringstream err;
        err << "Failed to rename " << tmp_path << " to "
            << index_path << ": " << strerror(errno);
        unlink(tmp_path.c_str());
        throw ChannelException(err.str());
    }

    {
        stringstream msg;
        msg << "    Created channel index with " << record_count
            << " records (" << incomplete << " incomplete).";
        log(msg.str());
    }
}

/*****************************************************************************/
