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

#ifndef CtlDialogChannelH
#define CtlDialogChannelH

/*****************************************************************************/

#include <list>
using namespace std;

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Choice.H>

/*****************************************************************************/

#include "CtlJobPreset.h"

/*****************************************************************************/

/**
   Dialog for changing the properties of channels to be acquired

   This input mask allows the user to set the sampling rate,
   meta-reduction, compression, etc. of a channel.
*/

class CtlDialogChannel
{
public:
    CtlDialogChannel(const string &);
    ~CtlDialogChannel();

    void show(CtlJobPreset *, const list<const LibDLS::ChannelPreset *> *);
    bool updated() const;

private:
    Fl_Double_Window *_wnd;       /**< Dialog box */
    Fl_Return_Button *_button_ok; /**< "OK"-Button */
    Fl_Button *_button_cancel;    /**< "Cancel"-Button */
    Fl_Input *_input_freq;        /**< Input field for the sampling frequency */
    Fl_Input *_input_block;       /**< Input field for the block size */
    Fl_Input *_input_mask;        /**< Input field for the meta mask */
    Fl_Input *_input_red;         /**< Input field for the meta reduction */
    Fl_Choice *_choice_format;    /**< Selection box for compression */
    Fl_Choice *_choice_mdct;      /**< Selection box for the MDCT block size */
    Fl_Input *_input_accuracy;    /**< Input field for the MDCT accuracy */

    string _dls_dir; /**< DLS data directory */
    CtlJobPreset *_job; /**< Pointer to the job object */
    const list<const LibDLS::ChannelPreset *> *_channels; /**< List of channel
                                                        to be changed */
    bool _updated; /**< Channels have changed */
    bool _choice_format_selected; /**< A format has been selected */
    bool _choice_mdct_selected; /**< A MDCT block size was chosen */

    static void _callback(Fl_Widget *, void *);
    void _button_ok_clicked();
    void _button_cancel_clicked();
    void _choice_format_changed();
    void _choice_mdct_changed();

    void _load_channel();
    bool _save_channels();
};

/*****************************************************************************/

/**
   Return whether channels have changed

   \return true, if channels have been changed
*/

inline bool CtlDialogChannel::updated() const
{
    return _updated;
}

/*****************************************************************************/

#endif
