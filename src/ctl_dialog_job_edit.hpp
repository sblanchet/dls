/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef CTLDialogJobEditHpp
#define CTLDialogJobEditHpp

/*****************************************************************************/

#include <FL/Fl_Double_Window.h>
#include <FL/Fl_Return_Button.h>
#include <FL/Fl_Input.h>
#include <FL/Fl_Choice.h>

/*****************************************************************************/

#include "ctl_job_preset.hpp"

/*****************************************************************************/

/**
   Dialog zum Editieren der Grunddaten eines Messauftrages
*/

class CTLDialogJobEdit
{
public:
    CTLDialogJobEdit(const string &);
    ~CTLDialogJobEdit();

    void show(CTLJobPreset *);
    bool updated();

private:
    Fl_Double_Window *_wnd; /**< Dialogfenster */
    Fl_Return_Button *_button_ok; /**< Bestätigungs-Button */
    Fl_Button *_button_cancel; /**< Button zum Abbrechen */
    Fl_Input *_input_desc; /**< Eingabefeld für die Beschreibung */
    Fl_Input *_input_source; /**< Eingabefeld für die Datenquelle */
    Fl_Input *_input_trigger; /**< Eingabefeld für den Trigger-Parameter */
    Fl_Input *_input_quota_time; /**< Eingabefeld für Zeit-Quota */
    Fl_Choice *_choice_time_ext; /**< Auswahlfenster für die Erweiterung
                                    der Zeit-Quota */
    Fl_Input *_input_quota_size; /**< Eingabefeld für Daten-Quota */
    Fl_Choice *_choice_size_ext; /**< Auswahlfenster für die Erweiterung
                                    der Daten-Quota */

    string _dls_dir;    /**< DLS-Datenverzeichnis */
    CTLJobPreset *_job; /**< Zeiger auf den Messauftrag */
    bool _updated;      /**< Wurde etwas geändert? */

    static void _callback(Fl_Widget *, void *);
    void _button_ok_clicked();
    void _button_cancel_clicked();
    bool _save_job();
    bool _create_job();
    bool _get_new_id(int *);
    void _display_time_quota();
    void _display_size_quota();
    bool _calc_time_quota(long long *);
    bool _calc_size_quota(long long *);
};

/*****************************************************************************/

/**
   Gibt zurück, ob sich etwas geändert hat

   \return true, wenn Daten geändert wurden
*/

inline bool CTLDialogJobEdit::updated()
{
    return _updated;
}

/*****************************************************************************/

#endif
