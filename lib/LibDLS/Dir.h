/*****************************************************************************
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
 ****************************************************************************/

#ifndef LibDLSDirH
#define LibDLSDirH

/****************************************************************************/

#include <string>
#include <list>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message.h>

#include "Exception.h"
#include "Job.h"

/****************************************************************************/

namespace DlsProto {
    class Request;
    class Response;
    class DirInfo;
}

namespace LibDLS {

/****************************************************************************/

/** DLS Directory Exception.
*/
class DirectoryException:
    public Exception
{
    public:
        DirectoryException(const std::string &pmsg):
            Exception(pmsg) {};
};

/****************************************************************************/

/** DLS Observer class.
*/
class Observer
{
    public:
        virtual void update() = 0;
};

/****************************************************************************/

/** DLS Data Directory.
 */
class Directory
{
    friend class Job;
    friend class Channel;

    public:
        Directory(const std::string & = std::string());
        ~Directory();

        void set_uri(const std::string &);
        const std::string &uri() const { return _uri_text; }

        enum Access {
            Unknown,
            Local,
            Network
        }; /**< Access type. */
        Access access() const { return _access; }

        const std::string &path() const { return _path; }
        const std::string &host() const { return _host; }
        const std::string &port() const { return _port; }

        void import();

        bool connected() const { return _fd != -1; }

        std::list<Job *> &jobs() { return _jobs; }
        Job *job(unsigned int);
        Job *find_job(unsigned int);

        void set_dir_info(DlsProto::DirInfo *) const;

        void attach_observer(Observer *);
        void remove_observer(Observer *);

        const std::string &error_msg() const { return _error_msg; }

    private:
        std::string _uri_text;
        Access _access;

        /* Local access. */
        std::string _path; /**< Path to DLS data directory. */

        /* Network access */
        std::string _host; /**< Host name/address. */
        std::string _port; /**< Port number / service name. */
        int _fd; /**< Socket file descriptor. */
        google::protobuf::io::FileInputStream *_fis;
        google::protobuf::io::FileOutputStream *_fos;

        std::list<Job *> _jobs; /**< list of jobs */

        std::set<Observer *> _observers;

        std::string _error_msg; /**< Last error message. */

        void _importLocal();
        void _importNetwork();

        void _connect();
        void _disconnect();
        void _network_request_sync(const DlsProto::Request &,
                DlsProto::Response &);
        void _send_message(const DlsProto::Request &);
        void _receive_message(google::protobuf::Message &, bool debug = 1);
        void _receive_hello();

        void _notify_observers();
};

/****************************************************************************/

} // namespace

/****************************************************************************/

#endif
