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

#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

/*****************************************************************************/

#include "LibDLS/globals.h"
#include "LibDLS/Dir.h"
#include "LibDLS/Job.h"

#include "proto/dls.pb.h"

#include "XmlParser.h"
#include "IndexT.h"
#include "File.h"
#include "BaseMessageList.h"
#include "BaseMessage.h"

using namespace LibDLS;

/*****************************************************************************/

/** Constructor.
 */
Job::Job(Directory *dir):
    _dir(dir),
    _messages(new BaseMessageList())
{
}

/*****************************************************************************/

Job::Job(
        Directory *dir,
        const DlsProto::JobInfo &job_info
        ):
    _dir(dir),
    _messages(new BaseMessageList())
{
    _preset.import_from(job_info.preset());

    google::protobuf::RepeatedPtrField<DlsProto::ChannelInfo>::const_iterator
        ch_i;
    for (ch_i = job_info.channel().begin();
            ch_i != job_info.channel().end(); ch_i++) {
        _channels.push_back(Channel(this, *ch_i));
    }
}

/*****************************************************************************/

/**
   Destructor
*/

Job::~Job()
{
    delete _messages;
}

/*****************************************************************************/

/**
   Imports job information.
*/

void Job::import(const string &dls_path, /**< DLS directory path */
                 unsigned int job_id /**< job ID */
                 )
{
    stringstream job_dir;

    job_dir << dls_path << "/job" << job_id;

    _path = job_dir.str();
    _channels.clear();

    try {
        _preset.import(dls_path, job_id);
    }
    catch (EJobPreset &e) {
        stringstream err;
        err << "WARNING: " << e.msg;
        log(err.str());
        return;
    }

    bool exists;

    try {
        exists = _messages->exists(_path);
    }
    catch (LibDLS::BaseMessageList::Exception &e) {
        stringstream err;
        err << "Failed to check for message file "
            << _messages->path(_path) << ": " << e.msg;
        log(err.str());
        return;
    }

    if (exists) {
        try {
            _messages->import(_path);
        }
        catch (LibDLS::BaseMessageList::Exception &e) {
            stringstream err;
            err << "Failed to import messages: " << e.msg;
            log(err.str());
        }

#if 0
        stringstream msg;
        msg << "Imported " << _messages->count() << " messages.";
        log(msg.str());
#endif
    }
}

/*****************************************************************************/

/**
   Imports all channels.
*/

void Job::fetch_channels()
{
    _channels.clear();

    if (_dir->access() == Directory::Local) {
        _fetch_channels_local();
    }
    else {
        _fetch_channels_network();
    }

    _channels.sort();
}

/*************************************************************************/

/**
*/

LibDLS::Channel *LibDLS::Job::channel(unsigned int index)
{
    list<Channel>::iterator channel_i;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++, index--) {
        if (!index) return &(*channel_i);
    }

    return NULL;
}

/*************************************************************************/

/**
*/

LibDLS::Channel *LibDLS::Job::find_channel(
        unsigned int index
        )
{
    list<Channel>::iterator channel_i;

    for (channel_i = _channels.begin();
         channel_i != _channels.end();
         channel_i++) {
        if (channel_i->dir_index() == index) return &(*channel_i);
    }

    return NULL;
}

/*************************************************************************/

/**
*/

set<LibDLS::Channel *> LibDLS::Job::find_channels_by_name(
        const std::string &name
        )
{
    set<LibDLS::Channel *> res;

    for (list<Channel>::iterator channel_i = _channels.begin();
         channel_i != _channels.end(); channel_i++) {

        if (channel_i->name() == name) {
            res.insert(&(*channel_i));
        }
    }

    return res;
}

/*************************************************************************/

/**
   Less-Operator for sorting.
   \return the "left" job is less than the "right" one
*/

bool LibDLS::Job::operator<(const Job &right) const
{
    return _preset.id() < right._preset.id();
}

/*****************************************************************************/

/** Lädt Nachrichten im angegebenen Zeitbereich.
 *
 * \param start Anfangszeit des Bereiches
 * \param end Endzeit des Bereiches
 */
