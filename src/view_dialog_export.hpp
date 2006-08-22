/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef ViewDialogExportHpp
#define ViewDialogExportHpp

/*****************************************************************************/

#include <pthread.h>

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Check_Button.h>
#include <FL/Fl_Progress.h>
#include <FL/Fl_Output.h>
#include <FL/Fl_Box.h>

#include "lib_export.hpp"

#include "view_channel.hpp"

/*****************************************************************************/

/**
   Export-Dialog des DLS-Viewers
*/

class ViewDialogExport
{
public:
    ViewDialogExport(const string &);
    ~ViewDialogExport();

    void show(const list<Channel> *, COMTime, COMTime);

private:
    string _dls_dir;           /**< DLS-Datenverzeichnis */
    Fl_Double_Window *_wnd;    /**< Dialogfenster */
    Fl_Box *_box_info; /**< Info-Zeile */
#if 0
    Fl_Output *_output_time; /**< Verbleibende Zeit */
    Fl_Output *_output_size; /**< Geschätzte Größe */
#endif
    Fl_Check_Button *_check_ascii; /**< export to ASCII */
    Fl_Check_Button *_check_mat4; /**< export to MATLAB level 4 file */
    Fl_Progress *_progress; /**< Fortschrittsanzeige */
    Fl_Button *_button_export; /**< Export-Button */
    Fl_Button *_button_close;  /**< "Schliessen"-Button */

    const list<Channel> *_channels;
    unsigned int _channel_count;
    COMTime _start, _end;
    list<Export *> _exporters;
    string _export_dir;
    bool _export_finished;
    pthread_t _thread; /**< Export-Thread */
    bool _thread_running; /**< true, wenn der Thread läuft */

    static void _callback(Fl_Widget *, void *);
    void _button_close_clicked();
    void _button_export_clicked();
    void _set_progress_value(int);

    static void *_static_thread_function(void *);
    void _thread_function();

    static int _export_data_callback(Data *, void *);
    void _clear_exporters();
};

/*****************************************************************************/

#endif
