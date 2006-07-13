/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#include <dirent.h>

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

    _progress = new Fl_Progress(10, 50, WIDTH - 20, 25, "Fortschritt");
    _progress->deactivate();

#if 0
    _output_time = new Fl_Output(WIDTH - 60, 85, 50, 25, "Verbleibende Zeit");
    _output_time->deactivate();
    _output_size = new Fl_Output(WIDTH - 60, 120, 50, 25, "Geschätzte Größe");
    _output_size->deactivate();
#endif

    _button_export = new Fl_Button(WIDTH - 90, HEIGHT - 35, 80, 25,
                                   "Exportieren");
    _button_export->callback(_callback, this);

    _button_close = new Fl_Button(WIDTH - 190, HEIGHT - 35, 80, 25,
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
    delete _wnd;
}

/*****************************************************************************/

/**
   Anzeigen des Dialoges
*/

void ViewDialogExport::show(const list<ViewChannel> *channels,
                            COMTime start, COMTime end)
{
    stringstream str;
    list<ViewChannel>::const_iterator channel_i;
    list<ViewChunk *>::const_iterator chunk_i;

    _channels = channels;
    _start = start;
    _end = end;
    _channel_count = _channels->size();


    str << "Exportieren von " << _channel_count
        << (_channel_count == 1 ? " Kanal" : " Kanäle") << endl
        << " von " << _start.to_real_time() << " bis " << _end.to_real_time()
        << ".";
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
    Fl_File_Chooser *chooser;
    list<ViewChannel>::const_iterator channel_i;

    if (_export_finished) {
        _wnd->hide();
        return;
    }

    chooser = new Fl_File_Chooser(".", NULL, Fl_File_Chooser::DIRECTORY,
                                  "Exportieren nach...");
    chooser->show();
    while (chooser->shown()) Fl::wait();

    if (chooser->value()) {
        _export_dir = chooser->value();
        if (pthread_create(&_thread, 0, _static_thread_function, this)) {
            cerr << "Failed to create thread!" << endl;
            return;
        }
    }

    delete chooser;
}

/*****************************************************************************/

void *ViewDialogExport::_static_thread_function(void *data)
{
    ViewDialogExport *dialog = (ViewDialogExport *) data;

    Fl::lock();
    dialog->_thread_running = true;
    dialog->_button_export->deactivate();
    dialog->_progress->maximum(dialog->_channel_count);
    dialog->_progress->value(0);
    dialog->_progress->activate();
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
    dialog->_button_export->activate();
    dialog->_button_export->label("Schliessen");
    dialog->_progress->label("Export abgeschlossen.");
    dialog->_button_close->deactivate();
    Fl::unlock();
    Fl::awake();

    return (void *) NULL;
}

/*****************************************************************************/

void ViewDialogExport::_thread_function()
{
    list<ViewChannel>::const_iterator channel_i;

    for (channel_i = _channels->begin();
         channel_i != _channels->end();
         channel_i++) {

        channel_i->export_data(_start, _end, _export_dir);

        Fl::lock();
        _progress->value(_progress->value() + 1);
        _progress->redraw();
        Fl::unlock();
        Fl::awake();

        if (!_thread_running) break;
    }
}

/*****************************************************************************/
