//---------------------------------------------------------------
//
//  D L S _ P R O C _ L O G G E R . C P P
//
//---------------------------------------------------------------

#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>

#include <fstream>
using namespace std;

//---------------------------------------------------------------

#include "com_ring_buffer_t.hpp"
#include "dls_globals.hpp"
#include "dls_proc_logger.hpp"

//#define DEBUG
//#define DEBUG_SIZES
//#define DEBUG_SEND
//#define DEBUG_REC

//---------------------------------------------------------------

RCS_ID("$Header: /home/fp/dls/src/RCS/dls_proc_logger.cpp,v 1.23 2005/02/23 17:06:50 fp Exp $");

//---------------------------------------------------------------

/**
   Konstruktor 

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

DLSProcLogger::DLSProcLogger(const string &dls_dir, unsigned int job_id)
{
  _dls_dir = dls_dir;
  _job_id = job_id;
  _sig_hangup = sig_hangup;
  _sig_child = sig_child;
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
    _job = new DLSJob(this, _dls_dir);
  }
  catch (...)
  {
    _job = 0;
    msg() << "Could not allocate memory for job!";
    log(DLSError);
  }

  try
  {
    _ring_buf = new COMRingBufferT<char, unsigned int>(RECEIVE_RING_BUF_SIZE);
  }
  catch (...)
  {
    _ring_buf = 0;
    msg() << "Could not allocate memory for ring buffer.";
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
      msg() << "Deleting job: " << e.msg;
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
  msg() << "Process started for job " << job_id << "!";
  log(DLSInfo);

  _create_pid_file();

  if (!_exit)
  {
    // Ablauf starten
    _start();
  
    if (process_type == dlsLoggingProcess)
    {
      // PID-Datei wieder entfernen
      _remove_pid_file();
    }
  }
    
  if (_exit_code == E_DLS_NO_ERROR)
  {
    msg() << "----- Logging process finished. Exiting gracefully. -----";
  }
  else
  {
    msg() << "----- Logging process finished. Exiting with ERROR! (Code " << _exit_code << ") -----";
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
    _job->import(_job_id);
  }
  catch (EDLSJob &e)
  {
    _exit_code = E_DLS_ERROR;

    msg() << "Importing: " << e.msg;
    log(DLSError);

    return;
  }

  // Meldungen über Quota-Benutzung ausgeben

  if (_job->preset()->quota_time())
  {
    msg() << "Using time quota of " << _job->preset()->quota_time() << " seconds";
    log(DLSInfo);
  }
  
  if (_job->preset()->quota_size())
  {
    msg() << "Using size quota of " << _job->preset()->quota_size() << " bytes";
    log(DLSInfo);
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

  msg() << "Connection to " << _job->preset()->source() << " closed.";
  log(DLSInfo);

  try
  {
    _job->finish();
  }
  catch (EDLSJob &e)
  {
    _exit_code = E_DLS_ERROR;

    msg() << "Finishing: " << e.msg;
    log(DLSError);
  }

#ifdef DEBUG_SIZES
  msg() << "Wrote " << _job->data_size() << " bytes of data.";
  log(DLSInfo);
#endif
}

//---------------------------------------------------------------

/**
   Verbindung zur Datenquelle aufbauen

   \return true, wenn die Verbindung aufgebaut werden konnte
*/

bool DLSProcLogger::_connect_socket()
{
  struct sockaddr_in address;
  struct hostent *hp;
  const char *source = _job->preset()->source().c_str();

  // Socket öffnen
  if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    msg() << "Could not create socket!"; 
    log(DLSError);

    return false;
  }

  // Socket geöffnet, Adresse übersetzen/auflösen
  address.sin_family = AF_INET;
  if ((hp = gethostbyname(source)) == NULL)
  {
    close(_socket);

    msg() << "Could not resolve \"" << source << "\"!";
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

    msg() << "Could not connect to \"" << source << "\"!";
    log(DLSError);
    
    return false;
  }

  // Verbunden!
  msg() << "Connected to \"" << source << "\"!";
  log(DLSInfo);

  return true;
}

//---------------------------------------------------------------

