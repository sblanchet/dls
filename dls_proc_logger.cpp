//---------------------------------------------------------------
//
//  D L S _ P R O C _ L O G G E R . C P P
//
//---------------------------------------------------------------

#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <syslog.h>

#include <fstream>
using namespace std;

//---------------------------------------------------------------

#include "com_ring_buffer_t.hpp"
#include "com_real_channel.hpp"
#include "dls_globals.hpp"
#include "dls_proc_logger.hpp"

//---------------------------------------------------------------

/**
   Konstruktor 

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

DLSProcLogger::DLSProcLogger(const string &dls_dir, int job_id)
{
  _dls_dir = dls_dir;
  _sig_hangup = sig_hangup;
  _exit = false;
  _exit_code = E_DLS_NO_ERROR;
  _state = dls_connecting;
  _got_channels = false;
  _socket = 0;
  _last_trigger_requested.tv_sec = 0;
  _last_trigger_requested.tv_usec = 0;
  _last_watchdog.tv_sec = 0;
  _last_watchdog.tv_usec = 0;
  _no_data_warning = true;
  _buffer_level = 0;
  
  openlog("dlsd_logger", LOG_PID, LOG_DAEMON);

  try
  {
    _job = new DLSJob(this, _dls_dir, job_id);
  }
  catch (...)
  {
    _job = 0;
    _msg << "could not allocate memory for job.";
    log(DLSError);
  }

  try
  {
    _ring_buf = new COMRingBufferT<char, unsigned int>(RECEIVE_RING_BUF_SIZE);
  }
  catch (...)
  {
    _ring_buf = 0;
    _msg << "could not allocate memory for ring buffer.";
    log(DLSError);
  }
}

//---------------------------------------------------------------

DLSProcLogger::~DLSProcLogger()
{
  if (_job)
  {
    try
    {
      delete _job;
    }
    catch (EDLSJob &e)
    {
      _msg << "deleting job: " << e.msg;
      log(DLSError);    
    }
  }

  if (_ring_buf)
  {
    delete _ring_buf;
  }

  closelog();
}

//---------------------------------------------------------------

/**
   Starten des Logging-Prozesses

   \return Exit-Code
*/

int DLSProcLogger::start()
{
  _msg << "process started!";
  log(DLSInfo);

  _start();

  if (_exit_code == E_DLS_NO_ERROR)
  {
    _msg << "----- process finished. exiting gracefully. -----";
  }
  else
  {
    _msg << "----- process finished. exiting with ERROR! (code " << _exit_code << ") -----";
  }

  log(DLSInfo);

  return _exit_code;
}

//---------------------------------------------------------------

/**
   Starten des Logging-Prozesses (intern)
*/

void DLSProcLogger::_start()
{
  try
  {
    // Auftragsdaten importieren
    _job->import();
  }
  catch (EDLSJob &e)
  {
    _exit_code = E_DLS_ERROR;

    _msg << "importing: " << e.msg;
    log(DLSError);

    return;
  }

  // Mit Prüfstand verbinden
  if (!_connect_socket())
  {
    _exit_code = E_DLS_ERROR;

    return;
  }
    
  // Kommunikation starten
  _read_write_socket();

  // Verbindung zu MSR schliessen
  close(_socket);

  try
  {
    _job->finish();
  }
  catch (EDLSJob &e)
  {
    _exit_code = E_DLS_ERROR;

    _msg << "finishing: " << e.msg;
    log(DLSError);
  }
}

//---------------------------------------------------------------

bool DLSProcLogger::_connect_socket()
{
  struct sockaddr_in address;
  struct hostent *hp;
  const char *source = _job->preset()->source().c_str();

  // Socket öffnen
  if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    _msg << "could not create socket!"; 
    log(DLSError);

    return false;
  }

  // Socket geöffnet, Adresse übersetzen/auflösen
  address.sin_family = AF_INET;
  if ((hp = gethostbyname(source)) == NULL)
  {
    close(_socket);

    _msg << "could not resolve \"" << source << "\"!";
    log(DLSError);

    return false;
  }
  
  // Adresse in sockaddr-Struktur kopieren
  memcpy((char *) &address.sin_addr, (char *) hp->h_addr, hp->h_length);
  address.sin_port = htons(MSRD_PORT);
    
  // Verbinden
  if ((::connect(_socket, (struct sockaddr *) &address, sizeof(address))) == -1)
  {
    close(_socket);

    _msg << "could not connect to \"" << source << "\"!";
    log(DLSError);
    
    return false;
  }

  // Verbunden!
  _msg << "connected to \"" << source << "\"!";
  log(DLSInfo);

  return true;
}

//---------------------------------------------------------------

