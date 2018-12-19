/******************************************************************************
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

#include <sstream>
#include <limits>
#include <iomanip>
using namespace std;

#include "LibDLS/Export.h"

#include "File.h"

using namespace LibDLS;

/******************************************************************************
 * Export::Impl
 *****************************************************************************/

class Export::Impl {
    public:
        Impl();

        Time referenceTime;
        bool trim;
        Time trimStart;
        Time trimEnd;
};

/*****************************************************************************/

Export::Impl::Impl():
    trim(false)
{
}

/******************************************************************************
 * Export
 *****************************************************************************/

Export::Export():
    _impl(new Impl())
{
}

/*****************************************************************************/

Export::~Export()
{
}

/*****************************************************************************/

void Export::setReferenceTime(const Time &ref)
{
    _impl->referenceTime = ref;
}

/*****************************************************************************/

void Export::setTrim(const Time &start, const Time &end)
{
    _impl->trim = true;
    _impl->trimStart = start;
    _impl->trimEnd = end;
}

/******************************************************************************
 * ExportAscii
 *****************************************************************************/

ExportAscii::ExportAscii()
{
}

/*****************************************************************************/

ExportAscii::~ExportAscii()
{
}

/*****************************************************************************/

void ExportAscii::begin(
        const Channel &channel,
        const string &path,
        const string &filename
        )
{
    stringstream filepath;

    filepath << path << "/";

    if (filename.empty()) {
        filepath << "channel" << channel.dir_index();
    }
    else {
        filepath << filename;
    }

    filepath << ".dat";
    _file.open(filepath.str().c_str(), ios::trunc);

    if (!_file.is_open()) {
        stringstream err;
        err << "Failed to open file \"" << filepath.str() << "\"!";
        throw ExportException(err.str());
    }

    _file << "% --- DLS exported data ---" << endl;
    _file << "%" << endl;
    _file << "% Channel: " << channel.name() << endl;
    _file << "%    Unit: " << channel.unit() << endl;
    _file << "%" << endl;

    _file << setprecision(std::numeric_limits<long double>::digits10);
}

/*****************************************************************************/

void ExportAscii::data(const Data *data)
{
    unsigned int i;

    for (i = 0; i < data->size(); i++) {
        Time time(data->time(i));
        if (!_impl->trim ||
                (time >= _impl->trimStart && time <= _impl->trimEnd)) {
            _file << fixed << time - _impl->referenceTime << "\t"
                << fixed << data->value(i) << endl;
        }
    }
}

/*****************************************************************************/

void ExportAscii::end()
{
    _file.close();
}

/******************************************************************************
 * ExportMat4
 *****************************************************************************/

ExportMat4::ExportMat4()
{
    _file = new File();
}

/*****************************************************************************/

ExportMat4::~ExportMat4()
{
    delete _file;
}

/*****************************************************************************/

void ExportMat4::begin(
        const Channel &channel,
        const string &path,
        const string &filename
        )
{
    stringstream name;

    if (filename.empty()) {
        name << "channel" << channel.dir_index();
    }
    else {
        name << filename;
    }

    _header.type = 0000; // Little-Endian, double, numeric (full) matrix
    _header.mrows = 2;
    _header.ncols = 0; // set later
    _header.imagf = 0; // only real data, no imaginary part
    _header.namelen = name.str().size() + 1;

    stringstream filepath;
    filepath << path << "/" << name.str() << ".mat";
    _file->open_read_write(filepath.str().c_str());

    _file->write((const char *) &_header, sizeof(Mat4Header));
    _file->write(name.str().c_str(), name.str().size() + 1);
}

/*****************************************************************************/

void ExportMat4::data(const Data *data)
{
    unsigned int i;
    double val;

    for (i = 0; i < data->size(); i++) {
        Time time(data->time(i));
        if (!_impl->trim ||
                (time >= _impl->trimStart && time <= _impl->trimEnd)) {
            val = (data->time(i) - _impl->referenceTime).to_dbl();
            _file->write((const char *) &val, sizeof(double));
            val = data->value(i);
            _file->write((const char *) &val, sizeof(double));
            _header.ncols++;
        }
    }
}

/*****************************************************************************/

void ExportMat4::end()
{
    _file->seek(0); // write header again, this time with number of columns
    _file->write((const char *) &_header, sizeof(Mat4Header));
    _file->close();
}

/*****************************************************************************/
