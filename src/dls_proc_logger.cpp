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

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <errno.h>
#include <pwd.h>

#include <fstream>
using namespace std;

/*****************************************************************************/

#include <pdcom/Variable.h>

/*****************************************************************************/

#include "dls_globals.hpp"
#include "dls_proc_logger.hpp"
#include "dls_saver_t.hpp"

//#define DEBUG_CONNECT
//#define DEBUG_SIZES
//#define DEBUG_SEND
//#define DEBUG_REC

/*****************************************************************************/

/**
   Konstruktor

   \param dls_dir DLS-Datenverzeichnis
   \param job_id Auftrags-ID
*/

DLSProcLogger::DLSProcLogger(
		const string &dls_dir,
		unsigned int job_id
		):
	Process(),
	_dls_dir(dls_dir),
	_job_id(job_id),
	_job(this, _dls_dir),
    _socket(-1),
	_write_request(false),
    _sig_hangup(sig_hangup),
    _sig_child(sig_child),
    _exit(false),
    _exit_code(E_DLS_SUCCESS),
	_state(Connecting),
    _receiving_data(false),
    _trigger(NULL)
{
	readOnly = true; // from PdCom::Process: disable writing

    openlog("dlsd_logger", LOG_PID, LOG_DAEMON);
}

/*****************************************************************************/

DLSProcLogger::~DLSProcLogger()
{
    closelog();
}

/*****************************************************************************/

/**
   Starten des Logging-Prozesses

   \return Exit-Code
*/

int DLSProcLogger::start()
{
    msg() << "Process started for job " << dlsd_job_id << "!";
    log(DLSInfo);

    _create_pid_file();

    if (!_exit) {

        // Ablauf starten
        _start();

        if (process_type == dlsLoggingProcess) {
            // PID-Datei wieder entfernen
            _remove_pid_file();
        }
    }

    if (_exit_code == E_DLS_SUCCESS) {
        msg() << "----- Logging process finished. Exiting gracefully. -----";
    }
    else {
        msg() << "----- Logging process finished."
              << " Exiting with ERROR! (Code " << _exit_code << ") -----";
    }

    log(DLSInfo);
    return _exit_code;
}

/*****************************************************************************/

/** Notify the process about an error and mark it for exiting.
 */
void DLSProcLogger::notify_error(int code)
{
    _exit = true;
    _exit_code = code;
}

/*****************************************************************************/

/** Notify the process about received data.
 */
void DLSProcLogger::notify_data()
{
    _last_receive_time.set_now();

    if (!_receiving_data) {
        _receiving_data = true;

        msg() << "Receiving data.";
        log(DLSInfo);
    }

    if (_quota_start_time.is_null()) {
        _quota_start_time = _last_receive_time;
    }
}

/*****************************************************************************/

/**
   Starten des Logging-Prozesses (intern)
*/

void DLSProcLogger::_start()
{
    try {
        // Auftragsdaten importieren
        _job.import(_job_id);
    }
    catch (EDLSJob &e) {
        _exit_code = E_DLS_ERROR; // no restart, invalid configuration
        msg() << "Importing: " << e.msg;
        log(DLSError);
        return;
    }

    // Meldungen �ber Quota-Benutzung ausgeben

    if (_job.preset()->quota_time()) {
        msg() << "Using time quota of " << _job.preset()->quota_time()
              << " seconds";
        log(DLSInfo);
    }

    if (_job.preset()->quota_size()) {
        msg() << "Using size quota of " << _job.preset()->quota_size()
              << " bytes";
        log(DLSInfo);
    }

    // Mit Pr�fstand verbinden
    if (!_connect_socket()) {
        _exit_code = E_DLS_ERROR_RESTART;
        return;
    }

    // Kommunikation starten
    _read_write_socket();

    // Verbindung zu MSR schliessen
    close(_socket);
	_socket = -1;

    msg() << "Connection to " << _job.preset()->source() << " closed.";
    log(DLSInfo);

	reset(); // PdCom::Process

    try {
        _job.finish();
    }
    catch (EDLSJob &e) {
        _exit_code = E_DLS_ERROR_RESTART;
        msg() << "Finishing: " << e.msg;
        log(DLSError);
    }

#ifdef DEBUG_SIZES
    msg() << "Wrote " << _job.data_size() << " bytes of data.";
    log(DLSInfo);
#endif
}

/*****************************************************************************/

/**
   Verbindung zur Datenquelle aufbauen

   \return true, wenn die Verbindung aufgebaut werden konnte
*/

