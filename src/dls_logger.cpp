/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <fstream>
#include <sstream>
using namespace std;

/*****************************************************************************/

#include "com_base64.hpp"
#include "com_xml_parser.hpp"
#include "com_xml_tag.hpp"
#include "dls_globals.hpp"
#include "dls_job.hpp"
#include "dls_saver_gen_t.hpp"
#include "dls_logger.hpp"

/*****************************************************************************/

//#define DEBUG

/*****************************************************************************/

/**
   Kontruktor

   \param job Zeiger auf das besitzende Auftrags-Objekt
   \param channel_preset Kanalvorgaben
   \param dls_dir DLS-Datenverzeichnis
*/

DLSLogger::DLSLogger(const DLSJob *job,
                     const COMChannelPreset *channel_preset,
                     const string &dls_dir)
{
    _parent_job = job;
    _channel_preset = *channel_preset;
    _dls_dir = dls_dir;

    _gen_saver = 0;
    _change_in_progress = false;
    _finished = true;

    _channel_dir_exists = false;
    _channel_file_exists = false;
    _chunk_created = false;

    _data_size = 0;
}

/*****************************************************************************/

/**
   Destruktor
*/

DLSLogger::~DLSLogger()
{
    if (_gen_saver) delete _gen_saver;
}

/*****************************************************************************/

/**
   Holt sich den zu den Vorgaben passenden msrd-Kanal

   Die Liste der msrd-Kanäle wird auf den Namen des
   Vorgabekanals hin durchsucht. Wird dieser gefunden, dann
   werden die Eigenschaften des msrd-Kanals für spätere
   Zwecke kopiert.

   Da jetzt der Datentyp des Kanals bekannt ist, kann
   jetzt eine Instanz der DLSSaverGenT - Template-Klasse
   gebildet und damit ein Saver-Objekt erzeugt werden.

   \param channels Konstante Liste der msrd-Kanäle
   \throw EDLSLogger Kanal nicht gefunden, unbekannter Datentyp,
   oder Fehler beim Erstellen des Saver-Objektes
*/

void DLSLogger::get_real_channel(const list<COMRealChannel> *channels)
{
    stringstream err;
    list<COMRealChannel>::const_iterator channel_i;

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
    throw EDLSLogger(err.str());
}

/*****************************************************************************/

/**
   Erstellt ein zu den Vorgaben passendes Saver-Objekt

   Instanziert das DLSSaverGenT-Template mit dem entsprechenden
   Datentyp und erstellt dann die vorgegebene Meta-Saver.
*/

void DLSLogger::create_gen_saver()
{
    if (_gen_saver) delete _gen_saver;
    _gen_saver = 0;

    try
    {
        switch (_real_channel.type)
        {
            case TCHAR:
                _gen_saver = new DLSSaverGenT<char>(this);
                break;
            case TUCHAR:
                _gen_saver = new DLSSaverGenT<unsigned char>(this);
                break;
            case TSHORT:
                _gen_saver = new DLSSaverGenT<short int>(this);
                break;
            case TUSHORT:
                _gen_saver = new DLSSaverGenT<unsigned short int>(this);
                break;
            case TINT:
                _gen_saver = new DLSSaverGenT<int>(this);
                break;
            case TUINT:
                _gen_saver = new DLSSaverGenT<unsigned int>(this);
                break;
            case TLINT:
                _gen_saver = new DLSSaverGenT<long>(this);
                break;
            case TULINT:
                _gen_saver = new DLSSaverGenT<unsigned long>(this);
                break;
            case TFLT:
                _gen_saver = new DLSSaverGenT<float>(this);
                break;
            case TDBL:
                _gen_saver = new DLSSaverGenT<double>(this);
                break;

            default: throw EDLSLogger("Unknown data type!");
        }
    }
    catch (EDLSSaver &e)
    {
        throw EDLSLogger("Constructing new saver: " + e.msg);
    }
    catch (...)
    {
        throw EDLSLogger("Out of memory while constructing saver!");
    }

    if (_channel_preset.meta_mask & DLSMetaMean)
    {
        _gen_saver->add_meta_saver(DLSMetaMean);
    }
    if (_channel_preset.meta_mask & DLSMetaMin)
    {
        _gen_saver->add_meta_saver(DLSMetaMin);
    }
    if (_channel_preset.meta_mask & DLSMetaMax)
    {
        _gen_saver->add_meta_saver(DLSMetaMax);
    }
}

