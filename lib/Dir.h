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
using namespace std;

#include "Exception.h"
#include "Job.h"

/****************************************************************************/

namespace LibDLS {

/****************************************************************************/

/** DLS Directory Exception.
*/
class DirectoryException:
    public Exception
{
    public:
        DirectoryException(const string &pmsg):
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

        void importLocal(const string &);
        void importNetwork(const string &);

        const string &path() const { return _path; }
        list<Job> &jobs() { return _jobs; }
        Job *job(unsigned int);
        Job *find_job(unsigned int);

    private:
        string _path; /**< path to DLS data directory */
        list<Job> _jobs; /**< list of jobs */
};

/****************************************************************************/

} // namespace

/****************************************************************************/

#endif
