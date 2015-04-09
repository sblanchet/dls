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

#ifndef ProcLoggerH
#define ProcLoggerH

/*****************************************************************************/

#include <string>
#include <list>
#include <sstream>
using namespace std;

/*****************************************************************************/

#include "lib/XmlParser.h"
#include "lib/LibDLS/Time.h"

#include "Job.h"

/*****************************************************************************/

/**
   Logging-Prozess
*/

class ProcLogger
{
public:
    ProcLogger(const string &, unsigned int);
    ~ProcLogger();

    int start();
    void send_command(const string &);
    double max_frequency() const;
    const list<LibDLS::RealChannel> *real_channels() const;

private:
    string _dls_dir;
    unsigned int _job_id;
    Job *_job;
    int _socket;
	LibDLS::RingBuffer *_ring_buf;
    unsigned int _sig_hangup;
    unsigned int _sig_child;
    unsigned int _sig_usr1;
    string _to_send;
    bool _exit;
    int _exit_code;
	LibDLS::XmlParser _xml;
	LibDLS::XmlTag _tag;
    enum State {
        dls_connecting,
        dls_waiting_for_channels,
        dls_getting_channels,
        dls_waiting_for_trigger,
        dls_listening,
        dls_getting_data
    };
    State _state;
    int _msr_version;
    list<LibDLS::RealChannel> _real_channels;
	LibDLS::Time _data_time;
	LibDLS::Time _first_data_time;
    bool _got_channels;
    struct timeval _last_trigger_requested;
    struct timeval _last_watchdog;
	LibDLS::Time _last_data_received;
    bool _receiving_data;
    unsigned int _buffer_level;
	LibDLS::Time _last_read_time;

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
    void _flush();
};

/*****************************************************************************/

inline const list<LibDLS::RealChannel> *ProcLogger::real_channels() const
{
    return &_real_channels;
}

/*****************************************************************************/

#endif