bool DLSProcLogger::_connect_socket()
{
    const char *host = _job.preset()->source().c_str();

	stringstream service;
	service << _job.preset()->port();

	struct addrinfo hints = {};
	hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_protocol = 0; // any protocol
	hints.ai_flags =
		AI_ADDRCONFIG & // return address types if local interface exists
		AI_NUMERICSERV; // service always numeric

	struct addrinfo *result = NULL;

	int ret = getaddrinfo(host, service.str().c_str(), &hints, &result);

	if (ret) {
        msg() << "Could not resolve \"" << host << ":" << service.str()
			<< "\": " << gai_strerror(ret);
        log(DLSError);
        return false;
    }

	struct addrinfo *rp;

	for (rp = result; rp != NULL; rp = rp->ai_next) {
#ifdef DEBUG_CONNECT
        msg() << "Trying socket(family=" << rp->ai_family
			<< ", type=" << rp->ai_socktype
			<< ", protocol=" << rp->ai_protocol << ")...";
		log(DLSInfo);
#endif

		_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (_socket == -1) {
#ifdef DEBUG_CONNECT
			msg() << "Failed: " << strerror(errno);
			log(DLSError);
#endif
			continue;
		}

#ifdef DEBUG_CONNECT
        msg() << "Trying connect(addr=" << rp->ai_addr
			<< ", len=" << rp->ai_addrlen;
		log(DLSInfo);
#endif

		if (::connect(_socket, rp->ai_addr, rp->ai_addrlen) != -1) {
			break; // success
		}

		close(_socket);
		_socket = -1;

#ifdef DEBUG_CONNECT
        msg() << "Could not connect: " << strerror(errno) << endl;
        log(DLSError);
#endif
	}

	freeaddrinfo(result);

	if (!rp) {
        msg() << "Failed to connect: " << strerror(errno) << endl;
        log(DLSError);

        return false;
	}

    msg() << "Connected to \"" << host << ":" << service.str() << "\"" << ".";
    log(DLSInfo);

    int optval = 1;
    ret = setsockopt(_socket, SOL_SOCKET, SO_KEEPALIVE, &optval,
            sizeof(optval));
    if (ret == -1) {
        msg() << "Failed to set keepalive socket option: "
            << strerror(errno);
        log(DLSWarning);
    }

    return true;
}

/*****************************************************************************/

/**
   F�hrt alle Lese- und Schreiboperationen durch
*/

void DLSProcLogger::_read_write_socket()
{
    fd_set read_fds, write_fds;
    int select_ret;
    struct timeval timeout;

    while (!_exit) {

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(_socket, &read_fds);

        if (_write_request) {
			FD_SET(_socket, &write_fds);
		}

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Warten auf �nderungen oder Timeout
        select_ret = select(_socket + 1, &read_fds, &write_fds, 0, &timeout);

        if (select_ret > 0) {
            if (FD_ISSET(_socket, &read_fds)) { // incoming data
                _read_socket();
                if (_exit) {
                    break;
				}
            }

            if (FD_ISSET(_socket, &write_fds)) {
				int ret = writeReady();
				if (ret < 0) {
                    _exit = true;
                    _exit_code = E_DLS_ERROR_RESTART;
                    msg() << "Sending data failed: " << strerror(errno);
                    log(DLSError);
				} else if (!ret) {
					// No more data to send
					_write_request = false;
				}
            }
        }
        else if (select_ret == -1 && errno != EINTR) {
			_exit = true;
			_exit_code = E_DLS_ERROR_RESTART;
			msg() << "Error " << errno << " in select()!";
			log(DLSError);
			break;
        }

        // Auf gesetzte Signale �berpr�fen
        _check_signals();

        if (_exit) {
			break;
		}

        // Watchdog
        _do_watchdogs();

        // Warnung ausgeben, wenn zu lange keine Daten mehr empfangen
        if (_state == Data &&
                ((COMTime::now() - _last_receive_time).to_dbl_time() >
                 NO_DATA_ABORT_TIME)) {
            _exit = true;
            _exit_code = E_DLS_ERROR_RESTART;

            msg() << "No data received for " << NO_DATA_ABORT_TIME
                << " s! Seems that the server is down. Restarting...";
            log(DLSError);
        }

        // Quota
        _do_quota();
    }
}

/*****************************************************************************/

/**
 * Reads new xml data into the ring buffer.
 */