/**
   Führt alle Lese- und Schreiboperationen durch
*/

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

          msg() << "FATAL: Ring buffer full!";
          log(DLSError);

          break;
        }

	// Daten abholen...
        if ((recv_ret = recv(_socket, write_addr, write_size, 0)) > 0)
        {
#ifdef DEBUG_REC
          msg() << "REC: \"" << string(write_addr, recv_ret) << "\""; 
          log(DLSInfo);
#endif

          // Dem Ring-Puffer mitteilen, dass Daten geschrieben wurden
          _ring_buf->written(recv_ret);

          // Daten im Ring-Pugger parsen...
          _parse_ring_buffer();

          // Wenn ein Fehler passiert ist, jetzt ausbrechen!
          if (_exit) break;
        }
        else if (recv_ret == 0) // Verbindung geschlossen!
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          msg() << "Connection closed by server."; 
          log(DLSError);

          break;
        }
        else // Fehler
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          msg() << "Error " << errno << " in recv()!";
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
#ifdef DEBUG_SEND
          msg() << "SENT: \"" << _to_send.substr(0, send_ret) << "\"";
          log(DLSInfo);
#endif

          _to_send.erase(0, send_ret); // Gesendetes entfernen
        }
        else if (send_ret == -1)
        {
          _exit = true;
          _exit_code = E_DLS_ERROR;

          msg() << "Error " << errno << " in send()!";
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
    else if (select_ret == -1)
    {
      if (errno == EINTR)
      {
        msg() << "select() interrupted by signal.";
        log(DLSInfo);
      }
      else
      {
        _exit = true;
        _exit_code = E_DLS_ERROR;
        
        msg() << "Error " << errno << " in select()!";
        log(DLSError);
        
        break;
      }
    }

    // Auf gesetzte Signale überprüfen
    _check_signals();

    if (_exit) break;

    // Trigger
    _do_trigger();

    // Watchdog
    _do_watchdogs();

    // Warnung ausgeben, wenn zu lange keine Daten mehr empfangen
    if (COMTime::now() - _last_data_received > (long long) NO_DATA_WARNING * 1000000)
    {
      if (!_no_data_warning)
      {
        if (_state != dls_waiting_for_trigger)
        {
          msg() << "No data received for a long time!";
          log(DLSWarning);
        }
        _no_data_warning = true;
        _first_data_time.set_null();
      }
    }
    else if (_no_data_warning)
    {
      msg() << "Receiving data!";
      log(DLSInfo);
      _no_data_warning = false;
    }

    // Quota
    _do_quota();
 
    // Soll der Prozess beendet werden?
    if (_exit) break;
  }
}

//---------------------------------------------------------------

/**
   Merkt ein Kommando zum Senden an die Datenquelle vor

   \param cmd Kommando-String
*/

void DLSProcLogger::send_command(const string &cmd)
{
  _to_send += cmd + "\n";
}

//---------------------------------------------------------------

/**
   Überprüft, ob Signale anstehen
*/

void DLSProcLogger::_check_signals()
{
  int status;
  pid_t pid;
  int exit_code;

  if (sig_int_term)
  {
    _exit = true;

    msg() << "SIGINT or SIGTERM received in state " << _state << "!";
    log(DLSInfo);   

    return;
  }

  // Nachricht von Elternprozess: Auftrag hat sich geändert!
  if (sig_hangup != _sig_hangup)
  {
    _sig_hangup = sig_hangup;

    msg() << "Received notification from mother process.";
    log(DLSInfo);
    
    try
    {
      _job->import(_job_id);
    }
    catch (EDLSJob &e)
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;

      msg() << "Importing job: " << e.msg;
      log(DLSError);

      return;
    }

    if (!_job->preset()->running()) // Erfassung gestoppt
    {
      _exit = true;

      msg() << "Job is no longer running."; 
      log(DLSInfo);
    }

    else // Erfassung soll weiterlaufen
    {
      if (_state == dls_waiting_for_trigger)
      {
        _state = dls_listening;

        msg() << "No trigger any more! Start logging.";
        log(DLSInfo);

        _job->start_logging();
      }
      else
      {
        _job->change_logging();
      }
    }
  }

  // Flush-Prozess hat sich beendet
  while (sig_child != _sig_child)
  {
    _sig_child++;
    
    pid = wait(&status); // Zombie töten!
    exit_code = (signed char) WEXITSTATUS(status);

    msg() << "Cleanup process exited with code " << exit_code;
    log(DLSInfo);
  }
}

//---------------------------------------------------------------

/**
   Parst die gelesenen Daten nach XML-Tags
*/

void DLSProcLogger::_parse_ring_buffer()
{
  while (1)
  {
    try
    {
      // Das nächste XML-Tag im Ringpuffer parsen
      _xml.parse(_ring_buf);
    }
    catch (ECOMXMLParserEOF &e) // Tag noch nicht komplett
    {
      // Weiter Daten empfangen
      return;
    }
    catch (ECOMXMLParser &e) // Anderer Parsing-Fehler
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;
      
      msg() << "Parsing incoming data: " << e.msg << " Tag: " << e.tag;
      log(DLSError);

      // Prozess beenden!
      return;
    }

    // Tag komplett; verarbeiten!
    _process_tag();

    // Nach der Verarbeitung jedes Tags kann ein Fehler aufgetreten
    // sein. Deshalb nach jedem Tag prüfen...
    if (_exit) break;
  }
}

