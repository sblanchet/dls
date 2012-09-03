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

#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <typeinfo>
using namespace std;

/*****************************************************************************/

#include "com_xml_parser.hpp"
#include "com_index_t.hpp"
#include "com_file.hpp"
#include "com_ring_buffer_t.hpp"
#include "com_compression_t.hpp"

#include "lib_channel.hpp"
#include "lib_chunk.hpp"
using namespace LibDLS;

/*****************************************************************************/

/**
   Constructor.
*/

Chunk::Chunk()
{
}

/*****************************************************************************/

/**
   Destruktor
*/

Chunk::~Chunk()
{
}

/*****************************************************************************/

/**
   Importiert die Informationen aus "chunk.xml"
*/

void Chunk::import(const string &path, COMChannelType type)
{
    stringstream err;
    string chunk_file_name, format_str;
    fstream file;
    COMXMLParser xml;
    int i;

    _dir = path;
    _type = type;
    chunk_file_name = _dir + "/chunk.xml";

    file.open(chunk_file_name.c_str(), ios::in);

    if (!file.is_open()) {
        err << "Failed to open chunk file \"" << chunk_file_name << "\"!";
        throw ChunkException(err.str());
    }

    try {
        xml.parse(&file, "dlschunk", dxttBegin);
        xml.parse(&file, "chunk", dxttSingle);

        _sample_frequency = xml.tag()->att("sample_frequency")->to_dbl();
        _meta_reduction = xml.tag()->att("meta_reduction")->to_int();
        format_str = xml.tag()->att("format")->to_str();

        _format_index = DLS_FORMAT_INVALID;
        for (i = 0; i < DLS_FORMAT_COUNT; i++) {
            if (format_str == dls_format_strings[i]) {
                _format_index = i;
                break;
            }
        }

        if (_format_index == DLS_FORMAT_INVALID) {
            throw ChunkException("Unknown compression format!");
        }

        if (_format_index == DLS_FORMAT_MDCT) {
            _mdct_block_size = xml.tag()->att("mdct_block_size")->to_int();
        }
    }
    catch (ECOMXMLParser &e) {
        file.close();
        err << "Parsing error: " << e.msg;
        throw ChunkException(err.str());
    }
    catch (ECOMXMLParserEOF &e) {
        file.close();
        err << "Parsing error: " << e.msg;
        throw ChunkException(err.str());
    }
    catch (ECOMXMLTag &e) {
        file.close();
        err << "Parsing (tag) error: " << e.msg;
        throw ChunkException(err.str());
    }

    file.close();
}

/*****************************************************************************/

/**
   Fetches data.
*/

void LibDLS::Chunk::fetch_data(
        COMTime start,
        COMTime end,
        unsigned int min_values,
        COMRingBuffer *ring,
        DataCallback cb,
        void *cb_data, /**< arbitrary callback param */
        unsigned int decimation
        ) const
{
    unsigned int level, decimationCounter = 0;
    Data *data = NULL;
    COMTime time_per_value;

    if (!decimation) {
        stringstream err;
        err << "Decimation may not be zero!";
        throw ChunkException(err.str());
    }

    // The requested time range does not intersect the chunk's range.
    if (start > _end || end < _start) return;

    level = _calc_optimal_level(start, end, min_values);
    time_per_value = _time_per_value(level);

    if (!level) {
        _fetch_level_data_wrapper(start, end, DLSMetaGen, level,
                                  time_per_value, ring, &data, cb, cb_data,
                                  decimation, decimationCounter);
    } else {
        _fetch_level_data_wrapper(start, end, DLSMetaMin, level,
                                  time_per_value, ring, &data, cb, cb_data,
                                  decimation, decimationCounter);
        _fetch_level_data_wrapper(start, end, DLSMetaMax, level,
                                  time_per_value, ring, &data, cb, cb_data,
                                  decimation, decimationCounter);
    }
}

/*****************************************************************************/

/**
   Loads data from a specified meta level.
*/

