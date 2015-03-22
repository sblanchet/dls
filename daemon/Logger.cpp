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

#include <fstream>
#include <sstream>
using namespace std;

/*****************************************************************************/

#include "lib/Base64.h"
#include "lib/XmlParser.h"
#include "lib/XmlTag.h"

#include "globals.h"
#include "Job.h"
#include "SaverGenT.h"
#include "Logger.h"

using namespace LibDLS;

/*****************************************************************************/

//#define DEBUG

/*****************************************************************************/

/**
   Kontruktor

   \param job Zeiger auf das besitzende Auftrags-Objekt
   \param channel_preset Kanalvorgaben
   \param dls_dir DLS-Datenverzeichnis
*/

Logger::Logger(const Job *job,
                     const ChannelPreset *channel_preset,
                     const string &dls_dir)
{
    _parent_job = job;
    _channel_preset = *channel_preset;
    _dls_dir = dls_dir;

    _gen_saver = 0;
    _change_in_progress = false;
    _finished = true;

    _channel_dir_acquired = false;
    _chunk_created = false;

    _data_size = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

Logger::~Logger()
{
    if (_gen_saver) delete _gen_saver;
}

/*****************************************************************************/

/**
   Holt sich den zu den Vorgaben passenden msrd-Kanal

   Die Liste der msrd-Kan�le wird auf den Namen des
   Vorgabekanals hin durchsucht. Wird dieser gefunden, dann
   werden die Eigenschaften des msrd-Kanals f�r sp�tere
   Zwecke kopiert.

   Da jetzt der Datentyp des Kanals bekannt ist, kann
   jetzt eine Instanz der SaverGenT - Template-Klasse
   gebildet und damit ein Saver-Objekt erzeugt werden.

   \param channels Konstante Liste der msrd-Kan�le
   \throw ELogger Kanal nicht gefunden, unbekannter Datentyp,
   oder Fehler beim Erstellen des Saver-Objektes
*/

void Logger::get_real_channel(const list<RealChannel> *channels)
{
    stringstream err;
    list<RealChannel>::const_iterator channel_i;

    channel_i = channels->begin();
    while (channel_i != channels->end())
    {
        if (channel_i->name == _channel_preset.name)
        {
            _real_channel = *channel_i;
            return;
        }

        channel_i++;
    }

    err << "Channel \"" << _channel_preset.name << "\" does not exist!";
    throw ELogger(err.str());
}

/*****************************************************************************/

/**
   Erstellt ein zu den Vorgaben passendes Saver-Objekt

   Instanziert das SaverGenT-Template mit dem entsprechenden
   Datentyp und erstellt dann die vorgegebene Meta-Saver.
*/

void Logger::create_gen_saver()
{
    if (_gen_saver) delete _gen_saver;
    _gen_saver = 0;

    try
    {
        switch (_real_channel.type)
        {
            case TCHAR:
                _gen_saver = new SaverGenT<char>(this);
                break;
            case TUCHAR:
                _gen_saver = new SaverGenT<unsigned char>(this);
                break;
            case TSHORT:
                _gen_saver = new SaverGenT<short int>(this);
                break;
            case TUSHORT:
                _gen_saver = new SaverGenT<unsigned short int>(this);
                break;
            case TINT:
                _gen_saver = new SaverGenT<int>(this);
                break;
            case TUINT:
                _gen_saver = new SaverGenT<unsigned int>(this);
                break;
            case TLINT:
                _gen_saver = new SaverGenT<long>(this);
                break;
            case TULINT:
                _gen_saver = new SaverGenT<unsigned long>(this);
                break;
            case TFLT:
                _gen_saver = new SaverGenT<float>(this);
                break;
            case TDBL:
                _gen_saver = new SaverGenT<double>(this);
                break;

            default: throw ELogger("Unknown data type!");
        }
    }
    catch (ESaver &e)
    {
        throw ELogger("Constructing new saver: " + e.msg);
    }
    catch (...)
    {
        throw ELogger("Out of memory while constructing saver!");
    }

    if (_channel_preset.meta_mask & MetaMean)
    {
        _gen_saver->add_meta_saver(MetaMean);
    }
    if (_channel_preset.meta_mask & MetaMin)
    {
        _gen_saver->add_meta_saver(MetaMin);
    }
    if (_channel_preset.meta_mask & MetaMax)
    {
        _gen_saver->add_meta_saver(MetaMax);
    }
}

/*****************************************************************************/

/**
   Verifiziert die angegebenen Vorgaben

   -# Die Sampling-Frequenz muss gr��er 0 sein
   -# Die Sampling-Frequenz darf den f�r den Kanal
      angegebenen Maximalwert nicht �bersteigen.
   -# Die Sampling-Frequenz muss zu einer ganzzahligen Untersetzung f�hren.
   -# blocksize * reduction < channel buffer size / 2!

   \param channel Konstanter Zeiger auf zu pr�fende Kanalvorgaben.
   Weglassen dieses Parameters erwirkt die Pr�fung der eigenen Vorgaben.
   \throw ELogger Kanalvorgaben nicht in Ordnung
*/

void Logger::check_presettings(const ChannelPreset *channel) const
{
    unsigned int reduction, block_size;
    stringstream err;

    // Wenn keine Kanalvorgaben �bergeben, eigene �berpr�fen
    if (!channel) channel = &_channel_preset;

    if (channel->sample_frequency <= 0.0) {
        err << "Channel \"" << channel->name << "\": "
            << "Invalid sample frequency "
            << channel->sample_frequency << "!";
        throw ELogger(err.str());
    }

    if (channel->sample_frequency > _real_channel.frequency) {
        err << "Channel \"" << channel->name << "\": "
            << "Sample frequency exceeds channel maximum"
            << " (" << channel->sample_frequency << " / "
            << _real_channel.frequency << " Hz)!";
        throw ELogger(err.str());
    }

    reduction = (unsigned int)
        (_real_channel.frequency / channel->sample_frequency + .5);
    block_size = (unsigned int) (channel->sample_frequency + .5);
    if (!block_size)
        block_size = 1;

    if (block_size * reduction > _real_channel.bufsize / 2) {
        err << "Channel \""<< channel->name << "\": "
            << "Buffer limit exceeded! "
            << block_size * reduction << " > " << _real_channel.bufsize / 2;
        throw ELogger(err.str());
    }

    if (channel->format_index == FORMAT_MDCT
            && _real_channel.type != TFLT
            && _real_channel.type != TDBL) {
        err << "MDCT compression only for floating point channels!";
        throw ELogger(err.str());
    }
}

/*****************************************************************************/

/**
 * Checks the given channel directory can be used for the current job.
 * \return non-zero, if the directory matches
 * \throw ELogger Failed to check.
 */

int Logger::_channel_dir_matches(const string &dir_name) const
{
    stringstream err;
    string channel_file_name;
    fstream channel_file;
    struct stat stat_buf;
    XmlParser xml;
    XmlTag channel_tag;

    if (stat(dir_name.c_str(), &stat_buf) == -1) {
        err << "Failed to stat() \"" << dir_name << "\": "
            << strerror(errno);
        throw ELogger(err.str());
    }

    if (!S_ISDIR(stat_buf.st_mode)) {
        err << "\"" << dir_name << "\" is not a directory!";
        throw ELogger(err.str());
    }

    channel_file_name = dir_name + "/channel.xml";
    if (lstat(channel_file_name.c_str(), &stat_buf) == -1) {
        err << "Failed to stat() \"" << channel_file_name << "\": "
            << strerror(errno);
        throw ELogger(err.str());
    }

    channel_file.open(channel_file_name.c_str(), ios::in);
    if (!channel_file) {
        err << "Failed to open() \"" << channel_file_name << "\": "
            << strerror(errno);
        throw ELogger(err.str());
    }

    try {
        xml.parse(&channel_file, "dlschannel", dxttBegin);
        channel_tag = *xml.parse(&channel_file, "channel", dxttSingle);
        xml.parse(&channel_file, "dlschannel", dxttEnd);
    }
    catch (EXmlParser &e) {
        err << "Parsing \"" << channel_file_name << "\": " << e.msg
            << " tag: " << e.tag;
        throw ELogger(err.str());
    }

    if (channel_tag.att("name")->to_str() != _channel_preset.name)
        return 0;

    if (channel_tag.att("unit")->to_str() != _real_channel.unit)
        return 0;

    if (channel_tag.att("type")->to_str()
            != channel_type_to_str(_real_channel.type))
        return 0;

    return 1;
}

/*****************************************************************************/

/**
   Erzeugt den Startbefehl f�r die Erfassung des Kanals

   \param channel Konstanter Zeiger auf die Kanalvorgabe
   \param id ID, die dem Startbefehl beigef�gt wird, um eine
   Best�tigung zu bekommen. Kann weggelassen werden.
   \return XML-Startbefehl
*/

string Logger::start_tag(const ChannelPreset *channel,
                            const string &id) const
{
    stringstream tag;
    unsigned int reduction;
    unsigned int block_size;

    reduction = (unsigned int)
        (_real_channel.frequency / channel->sample_frequency + .5);
    block_size = (unsigned int) (channel->sample_frequency + .5);
    if (!block_size)
        block_size = 1;

    // Blocksize begrenzen (msrd kann nur 1024)
    if (block_size > 1024)
        block_size = 1024;

#if 0
    // Blocksize muss ein Teiler von Sample-Frequency sein!
    if (fmod(channel->sample_frequency, block_size)) {
        stringstream err;
        err << "Block size (" << block_size << ")";
        err << " doesn't match frequency (" << channel->sample_frequency
            << ")!";
        throw ELogger(err.str());
    }
#endif

    tag << "<xsad channels=\"" << _real_channel.index << "\""
        << " reduction=\"" << reduction << "\""
        << " blocksize=\"" << block_size << "\""
        << " coding=\"Base64\"";
    if (id != "") tag << " id=\"" << id << "\"";
    tag << ">";

    return tag.str();
}

/*****************************************************************************/

/**
   Erzeugt den Befehl zum Anhalten der Datenerfassung

   \returns XML-Stoppbefehl
*/

string Logger::stop_tag() const
{
    stringstream tag;

    tag << "<xsod channels=\"" << _real_channel.index << "\">";
    return tag.str();
}

/*****************************************************************************/

/**
   Nimmt erfasste Daten zum Speichern entgegen

   Geht davon aus, dass die entgegengenommenen Daten
   Base64-kodierte Bin�rdaten sind. Diese werden
   dekodiert und intern dem Saver-Objekt �bergeben.

   \param data Konstante Referenz auf die Daten
   \param time Zeitpunkt des letzten Einzelwertes
   \throw ELogger Fehler beim Dekodieren oder Speichern
   \throw ETimeTolerance Toleranzfehler! Prozess beenden!
*/

void Logger::process_data(const string &data, Time time)
{
    Base64 base64;
#ifdef DEBUG
    Time start = Time::now();
#endif

    // Jetzt ist etwas im Gange
    if (_finished) _finished = false;

    try {
        // Daten dekodieren
        base64.decode(data.c_str(), data.length());
    }
    catch (EBase64 &e) {
        throw ELogger("Base64 error: " + e.msg);
    }

    try {
        // Daten an Saver �bergeben
        _gen_saver->process_data(base64.output(),
                                 base64.output_size(),
                                 time);
    }
    catch (ESaver &e) {
        throw ELogger("GenSaver: " + e.msg);
    }

#ifdef DEBUG
    cout << "Logger::process_data() for channel " << _real_channel.index
        << " took " << (Time::now() - start).to_dbl_time() << " s." << endl;
#endif
}

/*****************************************************************************/

/**
   Erzeugt ein neues Chunk-Verzeichnis

   \param time_of_first Zeit des ersten Einzelwertes zur
   Generierung des Verzeichnisnamens
   \throw ELogger Fehler beim Erstellen des Verzeichnisses
*/

void Logger::create_chunk(Time time_of_first)
{
    stringstream dir_name, err;
    fstream file;
    XmlTag tag;
    string arch_str, file_name;

    _chunk_created = false;

    if (_channel_preset.format_index < 0
        || _channel_preset.format_index >= FORMAT_COUNT) {
        throw ELogger("Invalid channel format!");
    }

    switch (arch) {
        case LittleEndian: arch_str = "LittleEndian"; break;
        case BigEndian: arch_str = "BigEndian"; break;
        default: throw ELogger("Unknown architecture!");
    }

    // acquire channel directory
    if (!_channel_dir_acquired)
        _acquire_channel_dir();

    // create chunk directory
    dir_name << _channel_dir_name << "/chunk" << time_of_first;
    _chunk_dir_name = dir_name.str();
    if (mkdir(_chunk_dir_name.c_str(), 0755)) {
        err << "Failed to create chunk directory \"" << _chunk_dir_name
            << "\": " << strerror(errno);
        throw ELogger(err.str());
    }

    // create chunk.xml
    file_name = dir_name.str() + "/chunk.xml";
    file.open(file_name.c_str(), ios::out);
    if (!file) {
        err << "Failed to create \"" << file_name << "\": "
            << strerror(errno);
        throw ELogger(err.str());
    }

    tag.clear();
    tag.title("dlschunk");
    tag.type(dxttBegin);
    file << tag.tag() << endl;

    tag.clear();
    tag.title("chunk");
    tag.push_att("sample_frequency", _channel_preset.sample_frequency);
    tag.push_att("block_size", _channel_preset.block_size);
    tag.push_att("meta_mask", _channel_preset.meta_mask);
    tag.push_att("meta_reduction", _channel_preset.meta_reduction);
    tag.push_att("format", format_strings[_channel_preset.format_index]);

    if (_channel_preset.format_index == FORMAT_MDCT) {
        tag.push_att("mdct_block_size", _channel_preset.mdct_block_size);
        tag.push_att("mdct_accuracy", _channel_preset.accuracy);
    }
    else if (_channel_preset.format_index == FORMAT_QUANT) {
        tag.push_att("accuracy", _channel_preset.accuracy);
    }

    tag.push_att("architecture", arch_str);

    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("dlschunk");
    tag.type(dxttEnd);
    file << tag.tag() << endl;

    file.close();
    _chunk_created = true;
}

/*****************************************************************************/

/**
 * Searches for a matching channel directory to store data.
 * If no matching directory is found, a new on is created.
 * \throw ELogger Failed to create directory.
 */

void Logger::_acquire_channel_dir()
{
    stringstream job_dir_name, err, index_stream;
    DIR *dir;
    struct dirent *dir_ent;
    string entry_name, channel_dir_name, file_name;
    ofstream file;
    unsigned int index, highest_index = 0;
    XmlTag tag;

    job_dir_name << _dls_dir << "/job" << _parent_job->preset()->id();

    if (!(dir = opendir(job_dir_name.str().c_str()))) {
        err << "Failed to open job directory \""
            << job_dir_name.str() << "\": " << strerror(errno);
        throw ELogger(err.str());
    }

    while ((dir_ent = readdir(dir))) {
        entry_name = dir_ent->d_name;

        if (entry_name.size() < 8) // size of "channelX"
            continue;

        if (entry_name.substr(0, 7) != "channel")
            continue;

        // only continue, if remaining characters are all numbers
        if (entry_name.find_first_not_of("0123456789", 7) != string::npos)
            continue;

        index_stream.clear();
        index_stream.str("");
        index_stream << entry_name.substr(7);
        index_stream >> index;
        if (index > highest_index) highest_index = index;

        channel_dir_name = job_dir_name.str() + "/" + entry_name;

        try {
            if (_channel_dir_matches(channel_dir_name)) {
                _channel_dir_name = channel_dir_name;
                break;
            }
        }
        catch (ELogger &e) {
            continue;
        }
    }

    closedir(dir);

    if (_channel_dir_name != "") // found a matching directory
        return;

    index_stream.clear();
    index_stream.str("");
    index_stream << (highest_index + 1);
    channel_dir_name = job_dir_name.str() + "/channel" + index_stream.str();

    if (mkdir(channel_dir_name.c_str(), 0755)) {
        err << "Failed to create channel directory \""
            << channel_dir_name << "\": " << strerror(errno);
        throw ELogger(err.str());
    }

    // create channel.xml
    file_name = channel_dir_name + "/channel.xml";
    file.open(file_name.c_str(), ios::out);
    if (!file) {
        err << "Failed to create \"" << file_name
            << "\": " << strerror(errno);
        throw ELogger(err.str());
    }

    tag.clear();
    tag.title("dlschannel");
    tag.type(dxttBegin);
    file << tag.tag() << endl;

    tag.clear();
    tag.title("channel");
    tag.push_att("name", _channel_preset.name);
    tag.push_att("unit", _real_channel.unit);
    tag.push_att("type", channel_type_to_str(_real_channel.type));
    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("dlschannel");
    tag.type(dxttEnd);
    file << tag.tag() << endl;

    file.close();

    _channel_dir_name = channel_dir_name;
}

/*****************************************************************************/

/**
   Verwirft alle Daten und erstellt einen neuen Chunk

   Diese Methode l�scht alle Daten im Speicher. Sie sollte
   nur mit Bedacht aufgerufen werden (z. B. in einem Zweig
   nach einem fork(), wobei der andere Zweig die Daten speichert).
*/

void Logger::discard_chunk()
{
    // Vorgeben, dass noch keine Daten geschrieben wurden
    _data_size = 0;

    // Neuen Saver erstellen (l�scht vorher den alten Saver)
    create_gen_saver();

    // Vorgeben, dass noch kein Chunk existiert
    _chunk_created = false;
}

/*****************************************************************************/

/**
   Merkt eine �nderung der Kanalvorgaben vor

   Wenn ein �nderungsbefehl gesendet wurde, die
   Best�tigung aber noch nicht da ist, kann es sein,
   dass noch Daten entsprechend der "alten" Vorgabe empfangen
   werden. Deshalb darf die Vorgabe solange nicht �bernommen
   werden, bis die Best�tigung da ist.

   Das passiert mit dieser Methode. Sie merkt die neue Vorgabe vor.
   Sobald eine Best�tigung mit der angegebenen ID
   empfangen wird, muss do_change() aufgerufen werden.

   \param channel Die neue, "wartende" Kanalvorgabe
   \param id �nderungs-ID, die mit dem �nderungsbefehl
   gesendet wurde.
   \see change_is()
   \see do_change()
*/

void Logger::set_change(const ChannelPreset *channel,
                           const string &id)
{
    if (_change_in_progress)
    {
        msg() << "Change in progress!";
        log(Warning);
    }
    else
    {
        _change_in_progress = true;
    }

    _change_id = id;
    _change_channel = *channel;
}

/*****************************************************************************/

/**
   Abfrage auf eine bestimmte �nderungs-ID

   Gibt "true" zur�ck, wenn die Angegebene ID
   auf die vorgemerkte passt und zudem eine �nderung wartet

   \return true, wenn ID passt
*/

bool Logger::change_is(const string &id) const
{
    return _change_id == id && _change_in_progress;
}

/*****************************************************************************/

/**
   F�hrt eine vorgemerkte �nderung durch

   \throw ELogger Fehler beim Speichern von wartenden Daten
*/

void Logger::do_change()
{
    if (!_change_in_progress) return;

    // Chunks beenden!
    finish();

    // �nderungen �bernehmen
    _channel_preset = _change_channel;

    // Saver-Objekt neu erstellen
    create_gen_saver();

    // Jetzt wartet keine �nderung mehr
    _change_in_progress = false;
}

/*****************************************************************************/

/**
   Speichert wartende Daten

   Speichert alle Daten, die noch nicht im Dateisystem sind.

   \throw ELogger Fehler beim Speichern - Datenverlust!
*/

void Logger::finish()
{
    stringstream err;
    bool error = false;
#ifdef DEBUG
    Time start = Time::now();
#endif

    try {
        // Alle Daten speichern
        if (_gen_saver) _gen_saver->flush();
    }
    catch (ESaver &e) {
        error = true;
        err << "saver::flush(): " << e.msg;
    }

    // Chunk beenden
    _chunk_created = false;

    if (error) {
        throw ELogger(err.str());
    }

#ifdef DEBUG
    cout << "Logger::finish() for channel " << _real_channel.index
        << " took " << (Time::now() - start).to_dbl_time() << " s." << endl;
#endif

    _finished = true;
}

/*****************************************************************************/