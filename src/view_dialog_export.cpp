/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include <FL/Fl.h>
#include <FL/Fl_File_Chooser.h>

/*****************************************************************************/

#include "view_globals.hpp"
#include "view_dialog_export.hpp"

/*****************************************************************************/

#define WIDTH 600
#define HEIGHT 230

/*****************************************************************************/

typedef struct
{
    ViewDialogExport *dialog;
    double channel_percentage;
    double channel_factor;
}
ExportInfo;

/*****************************************************************************/

/**
   Konstruktor
*/

ViewDialogExport::ViewDialogExport(const string &dls_dir
                                   /**< DLS-Datenverzeichnis */
                                   )
{
    int x = Fl::w() / 2 - WIDTH / 2;
    int y = Fl::h() / 2 - HEIGHT / 2;

    _dls_dir = dls_dir;

    _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Ansicht Exportieren");
    _wnd->callback(_callback, this);
    _wnd->set_modal();

    _box_info = new Fl_Box(10, 10, WIDTH - 20, 25);


#if 0
    _output_time = new Fl_Output(WIDTH - 60, 85, 50, 25, "Verbleibende Zeit");
    _output_time->deactivate();
    _output_size = new Fl_Output(WIDTH - 60, 120, 50, 25, "Geschätzte Größe");
    _output_size->deactivate();
#endif

    _check_ascii = new Fl_Check_Button(10, HEIGHT - 130, 240, 25,
                                       "Matlab ASCII (.dat)");
    _check_mat4 = new Fl_Check_Button(10, HEIGHT - 105, 240, 25,
                                      "Matlab binary, level 4 (.mat)");

    _progress = new Fl_Progress(10, HEIGHT - 70, WIDTH - 20, 25, "0%");
    _progress->maximum(100);
    _progress->value(0);
    _progress->deactivate();

    _button_export = new Fl_Button(WIDTH - 130, HEIGHT - 35, 120, 25,
                                   "Exportieren");
    _button_export->callback(_callback, this);

    _button_close = new Fl_Button(WIDTH - 220, HEIGHT - 35, 80, 25,
                                  "Abbrechen");
    _button_close->callback(_callback, this);

    _thread_running = false;
    _export_finished = false;
}

/*****************************************************************************/

/**
   Destruktor
*/

ViewDialogExport::~ViewDialogExport()
{
    _clear_exporters();
    delete _wnd;
}

/*****************************************************************************/

/**
 */

void ViewDialogExport::_clear_exporters()
{
    list<Export *>::iterator exp_i;

    for (exp_i = _exporters.begin(); exp_i != _exporters.end(); exp_i++) {
        delete *exp_i;
    }

    _exporters.clear();
}

/*****************************************************************************/

/**
   Anzeigen des Dialoges
*/

void ViewDialogExport::show(const list<Channel> *channels,
                            COMTime start, COMTime end)
{
    stringstream str;
    list<ViewChannel>::const_iterator channel_i;
    list<Chunk>::const_iterator chunk_i;

    _channels = channels;
    _start = start;
    _end = end;
    _channel_count = _channels->size();


    str << "Exportieren von " << _channel_count
        << (_channel_count == 1 ? " Kanal" : " Kanälen") << endl
        << _start.to_real_time() << " bis " << _end.to_real_time() << endl
        << "(" << _start.diff_str_to(_end) << ")";
    _box_info->copy_label(str.str().c_str());

    _wnd->show();

    while (_wnd->shown()) Fl::wait();
}

/*****************************************************************************/

/**
   Statische Callback-Funktion

   \param sender Widget, dass den Callback ausgelöst hat
   \param data Zeiger auf den Dialog
*/

void ViewDialogExport::_callback(Fl_Widget *sender, void *data)
{
    ViewDialogExport *dialog = (ViewDialogExport *) data;

    if (sender == dialog->_button_close) dialog->_button_close_clicked();
    if (sender == dialog->_wnd) dialog->_button_close_clicked();
    if (sender == dialog->_button_export) dialog->_button_export_clicked();
}

/*****************************************************************************/

/**
   Callback: Der "Schliessen"-Button wurde geklickt
*/

void ViewDialogExport::_button_close_clicked()
{
    if (_thread_running) {
        _thread_running = false;
        Fl::unlock();
        pthread_join(_thread, NULL);
        Fl::lock();
    }

    _wnd->hide();
}

/*****************************************************************************/

/**
   Callback: Der "Export"-Button wurde geklickt
*/