void DLSProcLogger::_read_write_socket()
{
  fd_set read_fds, write_fds;
  int select_ret, recv_ret, send_ret;
  struct timeval timeout;
  char *write_addr;
  unsigned int write_size;

  while (1)
  {
    // File-Descriptor-Sets nullen und mit Client-FD vorbesetzen
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(_socket, &read_fds);
    if (_to_send.length() > 0) FD_SET(_socket, &write_fds);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Warten auf Änderungen oder Timeout
    if ((select_ret = select(_socket + 1, &read_fds, &write_fds, 0, &timeout)) > 0)
    {
      // Eingehende Daten?
      if (FD_ISSET(_socket, &read_fds))
      {
        _ring_buf->write_info(&write_addr, &write_size);

        if (write_size == 0)
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          _msg << "FATAL: ring buffer full!";
          log(DLSError);

          break;
        }

	// Daten abholen...
        if ((recv_ret = recv(_socket, write_addr, write_size, 0)) > 0)
        {
          #if DEBUG_REC
          _msg << "REC: \"" << string(write_addr, recv_ret) << "\""; 
          log(DLSInfo);
          #endif

          _ring_buf->written(recv_ret);

          _parse_ring_buffer();
        }
        else if (recv_ret == 0) // Verbindung geschlossen!
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          _msg << "connection closed by server."; 
          log(DLSError);

          break;
        }
        else // Fehler
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          _msg << "error " << errno << " in recv()!";
          log(DLSError);

          break;
        }
      }

      // Bereit zum Senden?
      if (FD_ISSET(_socket, &write_fds))
      {
        // Daten senden
        if ((send_ret = send(_socket, _to_send.c_str(), _to_send.length(), 0)) > 0)
        {
          #if DEBUG_SEND
          _msg << "SENT: \"" << _to_send.substr(0, send_ret) << "\"";
          log(DLSInfo);
          #endif

          _to_send.erase(0, send_ret); // Gesendetes entfernen
        }
        else if (send_ret == -1)
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          _msg << "error " << errno << " in send()!";
          log(DLSError);

          break;
        }
      }
    }
	
    // Select-Timeout
    else if (select_ret == 0)
    {
    }

    // Select-Fehler
    else if (select_ret == -1 && errno != EINTR)
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;

      _msg << "error " << errno << " in select()!";
      log(DLSError);

      break;
    }

    // Auf gesetzte Signale überprüfen
    _check_signals();

    if (_exit) break;

    // Trigger
    _do_trigger();

    // Watchdog
    _do_watchdogs();

    // Warnung ausgeben, wenn zu lange keine Daten mehr empfangen
    if (COMTime::now() - _last_data_received > (long long) 61000000)
    {
      if (!_no_data_warning)
      {
        if (_state != dls_waiting_for_trigger)
        {
          _msg << "no data received for a long time!";
          log(DLSWarning);
        }
        _no_data_warning = true;
      }
    }
    else if (_no_data_warning)
    {
      _msg << "receiving data!";
      log(DLSInfo);
      _no_data_warning = false;
    }
 
    // Soll der Prozess beendet werden?
    if (_exit) break;
  }
}

//---------------------------------------------------------------

void DLSProcLogger::send_command(const string &cmd)
{
  _to_send += cmd + "\n";
}

//---------------------------------------------------------------

void DLSProcLogger::_check_signals()
{
  if (sig_int_term)
  {
    _exit = true;

    _msg << "SIGINT or SIGTERM received in state " << _state << "!";
    log(DLSInfo);    
  }

  if (_exit) return; // Bei Beendingung nicht mehr SIGHUP verarbeiten

  // Nachricht von Elternprozess: Auftrag hat sich geändert!
  if (sig_hangup != _sig_hangup)
  {
    _sig_hangup = sig_hangup;

    _msg << "received notification from mother process.";
    log(DLSInfo);
    
    try
    {
      _job->import();
    }
    catch (EDLSJob &e)
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;

      _msg << "importing job: " << e.msg;
      log(DLSError);

      return;
    }

    if (!_job->preset()->running()) // Erfassung gestoppt
    {
      _exit = true;

      _msg << "job is no longer running."; 
      log(DLSInfo);
    }

    else // Erfassung soll weiterlaufen
    {
      if (_state == dls_waiting_for_trigger)
      {
        _state = dls_listening;

        _msg << "no trigger any more! start logging.";
        log(DLSInfo);

        _job->start_logging();
      }
      else
      {
        _job->change_logging();
      }
    }
  }
}

//---------------------------------------------------------------

void DLSProcLogger::_parse_ring_buffer()
{
  while (1)
  {
    try
    {      
      _xml.parse(_ring_buf);
    }
    catch (ECOMXMLParserEOF &e) // Tag noch nicht komplett
    {
      break;
    }
    catch (ECOMXMLParser &e) // Anderer Parsing-Fehler
    {
      _msg << "parsing incoming data: " << e.msg << " tag: " << e.tag;
      log(DLSError);

      continue;
    }

    // Tag komplett; verarbeiten!
    _process_tag();

    if (_exit) break;
  }
}