void LibDLS::Chunk::_fetch_level_data_wrapper(COMTime start,
                                              COMTime end,
                                              DLSMetaType meta_type,
                                              unsigned int level,
                                              COMTime time_per_value,
                                              COMRingBuffer *ring,
                                              Data **data,
                                              DataCallback cb,
                                              void *cb_data, /**< arbitrary
                                                               callback
                                                               parameter */
                                              unsigned int decimation,
                                              unsigned int &decimationCounter
                                              ) const
{
    switch (_type) {
        case TCHAR:
            _fetch_level_data<char>(start, end, meta_type, level,
                                    time_per_value, ring, data, cb, cb_data,
                                    decimation, decimationCounter);
            break;
        case TUCHAR:
            _fetch_level_data<unsigned char>(start, end, meta_type, level,
                                             time_per_value, ring, data,
                                             cb, cb_data, decimation,
                                             decimationCounter);
            break;
        case TSHORT:
            _fetch_level_data<short>(start, end, meta_type, level,
                                     time_per_value, ring, data, cb, cb_data,
                                     decimation, decimationCounter);
            break;
        case TUSHORT:
            _fetch_level_data<unsigned short>(start, end, meta_type, level,
                                              time_per_value, ring, data,
                                              cb, cb_data, decimation,
                                              decimationCounter);
            break;
        case TINT:
            _fetch_level_data<int>(start, end, meta_type, level,
                                   time_per_value, ring, data, cb, cb_data,
                                   decimation, decimationCounter);
            break;
        case TUINT:
            _fetch_level_data<unsigned int>(start, end, meta_type, level,
                                            time_per_value, ring, data,
                                            cb, cb_data, decimation,
                                            decimationCounter);
            break;
        case TLINT:
            _fetch_level_data<long>(start, end, meta_type, level,
                                    time_per_value, ring, data, cb, cb_data,
                                    decimation, decimationCounter);
            break;
        case TULINT:
            _fetch_level_data<unsigned long>(start, end, meta_type, level,
                                             time_per_value, ring, data,
                                             cb, cb_data, decimation,
                                             decimationCounter);
            break;
        case TFLT:
            _fetch_level_data<float>(start, end, meta_type, level,
                                     time_per_value, ring, data, cb, cb_data,
                                     decimation, decimationCounter);
            break;
        case TDBL:
            _fetch_level_data<double>(start, end, meta_type, level,
                                      time_per_value, ring, data, cb, cb_data,
                                      decimation, decimationCounter);
            break;

        default: {
            stringstream err;
            err << "Unknown channel type " << _type << ".";
            throw ChunkException(err.str());
        }
    }
}

/*****************************************************************************/

/**
   Loads data.
*/