/*****************************************************************************/

/**
   Verifiziert die angegebenen Vorgaben

   -# Die Sampling-Frequenz muss größer 0 sein
   -# Die Sampling-Frequenz darf den für den Kanal
      angegebenen Maximalwert nicht übersteigen.
   -# Die Sampling-Frequenz muss zu einer ganzzahligen Untersetzung führen.
   -# blocksize * reduction < channel buffer size / 2!

   \param channel Konstanter Zeiger auf zu prüfende Kanalvorgaben.
   Weglassen dieses Parameters erwirkt die Prüfung der eigenen Vorgaben.
   \throw EDLSLogger Kanalvorgaben nicht in Ordnung
*/

void DLSLogger::check_presettings(const COMChannelPreset *channel) const
{
    double reduction;
    unsigned int block_size;
    stringstream err;

    // Wenn keine Kanalvorgaben übergeben, eigene überprüfen
    if (!channel) channel = &_channel_preset;

    if (channel->sample_frequency <= 0)
    {
        err << "Channel \"" << channel->name << "\": ";
        err << "Illegal frequency! (" << channel->sample_frequency << " Hz)";
        throw EDLSLogger(err.str());
    }

    if (channel->sample_frequency > _real_channel.frequency)
    {
        err << "Channel \"" << channel->name << "\": ";
        err << "Frequency exceeds channel maximum!";
        err << " (" << channel->sample_frequency << " / ";
        err << _real_channel.frequency << " Hz)";
        throw EDLSLogger(err.str());
    }

    reduction = _real_channel.frequency / (double) channel->sample_frequency;

    if (reduction != (unsigned int) reduction)
    {
        err << "Channel \""<< channel->name << "\": ";
        err << "Frequency leads to non-integer reduction!";
        throw EDLSLogger(err.str());
    }

    block_size = channel->sample_frequency;

    if (block_size * reduction > _real_channel.bufsize / 2)
    {
        err << "Channel \""<< channel->name << "\": ";
        err << "Buffer limit exceeded! ";
        err << block_size * reduction << " > " << _real_channel.bufsize / 2;
        throw EDLSLogger(err.str());
    }

    if (channel->format_index == DLS_FORMAT_MDCT
        && _real_channel.type != TFLT
        && _real_channel.type != TDBL)
    {
        err << "MDCT compression only for floating point channels!";
        throw EDLSLogger(err.str());
    }
}

/*****************************************************************************/

/**
   Prüft, ob eine bereits existierende "channel.xml" gültig ist

   Wenn im MSR-Modul Kanäle geändert werden (Typänderung,
   Verschiebung, ...), kann es sein, dass bei einer erneuten
   Erfassung die Angaben in einer existierenden "channel.xml"
   nicht mehr stimmen. Dies ist fatal. Dann muss erst ein neuer
   Job erstellt werden.

   \throw EDLSLogger Kanalinfodatei stimmt nicht mehr!
*/