void DLSProcLogger::_read_socket()
{
    char buf[4096];

    int ret = ::read(_socket, buf, sizeof(buf));

    if (ret > 0) {
#ifdef DEBUG_REC
		cerr << "read: " << string(buf, ret) << endl;
#endif
        try {
            newData(buf, ret);
        }
		catch (PdCom::Exception &e) {
			_exit = true;
			_exit_code = E_DLS_ERROR_RESTART;

            msg() << "newData() failed: " << e.what()
				<< ", last data: " << string(buf, ret);
        }

    } else if (ret < 0) {
        if (errno != EINTR) {
			_exit = true;
			_exit_code = E_DLS_ERROR_RESTART;
			msg() << "Error in recv(): " << strerror(errno);
			log(DLSError);
        }
    } else { // ret == 0
        _exit = true;
        _exit_code = E_DLS_ERROR_RESTART;
        msg() << "Connection closed by server.";
        log(DLSError);
    }
}

/****************************************************************************/

void DLSProcLogger::_subscribe_trigger()
{
    if (_trigger) {
        _trigger->unsubscribe(this);
        _trigger = NULL;
    }

    PdCom::Variable *pv = findVariable(_job.preset()->trigger());

    if (!pv) {
        msg() << "Trigger variable \"" << _job.preset()->trigger()
            << "\" does not exist!";
        log(DLSError);
        return;
    }

    try {
        pv->subscribe(this, 0.0); // event-based
    }
    catch (PdCom::Exception &e) {
        msg() << "Trigger subscription failed: " << e.what();
        log(DLSError);
        return;
    }

    _trigger = pv;
}

/*****************************************************************************/

/**
   �berpr�ft, ob Signale anstehen
*/

void DLSProcLogger::_check_signals()
{
    int status;
    int exit_code;

    if (sig_int_term) {
        _exit = true;
        msg() << "SIGINT or SIGTERM received in state " << _state << "!";
        log(DLSInfo);
        return;
    }

    // Nachricht von Elternprozess: Auftrag hat sich ge�ndert!
    if (sig_hangup != _sig_hangup) {
        _sig_hangup = sig_hangup;

        msg() << "Received notification from mother process.";
        log(DLSInfo);

        _reload();
    }

    // Flush-Prozess hat sich beendet
    while (sig_child != _sig_child) {
        _sig_child++;
        wait(&status); // Zombie t�ten!
        exit_code = (signed char) WEXITSTATUS(status);
        msg() << "Cleanup process exited with code " << exit_code;
        log(DLSInfo);
    }
}

/*****************************************************************************/

/** Reloads the job presettings.
 */
void DLSProcLogger::_reload()
{
    try {
        _job.import(_job_id);
    }
    catch (EDLSJob &e) {
        _exit = true;
        _exit_code = E_DLS_ERROR;
        msg() << "Importing job: " << e.msg;
        log(DLSError);
        return;
    }

    if (!_job.preset()->running()) { // Erfassung gestoppt
        _exit = true;
        msg() << "Job is no longer running.";
        log(DLSInfo);
        return;
    }

    // continue running

    if (_job.preset()->trigger() != "") { // triggered
        if (!_trigger || _trigger->path != _job.preset()->trigger()) {
            // no trigger yet or trigger changed
            _subscribe_trigger();
        }
        if (_state == Data) {
            _job.change_logging();
        }
    }
    else { // not triggered
        if (_state == Waiting) {
            _state = Data;
            _last_receive_time.set_now();
            _receiving_data = false;

            msg() << "No trigger any more! Start logging.";
            log(DLSInfo);

            _job.start_logging();
        } else {
            _job.change_logging();
        }
    }
}

/*****************************************************************************/

/**
   �ndert die Watchdog-Dateien
*/

void DLSProcLogger::_do_watchdogs()
{
    if ((COMTime::now() - _last_watchdog_time).to_dbl_time() <
            WATCHDOG_INTERVAL) {
        return;
    }

    _last_watchdog_time.set_now();

    stringstream dir_name;
    dir_name << _dls_dir << "/job" << _job.preset()->id();

    fstream watchdog_file;
    watchdog_file.open((dir_name.str() + "/watchdog").c_str(), ios::out);
    watchdog_file.close();

    if (_state == Data && _receiving_data) {
        fstream logging_file;
        logging_file.open((dir_name.str() + "/logging").c_str(), ios::out);
        logging_file.close();
    }
}

/*****************************************************************************/

/**
   Pr�ft, ob die Quota �berschritten wurde

   Wenn ja, wird ein Kindprozess abgeforkt, der f�r die Flush-
   Operationen zust�ndig ist. Der Elternprozess vergisst alle
   bisherigen Daten und empf�ngt neue Daten von der Quelle.
*/