template <class T>
void LibDLS::Chunk::_fetch_level_data(COMTime start,
                                      COMTime end,
                                      DLSMetaType meta_type,
                                      unsigned int level,
                                      COMTime time_per_value,
                                      COMRingBuffer *ring,
                                      Data **data,
                                      DataCallback cb,
                                      void *cb_data, /**< arbitrary callback
                                                       parameter */
                                      unsigned int decimation,
                                      unsigned int &decimationCounter
                                      ) const
{
    stringstream level_dir_name;
    string global_index_file_name;
    stringstream data_file_name;
    COMIndexT<COMGlobalIndexRecord> global_index;
    COMGlobalIndexRecord global_index_record;
    COMIndexT<COMIndexRecord> index;
    COMIndexRecord index_record;
    COMFile data_file;
    unsigned int i, j, write_len, blocks_read = 0;
    char *write_ptr;
    COMXMLParser xml;
    bool must_read_again;
    COMCompressionT<T> *comp;

    if (_format_index == DLS_FORMAT_ZLIB) {
        comp = new COMCompressionT_ZLib<T>();
    }
    else if (_format_index == DLS_FORMAT_MDCT) {
        if (typeid(T) == typeid(float)) {
            comp = (COMCompressionT<T> *)
                new COMCompressionT_MDCT<float>(_mdct_block_size, 0);
        }
        else if (typeid(T) == typeid(double)) {
            comp = (COMCompressionT<T> *)
                new COMCompressionT_MDCT<double>(_mdct_block_size, 0);
        }
        else {
            cout << "ERROR: MDCT only for floating point types!" << endl;
            return;
        }
    }
    else if (_format_index == DLS_FORMAT_QUANT) {
        if (typeid(T) == typeid(float)) {
            comp = (COMCompressionT<T> *)
                new COMCompressionT_Quant<float>(0.0);
        }
        else if (typeid(T) == typeid(double)) {
            comp = (COMCompressionT<T> *)
                new COMCompressionT_Quant<double>(0.0);
        }
        else {
            cout << "ERROR: Quant only for floating point types!" << endl;
            return;
        }
    }
    else {
        cout << "ERROR: Unknown compression type index: "
             << _format_index << endl;
        return;
    }

    level_dir_name << _dir << "/level" << level;
    global_index_file_name = level_dir_name.str() + "/data_"
        + dls_meta_type_str(meta_type) + ".idx";

    try {
        global_index.open_read(global_index_file_name);
    } catch (ECOMIndexT &e) {
        // global index not found.
        delete comp;
        return;
    }

    // loop through all indexed data files
    for (i = 0; i < global_index.record_count(); i++) {
        try {
            global_index_record = global_index[i];
        } catch (ECOMIndexT &e) {
            cout << "ERROR: Failed read record " << i
                 << " from global index \"";
            cout << global_index_file_name << "\". Reason: " << e.msg << endl;
            delete comp;
            return;
        }

        if (COMTime(global_index_record.end_time) < start
            && global_index_record.end_time != 0) {
            // this data file covers a time range before the requested range
            continue;
        }

        if (COMTime(global_index_record.start_time) > end) {
            // from here, all data files cover time ranges after
            // the requested range -> abort search
            break;
        }

        data_file_name.str("");
        data_file_name.clear();
        data_file_name << level_dir_name.str()
                       << "/data" << global_index_record.start_time
                       << "_" << dls_meta_type_str(meta_type);

        try {
            index.open_read(data_file_name.str() + ".idx");
            data_file.open_read(data_file_name.str().c_str());
        } catch (ECOMIndexT &e) {
            cout << "ERROR: Failed to open index \"";
            cout << data_file_name.str() << ".idx\": " << e.msg << endl;
            delete comp;
            return;
        } catch (ECOMFile &e) {
            cout << "ERROR: Failed to open data file \"";
            cout << data_file_name.str() << "\": " << e.msg << endl;
            delete comp;
            return;
        }

        // loop through all index records
        for (j = 0; j < index.record_count(); j++) {
            try {
                index_record = index[j];
            } catch (ECOMIndexT &e) {
                cout << "ERROR: Could not read from index: " << e.msg << endl;
                delete comp;
                return;
            }

            // the block covers a time range before the requested range.
            // try the next one.
            if (COMTime(index_record.end_time) < start) continue;

            // the following blocks cover time ranges after the requested
            // range. abort loading.
            if (COMTime(index_record.start_time) >= end) break;

            try {
                data_file.seek(index_record.position);
            } catch (ECOMFile &e) {
                cout << "ERROR: Could not seek in data file!" << endl;
                delete comp;
                return;
            }

            ring->clear();

            // read bytes, until a tag is complete
            while (1) {
                ring->write_info(&write_ptr, &write_len);
                if (write_len > 1024) write_len = 1024;

                try {
                    data_file.read(write_ptr, write_len, &write_len);
                } catch (ECOMFile &e) {
                    cout << "ERROR: Could not read from data file: "
                         << e.msg << endl;
                    delete comp;
                    return;
                }

                if (!write_len) {
                    cout << "ERROR: EOF in \"" << data_file_name.str()
                         << "\" after searching position "
                         << index_record.position << "!" << endl;
                    delete comp;
                    return;
                }

                ring->written(write_len);

                try {
                    xml.parse(ring);
                } catch (ECOMXMLParserEOF &e) {
                    continue;
                } catch (ECOMXMLParser &e) {
                    cout << "parsing error: " << e.msg << endl;
                    delete comp;
                    return;
                }

                if (xml.tag()->title() == "d") {
                    try {
                        _process_data_tag(xml.tag(), index_record.start_time,
                                          meta_type, level, time_per_value,
                                          comp, data, cb, cb_data,
                                          decimation, decimationCounter);
                    } catch (ECOMXMLTag &e) {
                        cout << "ERROR: Could not read block: "
                             << e.msg << endl;
                        delete comp;
                        return;
                    }

                    blocks_read++;
                }

                break;
            }
            // next index record
        }
    }

    // blocks read, files still open

    if (blocks_read && _format_index == DLS_FORMAT_MDCT) {
        // read one more record for MDCT
        try {
            xml.parse(ring);
            must_read_again = false;
        }
        catch (ECOMXMLParser &e) {
            cout << "ERROR: While parsing: " << e.msg << endl;
            delete comp;
            return;
        }
        catch (ECOMXMLParserEOF &e) {
            must_read_again = true;
        }

        while (must_read_again) {
            ring->write_info(&write_ptr, &write_len);
            if (write_len > 1024) write_len = 1024;

            try {
                data_file.read(write_ptr, write_len, &write_len);
            }
            catch (ECOMFile &e) {
                cout << "ERROR: Could not read data file: " << e.msg << endl;
                delete comp;
                return;
            }

            if (!write_len) {
                delete comp;
                return;
            }

            ring->written(write_len);

            try {
                xml.parse(ring);
                must_read_again = false;
            }
            catch (ECOMXMLParser &e) {
                cout << "ERROR: While parsing: " << e.msg << endl;
                delete comp;
                return;
            }
            catch (ECOMXMLParserEOF &e) {
            }
        }

        if (xml.tag()->title() == "d") {
            try {
                _process_data_tag(xml.tag(), index_record.start_time,
                                  meta_type, level,
                                  time_per_value, comp, data, cb, data,
                                  decimation, decimationCounter);
            }
            catch (ECOMXMLTag &e) {
                cout << "ERROR: Failed to read block!" << endl;
                delete comp;
                return;
            }
        }
    }

    delete comp;
}