void ViewDialogExport::_button_export_clicked()
{
    list<ViewChannel>::const_iterator channel_i;
    string env_export, env_export_fmt;
    char *env;
    stringstream info_file_name;
    ofstream info_file;
    COMTime now;
    Export *exporter;

    if (_export_finished) {
        _wnd->hide();
        return;
    }

    now.set_now();

    if ((env = getenv("DLS_EXPORT"))) env_export = env;
    else env_export = ".";

    if ((env = getenv("DLS_EXPORT_FMT"))) env_export_fmt = env;
    else env_export_fmt = "dls-export-%Y-%m-%d-%H-%M-%S";

    _export_dir += env_export + "/" + now.format_time(env_export_fmt.c_str());

    cout << "Exporting to \"" << _export_dir << "\"." << endl;

    // create unique directory
    if (mkdir(_export_dir.c_str(), 0755)) {
        cerr << "ERROR: Failed to create export directory: ";
        cerr << strerror(errno) << endl;
        return;
    }

    // create info file
    info_file_name << _export_dir << "/dls_export_info";
    info_file.open(info_file_name.str().c_str(), ios::trunc);

    if (!info_file.is_open()) {
        cerr << "Failed to write \"" << info_file_name.str() << "\"!" << endl;
        return;
    }

    info_file << endl
              << "This is a DLS export directory." << endl << endl
              << "Exported on: "
              << now.to_rfc811_time() << endl << endl
              << "Exported range from: "
              << _start.to_rfc811_time() << endl
              << "                 to: "
              << _end.to_rfc811_time() << endl << endl;

    info_file.close();

    _clear_exporters();

    if (_check_ascii->value()) {
        try {
            exporter = new ExportAscii();
        }
        catch (...) {
            cerr << "Failed to allocate Ascii-Exporter." << endl;
            return;
        }
        _exporters.push_back(exporter);
    }

    if (_check_mat4->value()) {
        try {
            exporter = new ExportMat4();
        }
        catch (...) {
            cerr << "Failed to allocate Mat4-Exporter." << endl;
            return;
        }
        _exporters.push_back(exporter);
    }

    if (pthread_create(&_thread, 0, _static_thread_function, this)) {
        cerr << "Failed to create thread!" << endl;
        return;
    }
}

/*****************************************************************************/

void *ViewDialogExport::_static_thread_function(void *data)
{
    ViewDialogExport *dialog = (ViewDialogExport *) data;

    Fl::lock();
    dialog->_thread_running = true;
    dialog->_button_export->deactivate();
    dialog->_progress->activate();
    dialog->_check_ascii->deactivate();
    dialog->_check_mat4->deactivate();
#if 0
    dialog->_output_time->activate();
    dialog->_output_size->activate();
#endif
    Fl::unlock();
    Fl::awake();

    dialog->_thread_function();

    Fl::lock();
    dialog->_thread_running = false;
    dialog->_export_finished = true;
#if 0
    dialog->_output_time->deactivate();
    dialog->_output_size->deactivate();
#endif
    dialog->_button_close->deactivate();
    dialog->_button_export->label("Schliessen");
    dialog->_button_export->activate();
    Fl::unlock();
    Fl::awake();

    cout << "Export finished." << endl;

    return (void *) NULL;
}

/*****************************************************************************/

/**
 */

int ViewDialogExport::_export_data_callback(Data *data, void *cb_data)
{
    ExportInfo *info = (ExportInfo *) cb_data;
    list<Export *>::iterator exp_i;
    double percentage;
    int int_perc;
    double diff_time;
    stringstream progress;

    for (exp_i = info->dialog->_exporters.begin();
         exp_i != info->dialog->_exporters.end();
         exp_i++)
        (*exp_i)->data(data);

    diff_time = (data->end_time() - info->dialog->_start).to_dbl();
    percentage = info->channel_percentage + diff_time * info->channel_factor;
    int_perc = (int) (percentage + 0.5);
    progress << int_perc << "%";

    Fl::lock();
    info->dialog->_progress->copy_label(progress.str().c_str());
    info->dialog->_progress->value(int_perc);
    info->dialog->_progress->redraw();
    Fl::unlock();
    Fl::awake();

    return 0; // not adopted
}

/*****************************************************************************/

void ViewDialogExport::_thread_function()
{
    list<Channel>::const_iterator channel_i;
    list<Export *>::iterator exp_i;
    ExportInfo info;
    unsigned int current_channel, total_channels;
    stringstream progress;
    int int_perc;

    current_channel = 0;
    total_channels = _channels->size();

    if (!total_channels) {
        _clear_exporters();
        return;
    }

    info.dialog = this;
    info.channel_percentage = 0.0;
    info.channel_factor = 100.0 / total_channels / (_end - _start).to_dbl();

    for (channel_i = _channels->begin();
         channel_i != _channels->end();
         channel_i++) {

        for (exp_i = _exporters.begin(); exp_i != _exporters.end(); exp_i++)
            (*exp_i)->begin(*channel_i, _export_dir);

        channel_i->fetch_data(_start, _end, 0, _export_data_callback, &info);

        for (exp_i = _exporters.begin(); exp_i != _exporters.end(); exp_i++)
            (*exp_i)->end();

        current_channel++;
        info.channel_percentage = 100.0 * current_channel / total_channels;
        int_perc = (int) (info.channel_percentage + 0.5);

        progress.clear();
        progress.str("");
        progress << int_perc << "%";

        Fl::lock();
        _progress->copy_label(progress.str().c_str());
        _progress->value(int_perc);
        _progress->redraw();
        Fl::unlock();
        Fl::awake();

        if (!_thread_running) break;
    }

    _clear_exporters();
}

/*****************************************************************************/