void DLSLogger::check_channel_info()
{
    stringstream channel_dir_name, err;
    string channel_file_name;
    fstream channel_file;
    struct stat stat_buf;
    COMXMLParser xml;
    COMXMLTag channel_tag;

    _channel_dir_exists = false;
    _channel_file_exists = false;

    channel_dir_name << _dls_dir;
    channel_dir_name << "/job" << _parent_job->preset()->id();
    channel_dir_name << "/channel" << _real_channel.index;
    channel_file_name = channel_dir_name.str() + "/channel.xml";

    // Prüfen, ob Kanalverzeichnis bereits existiert
    if (stat(channel_dir_name.str().c_str(), &stat_buf) == -1)
    {
        if (errno == ENOENT)
        {
            // Existiert noch nicht. Alles ok!
            return;
        }
        else
        {
            err << "Could not stat() \"" << channel_dir_name.str() << "\"!";
            throw EDLSLogger(err.str());
        }
    }

    if (!S_ISDIR(stat_buf.st_mode))
    {
        err << "\"" << channel_dir_name.str() << "\" is not a directory!";
        throw EDLSLogger(err.str());
    }

    _channel_dir_exists = true;

    // Prüfen, ob "channel.xml" existiert
    if (lstat(channel_file_name.c_str(), &stat_buf) == -1)
    {
        if (errno == ENOENT)
        {
            // Datei existiert noch nicht. Alles ok.
            return;
        }
        else
        {
            err << "Could not stat() \"" << channel_file_name << "\"!";
            throw EDLSLogger(err.str());
        }
    }

    _channel_file_exists = true;

    // Datei existiert. Jetzt öffnen.
    channel_file.open(channel_file_name.c_str(), ios::in);

    if (!channel_file.is_open())
    {
        err << "Could not open \"" << channel_file_name << "\"!";
        throw EDLSLogger(err.str());
    }

    try
    {
        xml.parse(&channel_file, "dlschannel", dxttBegin);
        channel_tag = *xml.parse(&channel_file, "channel", dxttSingle);
        xml.parse(&channel_file, "dlschannel", dxttEnd);
    }
    catch (ECOMXMLParser &e)
    {
        err << "Parsing \"" << channel_file_name << "\": " << e.msg
            << " tag: " << e.tag;
        throw EDLSLogger(err.str());
    }

    // Den Kanalnamen überprüfen
    if (channel_tag.att("name")->to_str() != _channel_preset.name)
    {
        err << "Different channel name in \"" << channel_file_name << "\": ";
        err << "\"" << channel_tag.att("name")->to_str() << "\" instead of ";
        err << "\"" << _channel_preset.name << "\"!";
        throw EDLSLogger(err.str());
    }

    // Index überprüfen (sollte immer richtig sein)
    if (channel_tag.att("index")->to_int() != _real_channel.index)
    {
        err << "Wrong channel index in \"" << channel_file_name << "\": ";
        err << channel_tag.att("index")->to_int() << " instead of ";
        err << _real_channel.index << "!";
        throw EDLSLogger(err.str());
    }

    // Einheit überprüfen
    if (channel_tag.att("unit")->to_str() != _real_channel.unit)
    {
        err << "Different channel unit in \"" << channel_file_name << "\": ";
        err << "\"" << channel_tag.att("unit")->to_str() << "\" instead of ";
        err << "\"" << _real_channel.unit << "\"!";
        throw EDLSLogger(err.str());
    }

    // Datentyp überprüfen
    if (channel_tag.att("type")->to_str()
        != dls_channel_type_to_str(_real_channel.type))
    {
        err << "Different channel type in \"" << channel_file_name << "\": ";
        err << "\"" << channel_tag.att("type")->to_str() << "\" instead of ";
        err << "\"" << dls_channel_type_to_str(_real_channel.type) << "\"!";
        throw EDLSLogger(err.str());
    }
}

/*****************************************************************************/

/**
   Erzeugt den Startbefehl für die Erfassung des Kanals

   \param channel Konstanter Zeiger auf die Kanalvorgabe
   \param id ID, die dem Startbefehl beigefügt wird, um eine
   Bestätigung zu bekommen. Kann weggelassen werden.
   \return XML-Startbefehl
*/

string DLSLogger::start_tag(const COMChannelPreset *channel,
                            const string &id) const
{
    stringstream tag;
    double reduction;
    unsigned int block_size;

    reduction = _real_channel.frequency / channel->sample_frequency;

    if (reduction != (unsigned int) reduction)
    {
        throw EDLSLogger("Frequency leads to no integer reduction!");
    }

    block_size = channel->sample_frequency;

    // Blocksize begrenzen (msrd kann nur 1024)
    if (block_size > 1000) block_size = 1000;

    // Blocksize muss ein Teiler von Sample-Frequency sein!
    if (channel->sample_frequency % block_size != 0)
    {
        stringstream err;
        err << "Block size (" << block_size << ")";
        err << " doesn't match frequency (" << channel->sample_frequency
            << ")!";
        throw EDLSLogger(err.str());
    }

    tag << "<xsad channels=\"" << _real_channel.index << "\"";
    tag << " reduction=\"" << (unsigned int) reduction << "\"";
    tag << " blocksize=\"" << block_size << "\"";
    tag << " coding=\"Base64\"";
    if (id != "") tag << " id=\"" << id << "\"";
    tag << ">";

    return tag.str();
}

/*****************************************************************************/

/**
   Erzuegt den Befehl zum Anhalten der Datenerfassung

   \returns XML-Stoppbefehl
*/

string DLSLogger::stop_tag() const
{
    stringstream tag;

    tag << "<xsod channels=\"" << _real_channel.index << "\">";
    return tag.str();
}

/*****************************************************************************/