/*****************************************************************************/

/**
   Loads data from an XML tag.
*/

template <class T>
void LibDLS::Chunk::_process_data_tag(const COMXMLTag *tag,
                                      COMTime start_time,
                                      DLSMetaType meta_type,
                                      unsigned int level,
                                      COMTime time_per_value,
                                      COMCompressionT<T> *comp,
                                      Data **data,
                                      DataCallback cb,
                                      void *cb_data, /**< arbitrary callback
                                                       parameter */
                                      unsigned int decimation,
                                      unsigned int &decimationCounter
                                      ) const
{
    unsigned int block_size;
    const char *block_data;

    block_data = tag->att("d")->to_str().c_str();
    block_size = tag->att("s")->to_int();

    if (block_size) {
        try {
            comp->uncompress(block_data, strlen(block_data), block_size);
        } catch (ECOMCompression &e) {
            cout << "ERROR while uncompressing: " << e.msg << endl;
            return;
        }

        if (!*data) *data = new Data();

        (*data)->import(start_time, time_per_value, meta_type, level,
                decimation, decimationCounter, comp->decompression_output(),
                comp->decompressed_length());

        // invoke data callback
        if (cb(*data, cb_data)) {
            // data structure adopted: forget its address.
            *data = NULL;
        }
        return;
    } else if (_format_index == DLS_FORMAT_MDCT) {
        try {
            comp->flush_uncompress(block_data, strlen(block_data));
        } catch (ECOMCompression &e) {
            cout << "ERROR while uncompressing: " << e.msg << endl;
            return;
        }

        if (!*data) *data = new Data();

        (*data)->import(start_time, time_per_value, meta_type, level,
                decimation, decimationCounter, comp->decompression_output(),
                comp->decompressed_length());

        // invoke data callback
        if (cb(*data, cb_data)) {
            // data structure adopted: forget its address.
            *data = NULL;
        }
        return;
    }

    return;
}