list<LibDLS::Job::Message> LibDLS::Job::load_msg(
        Time start, /**< Start time. */
        Time end, /**< End time. */
        std::string lang /**< Language for message translations. If empty,
                           "en" is tried, otherwise the first available
                           translation is used. */
        ) const
{
    IndexT<MessageIndexRecord> index;
    MessageIndexRecord index_record;
    File file;
    RingBuffer ring(10000); // FIXME
    XmlParser xml;
    list<Message> ret;
    Message msg;
    stringstream msg_dir, str, msg_chunk_dir;
    char *write_ptr;
    unsigned int i, write_len;
    DIR *dir;
    struct dirent *dir_ent;
    string entry_name;
    uint64_t msg_chunk_time;
    list<uint64_t> chunk_times;
    list<uint64_t>::iterator chunk_time_i;

    msg_dir << _path << "/messages";

    // Das Message-Verzeichnis öffnen
    if (!(dir = opendir(msg_dir.str().c_str()))) {
        if (errno != ENOENT) {
            stringstream err;
            err << "ERROR: Failed to open message directory \""
                << msg_dir.str() << "\":" << strerror(errno);
            log(err.str());
        }
        return ret;
    }

    // Alle Message-Chunks durchlaufen
    while ((dir_ent = readdir(dir))) {
        entry_name = dir_ent->d_name;

        // Wenn das Verzeichnis nicht mit "chunk" beginnt,
        // das Nächste verarbeiten
        if (entry_name.substr(0, 5) != "chunk")
            continue;

        str.str("");
        str.clear();
        str << entry_name.substr(5); // Alles nach "chunk" in den
                                     // Stringstream einfügen
        try {
            // Den Zeitstempel auslesen
            str >> msg_chunk_time;
        }
        catch (...) {
            // Der Rest des Verzeichnisnamens ist kein Zeitstempel
            continue;
        }

        // Die Chunk-Zeit in die Liste einfügen
        chunk_times.push_back(msg_chunk_time);
    }

    // Message-Verzeichnis wieder schliessen
    closedir(dir);

    // Chunk-Zeiten sortieren
    chunk_times.sort();

    // Alle Chunks aus der Liste nehmen, die hinter dem Ende liegen
    while (!chunk_times.empty() && Time(chunk_times.back()) > end) {
        chunk_times.pop_back();
    }

#if DEBUG
    cerr << start << " - " << end << endl;
    for (chunk_time_i = chunk_times.begin();
         chunk_time_i != chunk_times.end();
         chunk_time_i++) {
        cerr << *chunk_time_i << " = "
            << Time(*chunk_time_i).to_real_time() << endl;
    }
#endif

    // Alle Chunks entfernen, dessen Nachfolger noch vor dem Start sind
    while (chunk_times.size() > 1) {
        if (Time(*(++chunk_times.begin())) > start) {
            break;
        }
        chunk_times.pop_front();
    }

#if DEBUG
    for (chunk_time_i = chunk_times.begin();
         chunk_time_i != chunk_times.end();
         chunk_time_i++) {
        cerr << *chunk_time_i << endl;
    }
#endif

    // Alle übriggebliebenen Message-Chunks durchlaufen
    for (chunk_time_i = chunk_times.begin();
         chunk_time_i != chunk_times.end();
         chunk_time_i++) {
        msg_chunk_dir.str("");
        msg_chunk_dir.clear();
        msg_chunk_dir << msg_dir.str() << "/chunk" << *chunk_time_i;

        try {
            file.open_read((msg_chunk_dir.str() + "/messages").c_str());
            index.open_read((msg_chunk_dir.str() + "/messages.idx").c_str());

#if DEBUG
            cerr << (msg_chunk_dir.str() + "/messages.idx").c_str() << ": "
                << index.record_count() << " index records." << endl;
#endif

            for (i = 0; i < index.record_count(); i++) {
                bool skip = false;
                index_record = index[i];

#if DEBUG
                cerr << "idxrec " << index_record.time << ": "
                    << index_record.position << endl;
#endif

                if (Time(index_record.time) < start) continue;
                if (Time(index_record.time) > end) break;

                file.seek(index_record.position);
                ring.clear();

                // Solange lesen, bis ein Tag komplett ist
                while (1) {
                    ring.write_info(&write_ptr, &write_len);

                    if (!write_len) {
                        stringstream err;
                        err << "ERROR: Message ringbuffer full!";
                        log(err.str());
                        return ret;
                    }

                    if (write_len > 300) write_len = 300;

                    file.read(write_ptr, write_len, &write_len);

                    if (!write_len) {
                        stringstream err;
                        err << "Warning: Message file "
                            << (msg_chunk_dir.str() + "/messages").c_str()
                            << " inconsistent!";
                        log(err.str());
                        skip = true;
                        break;
                    }

                    ring.written(write_len);

                    try {
                        xml.parse(&ring);
                    }
                    catch (EXmlParserEOF &e) {
                        // Noch nicht genug Daten. Mehr einlesen!
                        continue;
                    }

                    break;
                }

                if (skip) {
                    break;
                }

                msg.time = index_record.time;

                try {
                    msg.text = xml.tag()->att("text")->to_str();
                }
                catch (EXmlTag &e) {
                    stringstream err;
                    err << "Message element: " << e.msg
                        << " Tag: " << e.tag;
                    log(err.str());
                    msg.text = string();
                }

                if (xml.tag()->title() == "info") {
                    msg.type = Message::Info;
                }
                else if (xml.tag()->title() == "warn") {
                    msg.type = Message::Warning;
                }
                else if (xml.tag()->title() == "error") {
                    msg.type = Message::Error;
                }
                else if (xml.tag()->title() == "crit_error") {
                    msg.type = Message::Critical;
                }
                else if (xml.tag()->title() == "broadcast") {
                    msg.type = Message::Broadcast;
                }
                else {
                    stringstream err;
                    err << "Unknown message type "
                        << xml.tag()->title();
                    log(err.str());
                    msg.type = Message::Unknown;
                }

                // lookup message text
                const BaseMessage *m = _messages->findPath(msg.text);
                if (m) {
                    string text = m->text(lang);
                    if (text != "") {
                        msg.text = text;
                    }
                }

                ret.push_back(msg);
            }
        }
        catch (EIndexT &e) {
            stringstream err;
            err << "FEHLER im Message-Index: " << e.msg;
            log(err.str());
            return ret;
        }
        catch (EFile &e) {
            stringstream err;
            err << "FEHLER in der Message-Datei: " << e.msg;
            log(err.str());
            return ret;
        }
        catch (EXmlParser &e) {
            stringstream err;
            err << "FEHLER beim Parsen: " << e.msg;
            log(err.str());
            return ret;
        }
    }

    return ret;
}