//---------------------------------------------------------------

/**
   \todo Maximalen Fehler der Zeitabweichung auf 50% der Abtastrate (1000Hz->.05ms) zulassen!
*/

void DLSProcLogger::_process_tag()
{
  string send_str, title;
  COMRealChannel real_channel;
  unsigned int new_buffer_level;

  title = _xml.last_tag()->title();

  try
  {
    if (title == "info" || title == "warn"
        || title == "error" || title == "crit_error"|| title == "broadcast") 
    {
      _msg << "MSRD: " << _xml.last_tag()->tag();
      log(DLSInfo);
      
      try
      {
        _job->message(_xml.last_tag()->tag());
      }
      catch (EDLSJob &e)
      {
        _msg << "could not log last message!";
        log(DLSError);
      }
      
      return;
    }

    // Acknoledgement-Tags direkt an _job weiterleiten
    else if (_xml.last_tag()->title() == "ack")
    {
      _job->ack_received(_xml.last_tag()->att("id")->to_str());
      return;
    }

    // Trigger-Parameter verarbeiten
    else if (_xml.last_tag()->title() == "parameter")
    {
      if (_job->preset()->trigger() != "") // Wenn Triggern aktiviert
      {
        if (_xml.last_tag()->att("name")->to_str() == _job->preset()->trigger())
        {
          if (_state == dls_waiting_for_trigger)
          {
            if (_xml.last_tag()->att("value")->to_int() != 0)
            {
              _state = dls_listening; // Zustandswechsel!
              
              _msg << "trigger active! start logging.";
              log(DLSInfo);

              _no_data_warning = true; // Gleich augeben, dass wieder Daten kommen

              _job->start_logging();
            }
          }
        
          else if (_state == dls_listening || _state == dls_getting_data)
          {
            if (_xml.last_tag()->att("value")->to_int() == 0)
            {
              _state = dls_waiting_for_trigger;
              
              _msg << "trigger not active! stop logging.";
              log(DLSInfo);

              _job->stop_logging();

              _msg << "waiting for trigger...";
              log(DLSInfo);
            }
          }
            
          return;
        }
      }
    }

    switch (_state) // Ab jetzt Zustandsabhängigkeit!
    {
      case dls_connecting: //------------------------------------

        // Nur <connected> annehmen
        if (_xml.last_tag()->title() == "connected")
        {
          // Nur mit MSR sprechen
          if (_xml.last_tag()->att("name")->to_str() != "MSR")
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            _msg << "don't wanna talk to anything but MSR!";
            log(DLSError);
          }
          else
          {
            // Version auslesen
            _msr_version = _xml.last_tag()->att("version")->to_int();

            if (_msr_version < MSR_VERSION(2, 7, 0))
            {
              _exit = true;
              _exit_code = E_DLS_ERROR;

              _msg << "expecting version > 2.7.0! actual version:";
              _msg << " " << MSR_V(_msr_version);
              _msg << "." << MSR_P(_msr_version);
              _msg << "." << MSR_S(_msr_version) << "...";
              log(DLSError);
            }
            else
            {
              _state = dls_waiting_for_frequency; // Zustandswechsel!

              // Maximale Abtastfrequenz des Systems auslesen
              send_command("<rp name=\"/Taskinfo/Abtastfrequenz\">");
            }
          }
        }
        break;

      case dls_waiting_for_frequency: //-------------------------

        if (_xml.last_tag()->title() == "parameter")
        {
          if (_xml.last_tag()->att("name")->to_str() != "/Taskinfo/Abtastfrequenz")
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            _msg << "expected /Taskinfo/Abtastfrequenz!";
            log(DLSError);
          }
          else
          {
            _max_frequency = _xml.last_tag()->att("value")->to_int();
      
            _state = dls_waiting_for_channels; // Zustandswechsel!

            _msg << "connected to MSR version";
            _msg << " " << MSR_V(_msr_version);
            _msg << "." << MSR_P(_msr_version);
            _msg << "." << MSR_S(_msr_version);
            _msg << ", " << _max_frequency << " Hz";
            log(DLSInfo);

            // Alle Kanäle auslesen
            send_command("<rk>");
          }
        }
        break;

      case dls_waiting_for_channels: //--------------------------

        if (_xml.last_tag()->title() == "channels"
            && _xml.last_tag()->type() == dxttBegin)
        {
          _state = dls_getting_channels; // Zustandswechsel
        }
        break;

      case dls_getting_channels: //------------------------------

        if (_xml.last_tag()->title() == "channel")
        {
          try
          {
            real_channel.read_from_tag(_xml.last_tag());
            _real_channels.push_back(real_channel);
          }
          catch (ECOMRealChannel &e)
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            _msg << "receiving channel: " << e.msg;
            log(DLSError);
          }
        }
        else if (_xml.last_tag()->title() == "channels"
                 && _xml.last_tag()->type() == dxttEnd)
        {
          _got_channels = true;

          if (_job->preset()->trigger() == "")
          {
            // Alle Logger starten
            _job->start_logging();
            _state = dls_listening; // Zustandswechsel!

            _msg << "start logging.";
          }
          else
          {
            _state = dls_waiting_for_trigger; // Zustandswechsel!

            _msg << "waiting for trigger \"";
            _msg << _job->preset()->trigger() << "\"...";
          }

          log(DLSInfo);
        }
        break;

      case dls_listening: //-------------------------------------

        if (_xml.last_tag()->title() == "data")
        {
          _state = dls_getting_data; // Zustandswechsel!

          _last_data_received.set_now();

          _data_time.from_dbl_time(_xml.last_tag()->att("time")->to_dbl());

          if (_xml.last_tag()->has_att("level"))
          {
            // Meldungen über Füllstand der Kanalpuffer auswerten
            new_buffer_level = _xml.last_tag()->att("level")->to_int();
            if (_buffer_level < BUFFER_LEVEL_WARNING
                && new_buffer_level >= BUFFER_LEVEL_WARNING)
            {
              // Warnung: Füllstand zu hoch!
              _msg << "channel buffers nearly full!";
              log(DLSWarning);
            }
            else if (_buffer_level >= BUFFER_LEVEL_WARNING
                     && new_buffer_level < BUFFER_LEVEL_WARNING)
            {
              // Entwarnung geben
              _msg << "level of channel buffers decreasing again...";
              log(DLSWarning);
            }
            _buffer_level = new_buffer_level;
          }
        }
        break;

      case dls_getting_data: //----------------------------------

        if (_xml.last_tag()->title() == "data"
            && _xml.last_tag()->type() == dxttEnd)
        {
          _state = dls_listening; // Zustandswechsel!
        }
        else if (_xml.last_tag()->title() == "F")
        {
          try
          {
            _job->process_data(_data_time,
                               _xml.last_tag()->att("c")->to_int(),
                               _xml.last_tag()->att("d")->to_str());
          }
          catch (EDLSTimeTolerance &e)
          {
            _exit = true;
            _exit_code = E_DLS_TIME_TOLERANCE;

            _msg << "TIME TOLERANCE EXCEEDED: " << e.msg;
            log(DLSError);
          }
          catch (EDLSJob &e)
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            _msg << "processing data: " << e.msg;
            log(DLSError);            
          }
        }
        break;

      default: break; //-----------------------------------------
    }
  }
  catch (ECOMXMLTag &e)
  {
    _exit = true;
    _exit_code = E_DLS_ERROR;

    _msg << "processing tag: " << e.msg << " tag: " << e.tag;
    log(DLSError);
  }
}

