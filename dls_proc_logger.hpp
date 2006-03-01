//---------------------------------------------------------------
//
//  D L S _ P R O C _ L O G G E R . H P P
//
//---------------------------------------------------------------

#ifndef DLSProcLoggerHpp
#define DLSProcLoggerHpp

//---------------------------------------------------------------

#include <string>
#include <list>
#include <sstream>
using namespace std;

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_time.hpp"
#include "dls_job.hpp"

//---------------------------------------------------------------

enum DLSProcLoggerState
{
  dls_connecting,
  dls_waiting_for_frequency,
  dls_waiting_for_channels,
  dls_getting_channels,
  dls_waiting_for_trigger,
  dls_listening,
  dls_getting_data
};

//---------------------------------------------------------------

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
  COMRingBufferT<char, unsigned int> *_ring_buf;
  unsigned int _sig_hangup;
  unsigned int _sig_child;
  string _to_send;
  bool _exit;
  int _exit_code;
  COMXMLParser _xml;
  COMXMLTag _tag;
  DLSProcLoggerState _state;
  int _msr_version;
  double _max_frequency;
  list<COMRealChannel> _real_channels;
  COMTime _data_time;
  COMTime _first_data_time;
  bool _got_channels;
  struct timeval _last_trigger_requested;
  struct timeval _last_watchdog;
  COMTime _last_data_received;
  bool _no_data_warning;
  unsigned int _buffer_level;

  void _start();
  bool _connect_socket();
  void _read_write_socket();
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

//---------------------------------------------------------------

inline double DLSProcLogger::max_frequency() const
{
  return _max_frequency;
}

//---------------------------------------------------------------

inline const list<COMRealChannel> *DLSProcLogger::real_channels() const
{
  return &_real_channels;
}

//---------------------------------------------------------------

#endif


