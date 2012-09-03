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

#ifndef DLSProcLoggerHpp
#define DLSProcLoggerHpp

/*****************************************************************************/

#include <string>
#include <list>
#include <sstream>
using namespace std;

/*****************************************************************************/

#include "com_xml_parser.hpp"
#include "com_time.hpp"
#include "dls_job.hpp"

/*****************************************************************************/

enum DLSProcLoggerState
{
    dls_connecting,
    dls_waiting_for_channels,
    dls_getting_channels,
    dls_waiting_for_trigger,
    dls_listening,
    dls_getting_data
};

/*****************************************************************************/

/**
   Logging-Prozess
*/

class DLSProcLogger
{
public:
    DLSProcLogger(const string &, unsigned int);
    ~DLSProcLogger();

    int start();
    void send_command(const string &);
    double max_frequency() const;
    const list<COMRealChannel> *real_channels() const;

private:
    string _dls_dir;
    unsigned int _job_id;
    DLSJob *_job;
    int _socket;
    COMRingBuffer *_ring_buf;
    unsigned int _sig_hangup;
    unsigned int _sig_child;
    string _to_send;
    bool _exit;
    int _exit_code;
    COMXMLParser _xml;
    COMXMLTag _tag;
    DLSProcLoggerState _state;
    int _msr_version;
    list<COMRealChannel> _real_channels;
    COMTime _data_time;
    COMTime _first_data_time;
    bool _got_channels;
    struct timeval _last_trigger_requested;
    struct timeval _last_watchdog;
    COMTime _last_data_received;
    bool _receiving_data;
    unsigned int _buffer_level;
    COMTime _last_read_time;

    void _start();
    bool _connect_socket();
    void _read_write_socket();
    void _read_socket();
    void _check_signals();
    void _parse_ring_buffer();
    void _process_tag();
    void _save_data();
    void _do_trigger();
    void _do_watchdogs();
    void _do_quota();
    void _create_pid_file();
    void _remove_pid_file();
};

/*****************************************************************************/

inline const list<COMRealChannel> *DLSProcLogger::real_channels() const
{
    return &_real_channels;
}

/*****************************************************************************/

#endif