void DLSProcLogger::_do_quota()
{
    int fork_ret;
    uint64_t quota_time = _job.preset()->quota_time();
    uint64_t quota_size = _job.preset()->quota_size();
    bool quota_reached = false;
    COMTime quota_time_limit;

    if (quota_time && !_quota_start_time.is_null()) {
        quota_time_limit = _quota_start_time
            + (uint64_t) (quota_time * 1000000 / QUOTA_PART_QUOTIENT);

        if (_last_receive_time >= quota_time_limit) {
            quota_reached = true;
            msg() << "Time quota (1/" << QUOTA_PART_QUOTIENT
                  << " of " << quota_time << " seconds) reached.";
            log(DLSInfo);
        }
    }

    if (quota_size) {
        if (_job.data_size() >= quota_size / QUOTA_PART_QUOTIENT) {
            quota_reached = true;
            msg() << "Size quota (1/" << QUOTA_PART_QUOTIENT
                  << " of " << quota_size << " bytes) reached.";
            log(DLSInfo);
        }
    }

    if (quota_reached) {
        if ((fork_ret = fork()) == -1) {
            _exit = true;
            _exit_code = E_DLS_ERROR_RESTART;
            msg() << "Failed to fork()!";
            log(DLSError);
            return;
        }

        if (fork_ret == 0) { // "Kind"
            // Wir sind jetzt der Aufr�um-Prozess
            process_type = dlsCleanupProcess;
            // Normal beenden und Daten speichern
            _exit = true;
            msg() << "Flushing process forked.";
            log(DLSInfo);
        }
        else {
            // Alle Daten vergessen. Diese werden vom anderen
            // Zweig gespeichert.
            _job.discard();
            _quota_start_time.set_null();
        }
    }
}

/*****************************************************************************/

/**
   Erstellt die PID-Datei
*/

void DLSProcLogger::_create_pid_file()
{
    stringstream pid_file_name;
    fstream new_pid_file, old_pid_file;
    pid_t pid;
    struct stat stat_buf;

    pid_file_name << _dls_dir << "/job" << dlsd_job_id << "/" << DLS_PID_FILE;

    if (stat(pid_file_name.str().c_str(), &stat_buf) == -1) {
        if (errno != ENOENT) {
            _exit = true;
            _exit_code = E_DLS_ERROR;
            msg() << "Could not stat() file \"" << pid_file_name.str();
            msg() << "\": " << strerror(errno);
            log(DLSError);
            return;
        }
    }
    else { // PID-Datei existiert bereits!
        old_pid_file.exceptions(ios::badbit | ios::failbit);
        old_pid_file.open(pid_file_name.str().c_str(), ios::in);
        if (!old_pid_file.is_open()) {
            _exit = true;
            _exit_code = E_DLS_ERROR;
            msg() << "Could not open existing PID file \""
                  << pid_file_name.str() << "\"!";
            log(DLSError);
            return;
        }

        try {
            old_pid_file >> pid;
        }
        catch (...) {
            _exit = true;
            _exit_code = E_DLS_ERROR;
            old_pid_file.close();
            msg() << "Existing PID file \"" << pid_file_name.str()
                  << "\" is corrupt!";
            log(DLSError);
            return;
        }

        old_pid_file.close();

        if (kill(pid, 0) == -1) {
            if (errno != ESRCH) {
                _exit = true;
                _exit_code = E_DLS_ERROR;
                msg() << "Could not signal process " << pid << "!";
                log(DLSError);
                return;
            }
        }
        else { // Prozess l�uft noch!
            _exit = true;
            _exit_code = E_DLS_ERROR;
            msg() << "Another logging process ist running on job "
                  << dlsd_job_id << "!";
            log(DLSError);
            return;
        }

        // Existierende PID-Datei l�schen
        _remove_pid_file();

        if (_exit) return;
    }

    new_pid_file.open(pid_file_name.str().c_str(), ios::out);

    if (!new_pid_file.is_open()) {
        _exit = true;
        _exit_code = E_DLS_ERROR;
        msg() << "Could not create PID file \"" << pid_file_name.str()
              << "\"!";
        log(DLSError);
        return;
    }

    new_pid_file << getpid() << endl;
    new_pid_file.close();
}

/*****************************************************************************/

/**
   Entfernt die PID-Datei
*/