/*****************************************************************************/

/**
   Holt den Zeitbereich des Chunks
*/

void Chunk::fetch_range()
{
    string global_index_file_name;
    stringstream err, index_file_name;
    COMIndexT<COMGlobalIndexRecord> global_index;
    COMGlobalIndexRecord first_global_index_record;
    COMGlobalIndexRecord last_global_index_record;
    COMIndexT<COMIndexRecord> index;
    COMIndexRecord index_record;

    _start = (uint64_t) 0;
    _end = (uint64_t) 0;

    global_index_file_name = _dir + "/level0/data_gen.idx";

    try
    {
        global_index.open_read(global_index_file_name);
    }
    catch (ECOMIndexT &e)
    {
        err << "Opening global index: " << e.msg << endl;
        throw ChunkException(err.str());
    }

    if (global_index.record_count() == 0)
    {
        err << "Global index file \"" << global_index_file_name
            << "\" has no records!";
        throw ChunkException(err.str());
    }

    try
    {
        // Ersten und letzten Record lesen, um die Zeitspanne zu bestimmen
        first_global_index_record = global_index[0];
        last_global_index_record
            = global_index[global_index.record_count() - 1];
    }
    catch (ECOMIndexT &e)
    {
        err << "Could not read first and last"
            << " record from global index file \""
            << global_index_file_name << "\"!";
        throw ChunkException(err.str());
    }

    // In die letzte Datendatei wird noch erfasst
    // -> Die aktuelle, letzte Zeit aus dem Datendatei-Index holen
    if (last_global_index_record.end_time == 0)
    {
        index_file_name << _dir << "/level0/data"
                        << last_global_index_record.start_time << "_gen.idx";

        try
        {
            // Index öffnen
            index.open_read(index_file_name.str());
        }
        catch (ECOMIndexT &e)
        {
            err << "Could not open index file \""
                << index_file_name.str() << "\": " << e.msg;
            throw ChunkException(err.str());
        }

        if (index.record_count() == 0)
        {
            err << "Index file \"" << index_file_name.str()
                << "\" has no records!";
            throw ChunkException(err.str());
        }

        try
        {
            // Letzten Record lesen
            index_record = index[index.record_count() - 1];
        }
        catch (ECOMIndexT &e)
        {
            err << "Could not read from index file \""
                << index_file_name.str() << "\": " << e.msg;
            throw ChunkException(err.str());
        }

        last_global_index_record.end_time = index_record.end_time;

        index.close();
    }

    global_index.close();

    _start = first_global_index_record.start_time;
    _end = last_global_index_record.end_time;
}

/*****************************************************************************/

/**
   Berechnet die optimale Meta-Ebene für die angegebene Auflösung

   \param start Anfang des zu ladenden zeitbereichs
   \param end Ende des zu ladenden Zeitbereichs
   \param values_wanted Maximale Anzahl der zu ladenden Werte
*/

unsigned int Chunk::_calc_optimal_level(COMTime start,
                                        COMTime end,
                                        unsigned int min_values) const
{
    double level;

    if (!min_values) return 0;

    level = floor(log10(_sample_frequency
                        * (end - start).to_dbl_time()
                        / min_values)
                  / log10((double) _meta_reduction));

    if (level < 0) level = 0;

    return (unsigned int) level;
}

/*****************************************************************************/

/**
   Calculates the time per value (delta t) for the specified meta level.
*/

COMTime Chunk::_time_per_value(unsigned int level) const
{
    return (pow((double) _meta_reduction, (double) level)
            * 1000000.0 / _sample_frequency);
}

/*****************************************************************************/

/**
   Kleiner-Operator.

   \return true, wenn der linke Chunk eher beginnt, als der Rechte.
*/

bool Chunk::operator<(const Chunk &other) const
{
    return _start < other._start;
}

/*****************************************************************************/

/**
   == operator.
   \return true, if the chunks have exactly the same times.
*/

bool Chunk::operator==(const Chunk &other) const
{
    return _start == other._start && _end == other._end;
}

/*****************************************************************************/