//---------------------------------------------------------------

/**
   Verarbeitet ein vollständig geparstes XML-Tag
*/

void DLSProcLogger::_process_tag()
{
  string send_str, title;
  COMRealChannel real_channel;
  unsigned int new_buffer_level;

  title = _xml.tag()->title();

  try
  {
    if (title == "info" || title == "warn"
        || title == "error" || title == "crit_error"|| title == "broadcast") 
    {
      msg() << "MSRD: " << _xml.tag()->tag();
      log(DLSInfo);
     
      // Message- und Message-Index-Datei aktualisieren
      _job->message(_xml.tag());
      
      return;
    }

    // Acknoledgement-Tags direkt an _job weiterleiten
    else if (_xml.tag()->title() == "ack")
    {
      _job->ack_received(_xml.tag()->att("id")->to_str());
      return;
    }

    // Trigger-Parameter verarbeiten
    else if (_xml.tag()->title() == "parameter"                                 // Parameter empfangen
             && _job->preset()->trigger() != ""                                      // Triggern aktiviert
             && _xml.tag()->att("name")->to_str() == _job->preset()->trigger()) // Trigger-Parameter
    {
      if (_state == dls_waiting_for_trigger
          && _xml.tag()->att("value")->to_int() != 0)
      {
        _state = dls_listening; // Zustandswechsel!
            
        msg() << "Trigger active! Start logging.";
        log(DLSInfo);
            
        _no_data_warning = true; // Gleich augeben, dass wieder Daten kommen
            
        _job->start_logging();
      }
        
      else if (_state == dls_listening || _state == dls_getting_data)
      {
        if (_xml.tag()->att("value")->to_int() == 0)
        {
          _state = dls_waiting_for_trigger;
              
          msg() << "Trigger not active! Stop logging.";
          log(DLSInfo);

          _job->stop_logging();

          msg() << "Waiting for trigger...";
          log(DLSInfo);
        }
      }
            
      return;
    }

    switch (_state) // Ab jetzt Zustandsabhängigkeit!
    {
      case dls_connecting: //------------------------------------

        // Nur <connected> annehmen
        if (_xml.tag()->title() == "connected")
        {
          // Nur mit MSR sprechen
          if (_xml.tag()->att("name")->to_str() != "MSR")
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;
            msg() << "Expected name: MSR!";
            log(DLSError);
            break;
          }

          // Version auslesen
          _msr_version = _xml.tag()->att("version")->to_int();
          
          if (_msr_version < MSR_VERSION(2, 7, 0))
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;
            msg() << "expecting version > 2.7.0! actual version:";
            msg() << " " << MSR_V(_msr_version);
            msg() << "." << MSR_P(_msr_version);
            msg() << "." << MSR_S(_msr_version) << "...";
            log(DLSError);
            break;
          }

          // Eendianess bestimmen
          if (_xml.tag()->has_att("arch"))
          {
            if (_xml.tag()->att("arch")->to_str() == "little")
            {
              source_arch = LittleEndian;
              msg() << "Source architecture: Little-endian.";
              log(DLSInfo);
            }
            else if (_xml.tag()->att("arch")->to_str() == "big")
            {
              source_arch = BigEndian;
              msg() << "Source architecture: Big-endian.";
              log(DLSInfo);
            }
            else
            {
              _exit = true;
              _exit_code = E_DLS_ERROR;
              msg() << "Unknown architecture: " << _xml.tag()->att("arch")->to_str();
              log(DLSError);
              break;
            }
          }
          else
          {
            source_arch = LittleEndian;
            msg() << "No architecture information! Assuming little-endian.";
            log(DLSWarning);
          }
          
          // Zustandswechsel!
          _state = dls_waiting_for_frequency;

          // Maximale Abtastfrequenz des Systems auslesen
          send_command("<rp name=\"/Taskinfo/Abtastfrequenz\">");
        }
        break;

      case dls_waiting_for_frequency: //-------------------------

        if (_xml.tag()->title() == "parameter")
        {
          if (_xml.tag()->att("name")->to_str() != "/Taskinfo/Abtastfrequenz")
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            msg() << "expected /Taskinfo/Abtastfrequenz!";
            log(DLSError);
          }
          else
          {
            _max_frequency = _xml.tag()->att("value")->to_int();
      
            _state = dls_waiting_for_channels; // Zustandswechsel!

            msg() << "connected to MSR version";
            msg() << " " << MSR_V(_msr_version);
            msg() << "." << MSR_P(_msr_version);
            msg() << "." << MSR_S(_msr_version);
            msg() << ", " << _max_frequency << " Hz";
            log(DLSInfo);

            // Alle Kanäle auslesen
            send_command("<rk>");
          }
        }
        break;

      case dls_waiting_for_channels: //--------------------------

        if (_xml.tag()->title() == "channels"
            && _xml.tag()->type() == dxttBegin)
        {
          _state = dls_getting_channels; // Zustandswechsel
        }
        break;

      case dls_getting_channels: //------------------------------

        if (_xml.tag()->title() == "channel")
        {
          try
          {
            real_channel.name = _xml.tag()->att("name")->to_str();
            real_channel.unit = _xml.tag()->att("unit")->to_str();
            real_channel.index = _xml.tag()->att("index")->to_int();
            real_channel.frequency = _xml.tag()->att("HZ")->to_int();
            real_channel.bufsize = _xml.tag()->att("bufsize")->to_int();
            real_channel.type = dls_str_to_channel_type(_xml.tag()->att("typ")->to_str());
            _real_channels.push_back(real_channel);
          }
          catch (ECOMXMLTag &e)
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            msg() << "receiving MSR channel: " << e.msg << " tag: " << e.tag;
            log(DLSError);
          }
          catch (COMException &e) // Channel-Typ unbekannt
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            msg() << "receiving MSR channel: unknown channel type \"";
            msg() << "\"" << _xml.tag()->att("typ")->to_str() << "\"";
            log(DLSError);
          }
        }
        else if (_xml.tag()->title() == "channels"
                 && _xml.tag()->type() == dxttEnd)
        {
          _got_channels = true;

          if (_job->preset()->trigger() == "")
          {
            // Alle Logger starten
            _job->start_logging();
            _state = dls_listening; // Zustandswechsel!

            msg() << "start logging.";
          }
          else
          {
            _state = dls_waiting_for_trigger; // Zustandswechsel!

            msg() << "waiting for trigger \"";
            msg() << _job->preset()->trigger() << "\"...";
          }

          log(DLSInfo);
        }
        break;

      case dls_listening: //-------------------------------------

        if (_xml.tag()->title() == "data")
        {
          _state = dls_getting_data; // Zustandswechsel!

          _last_data_received.set_now();

          _data_time.from_dbl_time(_xml.tag()->att("time")->to_dbl());

          // zeit des ersten Datenempfanges vermerken
          if (_first_data_time.is_null()) _first_data_time = _data_time;

          if (_xml.tag()->has_att("level"))
          {
            // Meldungen über Füllstand der Kanalpuffer auswerten
            new_buffer_level = _xml.tag()->att("level")->to_int();
            if (_buffer_level < BUFFER_LEVEL_WARNING
                && new_buffer_level >= BUFFER_LEVEL_WARNING)
            {
              // Warnung: Füllstand zu hoch!
              msg() << "channel buffers nearly full!";
              log(DLSWarning);
            }
            else if (_buffer_level >= BUFFER_LEVEL_WARNING
                     && new_buffer_level < BUFFER_LEVEL_WARNING)
            {
              // Entwarnung geben
              msg() << "level of channel buffers decreasing...";
              log(DLSInfo);
            }

            _buffer_level = new_buffer_level;
          }
        }
        break;

      case dls_getting_data: //----------------------------------

        if (_xml.tag()->title() == "data"
            && _xml.tag()->type() == dxttEnd)
        {
          _state = dls_listening; // Zustandswechsel!
        }
        else if (_xml.tag()->title() == "F")
        {
          try
          {
            _job->process_data(_data_time,
                               _xml.tag()->att("c")->to_int(),
                               _xml.tag()->att("d")->to_str());
          }
          catch (EDLSTimeTolerance &e)
          {
            _exit = true;
            _exit_code = E_DLS_TIME_TOLERANCE;

            msg() << "TIME TOLERANCE EXCEEDED: " << e.msg;
            log(DLSError);
          }
          catch (EDLSJob &e)
          {
            _exit = true;
            _exit_code = E_DLS_ERROR;

            msg() << "processing data: " << e.msg;
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

    msg() << "processing tag: " << e.msg << " tag: " << e.tag;
    log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   Veranlasst das Senden des Trigger-Parameters
*/

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

/**
   Ändert die Watchdog-Dateien
*/

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

/**
   Erstellt die PID-Datei
*/

void DLSProcLogger::_create_pid_file()
{
  stringstream pid_file_name;
  fstream new_pid_file, old_pid_file;
  pid_t pid;
  struct stat stat_buf;

  pid_file_name << _dls_dir << "/job" << job_id << "/" << DLS_PID_FILE;

  if (stat(pid_file_name.str().c_str(), &stat_buf) == -1)
  {
    if (errno != ENOENT)
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;

      msg() << "Could not stat() file \"" << pid_file_name.str();
      msg() << "\": " << strerror(errno);
      log(DLSError);

      return;
    }
  }
  else // PID-Datei existiert bereits!
  {
    old_pid_file.exceptions(ios::badbit | ios::failbit);

    old_pid_file.open(pid_file_name.str().c_str(), ios::in);

    if (!old_pid_file.is_open())
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;

      msg() << "Could not open existing PID file \"" << pid_file_name.str() << "\"!";
      log(DLSError);

      return;
    }

    try
    {
      old_pid_file >> pid;
    }
    catch (...)
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;

      old_pid_file.close();

      msg() << "Exsting PID file \"" << pid_file_name.str() << "\" is corrupt!";
      log(DLSError);

      return;
    }

    old_pid_file.close();

    if (kill(pid, 0) == -1)
    {
      if (errno != ESRCH)
      {
        _exit = true;
        _exit_code = E_DLS_ERROR;

        msg() << "Could not signal process " << pid << "!";
        log(DLSError);

        return;
      }
    }
    else // Prozess läuft noch!
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;
        
      msg() << "Another logging process ist running on job " << job_id << "!";
      log(DLSError);

      return;
    }

    // Existierende PID-Datei löschen
    _remove_pid_file();

    if (_exit) return;
  }

  new_pid_file.open(pid_file_name.str().c_str(), ios::out);

  if (!new_pid_file.is_open())
  {
    _exit = true;
    _exit_code = E_DLS_ERROR;
        
    msg() << "Could not create PID file \"" << pid_file_name.str() << "\"!";
    log(DLSError);

    return;
  }

  new_pid_file << getpid() << endl;
  new_pid_file.close();
}

