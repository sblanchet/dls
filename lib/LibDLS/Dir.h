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

#include "Exception.h"
#include "Job.h"

/****************************************************************************/

namespace DlsProto {
    class Request;
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

/** DLS Data Directory.
 */
class Directory
{
    public:
        Directory();
        ~Directory();

        void import(const std::string &);

        const std::string &path() const { return _path; }
        std::list<Job *> &jobs() { return _jobs; }
        Job *job(unsigned int);
        Job *find_job(unsigned int);

    private:
        enum {
            Unknown,
            Local,
            Network
        } _access; /**< Access type. */

        /* Local access. */
        std::string _path; /**< Path to DLS data directory. */

        /* Network access */
        std::string _host; /**< Host name/address. */
        std::string _port; /**< Port number / service name. */
        int _fd; /**< Socket file descriptor. */

        std::list<Job *> _jobs; /**< list of jobs */

        void _importLocal();
        void _importNetwork();

        void _connect();
        std::string _recv_message();
        void _send_message(const DlsProto::Request &);

        void _recv_hello();
        void _send_dir_info();
        void _recv_dir_info();
};

/****************************************************************************/

} // namespace

/****************************************************************************/

#endif
