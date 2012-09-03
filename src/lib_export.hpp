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

#ifndef LibExportHpp
#define LibExportHpp

/*****************************************************************************/

#include <fstream>
using namespace std;

#include "com_exception.hpp"
#include "com_file.hpp"

#include "lib_channel.hpp"

/*****************************************************************************/

namespace LibDLS
{
    /*************************************************************************/

    class ExportException : public COMException
    {
    public:
        ExportException(const string &pmsg) : COMException(pmsg) {};
    };

    /*************************************************************************/

    typedef struct
    {
        long type;
        long mrows;
        long ncols;
        long imagf;
        long namelen;
    }
    Mat4Header;

    /*************************************************************************/

    class Export
    {
    public:
        Export();
        virtual ~Export();

        virtual void begin(const Channel &, const string &) = 0;
        virtual void data(const Data *) = 0;
        virtual void end() = 0;
    };

    /*************************************************************************/

    class ExportAscii : public Export
    {
    public:
        ExportAscii();
        ~ExportAscii();

        void begin(const Channel &, const string &);
        void data(const Data *);
        void end();

    private:
        ofstream _file;
    };

    /*************************************************************************/

    class ExportMat4 : public Export
    {
    public:
        ExportMat4();
        ~ExportMat4();

        void begin(const Channel &, const string &);
        void data(const Data *);
        void end();

    private:
        Mat4Header _header;
        COMFile _file;
    };
}

/*****************************************************************************/

#endif
