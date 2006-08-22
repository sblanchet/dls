/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include "lib_export.hpp"
using namespace LibDLS;

/******************************************************************************
 * Export
 *****************************************************************************/

Export::Export()
{
}

/*****************************************************************************/

Export::~Export()
{
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

void ExportAscii::begin(const Channel &channel, const string &path)
{
    stringstream filename;

    filename << path << "/channel" << channel.index() << ".dat";
    _file.open(filename.str().c_str(), ios::trunc);

    if (!_file.is_open()) {
        stringstream err;
        err << "Failed to open file \"" << filename.str() << "\"!";
        throw ExportException(err.str());
    }

    _file << "% --- DLS exported data ---" << endl;
    _file << "%" << endl;
    _file << "% Channel: " << channel.name() << endl;
    _file << "%    Unit: " << channel.unit() << endl;
    _file << "%" << endl;
}

/*****************************************************************************/

void ExportAscii::data(const Data *data)
{
    unsigned int i;

    for (i = 0; i < data->size(); i++) {
        _file << fixed << data->time(i) << "\t"
              << fixed << data->value(i) << endl;
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
}

/*****************************************************************************/

ExportMat4::~ExportMat4()
{
}

/*****************************************************************************/

void ExportMat4::begin(const Channel &channel, const string &path)
{
    stringstream filename, name;

    name << "channel" << channel.index();

    _header.type = 0000; // Little-Endian, double, numeric (full) matrix
    _header.mrows = 2;
    _header.ncols = 0; // set later
    _header.imagf = 0; // only real data, no imaginary part
    _header.namelen = name.str().size() + 1;

    filename << path << "/channel" << channel.index() << ".mat";
    _file.open_read_write(filename.str().c_str());

    _file.write((const char *) &_header, sizeof(Mat4Header));
    _file.write(name.str().c_str(), name.str().size() + 1);
}

/*****************************************************************************/

void ExportMat4::data(const Data *data)
{
    unsigned int i;
    double val;

    _header.ncols += data->size();

    for (i = 0; i < data->size(); i++) {
        val = data->time(i).to_dbl();
        _file.write((const char *) &val, sizeof(double));
        val = data->value(i);
        _file.write((const char *) &val, sizeof(double));
    }
}

/*****************************************************************************/

void ExportMat4::end()
{
    _file.seek(0); // write header again, this time with number of columns
    _file.write((const char *) &_header, sizeof(Mat4Header));
    _file.close();
}

/*****************************************************************************/