//---------------------------------------------------------------

/**
   Entfernt die PID-Datei
*/

void DLSProcLogger::_remove_pid_file()
{
  stringstream pid_file_name;

  pid_file_name << _dls_dir << "/job" << job_id << "/" << DLS_PID_FILE;

  if (unlink(pid_file_name.str().c_str()) == -1)
  {
    _exit = true;
    _exit_code = E_DLS_ERROR;
    
    msg() << "Could not remove PID file \"" << pid_file_name << "\"!";
    log(DLSError);
  }
}

//---------------------------------------------------------------

/**
   Prüft, ob die Quota überschritten wurde

   Wenn ja, wird ein Kindprozess abgeforkt, der für die Flush-
   Operationen zuständig ist. Der Elternprozess vergisst alle
   bisherigen Daten und empfängt neue Daten von der Quelle.
*/

void DLSProcLogger::_do_quota()
{
  int fork_ret;
  long long quota_time = _job->preset()->quota_time();
  long long quota_size = _job->preset()->quota_size();
  bool quota_reached = false;
  COMTime quota_time_limit;

  if (quota_time && !_first_data_time.is_null())
  {
    quota_time_limit = _first_data_time + (long long) (quota_time * 1000000 / QUOTA_PART_QUOTIENT);

    if (_data_time >= quota_time_limit)
    {
      quota_reached = true;

      msg() << "time quota (1/" << QUOTA_PART_QUOTIENT << " of " << quota_time << " seconds) reached.";
      log(DLSInfo);
    }
  }

  if (quota_size)
  {
    if (_job->data_size() >= quota_size / QUOTA_PART_QUOTIENT)
    {
      quota_reached = true;

      msg() << "size quota (1/" << QUOTA_PART_QUOTIENT << " of " << quota_size << " bytes) reached.";
      log(DLSInfo);
    }
  }

  if (quota_reached)
  {
    _first_data_time.set_null();
    
    if ((fork_ret = fork()) == -1)
    {
      _exit = true;
      _exit_code = E_DLS_ERROR;
      
      msg() << "could not fork!";
      log(DLSError);
      
      return;
    }

    if (fork_ret == 0) // "Kind"
    {
      // Wir sind jetzt der Aufräum-Prozess
      process_type = dlsCleanupProcess;

      // Normal beenden und Daten speichern
      _exit = true;
      
      msg() << "flushing process forked.";
      log(DLSInfo);
    }
    else
    {
      // Alle Daten vergessen. Diese werden vom anderen
      // Zweig gespeichert.
      _job->discard_data();
    }
  }
}

//---------------------------------------------------------------