/*****************************************************************************/

void Job::set_job_info(DlsProto::JobInfo *job_info, bool preset) const
{
    if (preset) {
        _preset.set_job_preset_info(job_info->mutable_preset());
    }

    /* Channels */
    for (list<LibDLS::Channel>::const_iterator ch_i = _channels.begin();
            ch_i != _channels.end(); ch_i++) {
        ch_i->set_channel_info(job_info->add_channel());
    }
}

/*****************************************************************************/

void Job::_fetch_channels_local()
{
    stringstream str;
    DIR *dir;
    struct dirent *dir_ent;
    string channel_dir_name;
    int channel_index;
    Channel channel(this);

    str.exceptions(ios::failbit | ios::badbit);

    if (!(dir = opendir(_path.c_str()))) {
        stringstream err;
        err << "ERROR: Failed to open job directory \"" << _path << "\".";
        log(err.str());
        return;
    }

    while ((dir_ent = readdir(dir))) {
        channel_dir_name = dir_ent->d_name;
        if (channel_dir_name.find("channel")) continue;

        str.str("");
        str.clear();
        str << channel_dir_name.substr(7);

        try {
            str >> channel_index;
        }
        catch (...) {
            continue;
        }

        try {
            channel.import(_path + "/" + channel_dir_name, channel_index);
        }
        catch (ChannelException &e) {
            stringstream err;
            err << "WARNING: " << e.msg;
            log(err.str());
            continue;
        }

        _channels.push_back(channel);
    }


    closedir(dir);
}

/*****************************************************************************/

void Job::_fetch_channels_network()
{
    DlsProto::Request req;
    DlsProto::Response res;

    DlsProto::JobRequest *job_req = req.mutable_job_request();
    job_req->set_id(_preset.id());
    job_req->set_fetch_channels(true);

    _dir->network_request(req, res);

    if (res.has_error()) {
        cerr << "Error response: " << res.error().message() << endl;
        return;
    }

    const DlsProto::DirInfo &dir_info = res.dir_info();
    const DlsProto::JobInfo &job_info = dir_info.job(0); // FIXME check

    google::protobuf::RepeatedPtrField<DlsProto::ChannelInfo>::const_iterator
        ch_i;
    for (ch_i = job_info.channel().begin();
            ch_i != job_info.channel().end(); ch_i++) {
        _channels.push_back(Channel(this, *ch_i));
    }
}

/*****************************************************************************/