void DLSProcLogger::_remove_pid_file()
{
    stringstream pid_file_name;

    pid_file_name << _dls_dir << "/job" << dlsd_job_id << "/" << DLS_PID_FILE;

    if (unlink(pid_file_name.str().c_str()) == -1) {
        _exit = true;
        _exit_code = E_DLS_ERROR;
        msg() << "Could not remove PID file \"" << pid_file_name << "\"!";
        log(DLSError);
    }
}

/****************************************************************************/

/** Called by PdCom::Process when client data is queried.
 */
bool DLSProcLogger::clientInteraction(
        const string &,
        const string &,
        const string &,
        list<ClientInteraction> &interactionList
        )
{
    list<ClientInteraction>::iterator it;

    for (it = interactionList.begin(); it != interactionList.end(); it++) {
        if (it->prompt == "Username") {
			struct passwd *passwd = getpwuid(getuid());
			if (passwd) {
                it->response = passwd->pw_name;
			}
        }
		else if (it->prompt == "Hostname") {
			char hostname[256];
			if (!gethostname(hostname, sizeof(hostname))) {
				it->response = hostname;
			}
        }
		else if (it->prompt == "Application") {
			stringstream ident;
			ident << "dlsd-" << PACKAGE_VERSION
				<< "-r" << REVISION
				<< ", job " << _job_id;
			it->response = ident.str();
        }
    }

    return true;
}

/*****************************************************************************/

void DLSProcLogger::sigConnected()
{
	if (_job.preset()->trigger() == "") { // no trigger variable
		_state = Data;
        _last_receive_time.set_now();
        _receiving_data = false;
		_job.start_logging();

		msg() << "Start logging.";
        log(DLSInfo);
	}
	else { // trigger variable
		_state = Waiting;

		msg() << "Waiting for trigger \"";
		msg() << _job.preset()->trigger() << "\"...";
        log(DLSInfo);

        _subscribe_trigger();
	}
}

/****************************************************************************/

void DLSProcLogger::sendRequest()
{
#ifdef DEBUG_SEND
	cerr << __func__ << "()" << endl;
#endif
	_write_request = true;
}

/****************************************************************************/

int DLSProcLogger::sendData(const char *buf, size_t len)
{
#ifdef DEBUG_SEND
	cerr << __func__ << "(): " << string(buf, len) << endl;
#endif
	return ::write(_socket, buf, len);
}

/****************************************************************************/

void DLSProcLogger::processMessage(
        const PdCom::Time &time,
        LogLevel_t level,
        unsigned int, // messageNo
        const std::string& message
        ) const
{
    COMTime t;
    string storeType, displayType;

    t.from_dbl_time(time);

    switch (level) {
        case LogError:
            storeType = "error";
            displayType = "Error";
            break;
        case LogWarn:
            storeType = "warn";
            displayType = "Warning";
            break;
        case LogInfo:
            storeType = "info";
            displayType = "Information";
            break;
        case LogDebug:
            storeType = "info";
            displayType = "Debug";
        default:
            storeType = "info";
            displayType = "Unknown";
            break;
    }

	msg() << _job.preset()->source() << ":" << _job.preset()->port()
        << ": " << t.to_str()
		<< ", " << displayType
		<< ": " << message;
	log(DLSInfo);

    /* Unfortunately, processMessage is defined constant in PdCom::Process. */
    DLSProcLogger *logger = (DLSProcLogger *) this;
	logger->_job.message(t, storeType, message);
}

/****************************************************************************/

void DLSProcLogger::protocolLog(
        LogLevel_t level,
        const string &message
        ) const
{
    if (level > 2) {
        return;
    }

	msg() << "PdCom: " << message;
	log(DLSInfo);
}

/*****************************************************************************/

void DLSProcLogger::notify(PdCom::Variable *pv)
{
    bool run;

    pv->getValue(&run);

#ifdef DEBUG
    cout << __func__ << ": " << run << endl;
#endif

    if (_state == Waiting && run) {
        _state = Data;
        _last_receive_time.set_now();
        _receiving_data = false;

        msg() << "Trigger active! Start logging.";
        log(DLSInfo);

        _job.start_logging();
    }
    else if (_state == Data && !run) {
        msg() << "Trigger not active! Stop logging.";
        log(DLSInfo);


        _state = Waiting;
        _job.stop_logging();

        msg() << "Waiting for trigger...";
        log(DLSInfo);
    }
}

/***************************************************************************/

void DLSProcLogger::notifyDelete(PdCom::Variable *pv)
{
#ifdef DEBUG
    cout << __func__ << endl;
#endif

    if (_trigger && _trigger == pv) {
        _trigger = NULL;
    }
}

/*****************************************************************************/