/**
   Nimmt erfasste Daten zum Speichern entgegen

   Geht davon aus, dass die entgegengenommenen Daten
   Base64-kodierte Binärdaten sind. Diese werden
   dekodiert und intern dem Saver-Objekt übergeben.

   \param data Konstante Referenz auf die Daten
   \param time Zeitpunkt des letzten Einzelwertes
   \throw EDLSLogger Fehler beim Dekodieren oder Speichern
   \throw EDLSTimeTolerance Toleranzfehler! Prozess beenden!
*/

void DLSLogger::process_data(const string &data, COMTime time)
{
    COMBase64 base64;

    // Jetzt ist etwas im Gange
    if (_finished) _finished = false;

    try
    {
        // Daten dekodieren
        base64.decode(data.c_str(), data.length());
    }
    catch (ECOMBase64 &e)
    {
        throw EDLSLogger("Base64 error: " + e.msg);
    }

    try
    {
        // Daten an Saver übergeben
        _gen_saver->process_data(base64.output(),
                                 base64.output_size(),
                                 time);
    }
    catch (EDLSSaver &e)
    {
        throw EDLSLogger("GenSaver: " + e.msg);
    }
}

/*****************************************************************************/

/**
   Erzeugt ein neues Chunk-Verzeichnis

   \param time_of_first Zeit des ersten Einzelwertes zur
   Generierung des Verzeichnisnamens
   \throw EDLSLogger Fehler beim Erstellen des Verzeichnisses
*/