//---------------------------------------------------------------

void DLSProcLogger::_do_trigger()
{
  stringstream cmd;
  struct timeval now;

  if (_job->preset()->trigger() == "") return;

  gettimeofday(&now, 0);

  if (now.tv_sec > _last_trigger_requested.tv_sec + TRIGGER_INTERVAL)
  {
    cmd << "<rp name=\"" << _job->preset()->trigger() << "\" short=\"true\">";
    send_command(cmd.str());

    _last_trigger_requested = now;
  }
}

//---------------------------------------------------------------

void DLSProcLogger::_do_watchdogs()
{
  struct timeval now;
  fstream watchdog_file;
  fstream logging_file;
  stringstream dir_name;

  dir_name << _dls_dir << "/job" << _job->preset()->id();

  gettimeofday(&now, 0);

  if (now.tv_sec > _last_watchdog.tv_sec + WATCHDOG_INTERVAL)
  {
    watchdog_file.open((dir_name.str() + "/watchdog").c_str(), ios::out);

    // Schließen ohne Fehlerbehandlung!
    watchdog_file.close();

    if (_state != dls_waiting_for_trigger && !_no_data_warning)
    {
      logging_file.open((dir_name.str() + "/logging").c_str(), ios::out);

      // Schließen ohne Fehlerbehandlung!
      logging_file.close();
    }

    _last_watchdog = now;
  }
}

//---------------------------------------------------------------

void DLSProcLogger::log(DLSLogType type)
{
  string mode;

  if (type == DLSError) mode = "ERROR";
  else if (type == DLSInfo) mode = "INFO";
  else if (type == DLSWarning) mode = "WARNING";

  syslog(LOG_INFO, "%s: %s", mode.c_str(), _msg.str().c_str());

  // Nachricht entfernen
  _msg.str("");
}

//---------------------------------------------------------------