void DLSLogger::create_chunk(COMTime time_of_first)
{
    stringstream dir_name;
    stringstream err;
    fstream file;
    COMXMLTag tag;
    string arch_str;

    _chunk_created = false;

    if (_channel_preset.format_index < 0
        || _channel_preset.format_index >= DLS_FORMAT_COUNT)
    {
        throw EDLSLogger("Invalid channel format!");
    }

    switch (arch)
    {
        case LittleEndian: arch_str = "LittleEndian"; break;
        case BigEndian: arch_str = "BigEndian"; break;
        default: throw EDLSLogger("Unknown architecture!");
    }

    dir_name << _dls_dir;
    dir_name << "/job" << _parent_job->preset()->id();
    dir_name << "/channel" << _real_channel.index;

    // Prüfen, ob Kanalverzeichnis bereits existiert
    if (!_channel_dir_exists)
    {
        // Kanalverzeichnis existiert nicht. Anlegen.
        if (mkdir(dir_name.str().c_str(), 0755) != 0)
        {
            err << "Could not create channel directory: ";
            err << " \"" << dir_name.str() << "\" while creating chunk!";
            throw EDLSLogger(err.str());
        }

        _channel_dir_exists = true;
    }

    if (!_channel_file_exists)
    {
        // channel.xml anlegen
        file.open((dir_name.str() + "/channel.xml").c_str(), ios::out);

        if (!file.is_open())
        {
            err << "Could not write channel.xml into dir \"";
            err << dir_name.str() << "\" while creating chunk!";
            throw EDLSLogger(err.str());
        }

        tag.clear();
        tag.title("dlschannel");
        tag.type(dxttBegin);
        file << tag.tag() << endl;

        tag.clear();
        tag.title("channel");
        tag.push_att("name", _channel_preset.name);
        tag.push_att("index", _real_channel.index);
        tag.push_att("unit", _real_channel.unit);
        tag.push_att("type", dls_channel_type_to_str(_real_channel.type));
        file << " " << tag.tag() << endl;

        tag.clear();
        tag.title("dlschannel");
        tag.type(dxttEnd);
        file << tag.tag() << endl;

        file.close();

        _channel_file_exists = true;
    }

    // Chunk-Verzeichnis erstellen
    dir_name << "/chunk" << time_of_first;
    if (mkdir(dir_name.str().c_str(), 0755) != 0)
    {
        err << "Could not create directory \"";
        err << dir_name.str() << "\" while creating chunk!";
        if (errno == EEXIST) err << " It already exists!";
        throw EDLSLogger(err.str());
    }

    // chunk.xml anlegen
    file.open((dir_name.str() + "/chunk.xml").c_str(), ios::out);

    if (!file.is_open())
    {
        err << "Could not write chunk.xml into dir \"";
        err << dir_name.str() << "\" while creating chunk!";
        throw EDLSLogger(err.str());
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
    tag.push_att("format", dls_format_strings[_channel_preset.format_index]);

    if (_channel_preset.format_index == DLS_FORMAT_MDCT)
    {
        tag.push_att("mdct_block_size", _channel_preset.mdct_block_size);
        tag.push_att("mdct_accuracy", _channel_preset.accuracy);
    }

    else if (_channel_preset.format_index == DLS_FORMAT_QUANT)
    {
        tag.push_att("accuracy", _channel_preset.accuracy);
    }

    tag.push_att("architecture", arch_str);

    file << " " << tag.tag() << endl;

    tag.clear();
    tag.title("dlschunk");
    tag.type(dxttEnd);
    file << tag.tag() << endl;

    file.close();

    _chunk_dir = dir_name.str();
    _chunk_created = true;
}

/*****************************************************************************/

/**
   Verwirft alle Daten und erstellt einen neuen Chunk

   Diese Methode löscht alle Daten im Speicher. Sie sollte
   nur mit Bedacht aufgerufen werden (z. B. in einem Zweig
   nach einem fork(), wobei der andere Zweig die Daten speichert).
*/

void DLSLogger::discard_chunk()
{
#ifdef DEBUG
    msg() << "Discarding chunk. " << _data_size << " bytes written.";
    log(DLSDebug);
#endif

    // Vorgeben, dass noch keine Daten geschrieben wurden
    _data_size = 0;

    // Neuen Saver erstellen (löscht vorher den alten Saver)
    create_gen_saver();

    // Vorgeben, dass noch kein Chunk existiert
    _chunk_created = false;
}

/*****************************************************************************/

/**
   Merkt eine Änderung der Kanalvorgaben vor

   Wenn ein Änderungsbefehl gesendet wurde, die
   Bestätigung aber noch nicht da ist, kann es sein,
   dass noch Daten entsprechend der "alten" Vorgabe empfangen
   werden. Deshalb darf die Vorgabe solange nicht übernommen
   werden, bis die Bestätigung da ist.

   Das passiert mit dieser Methode. Sie merkt die neue Vorgabe vor.
   Sobald eine Bestätigung mit der angegebenen ID
   empfangen wird, muss do_change() aufgerufen werden.

   \param channel Die neue, "wartende" Kanalvorgabe
   \param id Änderungs-ID, die mit dem Änderungsbefehl
   gesendet wurde.
   \see change_is()
   \see do_change()
*/

void DLSLogger::set_change(const COMChannelPreset *channel,
                           const string &id)
{
    if (_change_in_progress)
    {
        msg() << "Change in progress!";
        log(DLSWarning);
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
   Abfrage auf eine bestimmte Änderungs-ID

   Gibt "true" zurück, wenn die Angegebene ID
   auf die vorgemerkte passt und zudem eine Änderung wartet

   \return true, wenn ID passt
*/

bool DLSLogger::change_is(const string &id) const
{
    return _change_id == id && _change_in_progress;
}

/*****************************************************************************/

/**
   Führt eine vorgemerkte Änderung durch

   \throw EDLSLogger Fehler beim Speichern von wartenden Daten
*/

void DLSLogger::do_change()
{
    if (!_change_in_progress) return;

    // Chunks beenden!
    finish();

    // Änderungen übernehmen
    _channel_preset = _change_channel;

    // Saver-Objekt neu erstellen
    create_gen_saver();

    // Jetzt wartet keine Änderung mehr
    _change_in_progress = false;
}

/*****************************************************************************/

/**
   Speichert wartende Daten

   Speichert alle Daten, die noch nicht im Dateisystem sind.

   \throw EDLSLogger Fehler beim Speichern - Datenverlust!
*/

void DLSLogger::finish()
{
    stringstream err;
    bool error = false;

#ifdef DEBUG
    msg() << "logger::finish()";
    log(DLSInfo);
#endif

    try
    {
        // Alle Daten speichern
        if (_gen_saver) _gen_saver->flush();
    }
    catch (EDLSSaver &e)
    {
        error = true;
        err << "saver::flush(): " << e.msg;
    }

#ifdef DEBUG
    msg() << "logger::finish() ready";
    log(DLSInfo);
#endif

    // Chunk beenden
    _chunk_created = false;

    if (error)
    {
        throw EDLSLogger(err.str());
    }

#ifdef DEBUG
    msg() << "logger::finish() returning";
    log(DLSInfo);
#endif

    _finished = true;
}

/*****************************************************************************/
