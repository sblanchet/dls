//---------------------------------------------------------------
//
//  C T L _ D I A L O G _ C H A N N E L S . C P P
//
//---------------------------------------------------------------

#include <sys/socket.h>
#include <netdb.h>

#include <iostream>
#include <sstream>
using namespace std;

#include <FL/Fl.h>
#include <FL/fl_ask.h>

//---------------------------------------------------------------

#include "com_xml_parser.hpp"
#include "com_ring_buffer_t.hpp"
#include "ctl_globals.hpp"
#include "ctl_dialog_channels.hpp"

//---------------------------------------------------------------

#define WIDTH 550
#define HEIGHT 350

//---------------------------------------------------------------

CTLDialogChannels::CTLDialogChannels(const string &source)
{
  int x = Fl::w() / 2 - WIDTH / 2;
  int y = Fl::h() / 2 - HEIGHT / 2;

  _source = source;
  _thread_running = false;

  _wnd = new Fl_Double_Window(x, y, WIDTH, HEIGHT, "Kanäle auswählen");
  _wnd->set_modal();
  _wnd->callback(_callback, this);

  _button_ok = new Fl_Button(WIDTH - 90, HEIGHT - 35, 80, 25, "OK");
  _button_ok->deactivate();
  _button_ok->callback(_callback, this);
  _button_cancel = new Fl_Button(WIDTH - 180, HEIGHT - 35, 80, 25, "Abbrechen");
  _button_cancel->callback(_callback, this);

  _grid_channels = new Fl_Grid(10, 10, WIDTH - 20, HEIGHT - 55);
  _grid_channels->add_column("name", "Kanal", 200);
  _grid_channels->add_column("unit", "Einheit", 50);
  _grid_channels->add_column("freq", "Abtastrate (Hz)");
  _grid_channels->select_mode(flgMultiSelect);
  _grid_channels->callback(_callback, this);
  _grid_channels->hide();

  _box_message = new Fl_Box(10, 10, WIDTH - 20, HEIGHT - 55);
  _box_message->align(FL_ALIGN_CENTER);
  _box_message->label("Empfange Kanäle...");

  _wnd->resizable(_grid_channels);
}

//---------------------------------------------------------------

CTLDialogChannels::~CTLDialogChannels()
{
  delete _wnd;
}

//---------------------------------------------------------------

void CTLDialogChannels::_callback(Fl_Widget *sender, void *data)
{
  CTLDialogChannels *dialog = (CTLDialogChannels *) data;

  if (sender == dialog->_grid_channels) dialog->_grid_channels_callback();
  if (sender == dialog->_button_ok) dialog->_button_ok_clicked();
  if (sender == dialog->_button_cancel) dialog->_button_cancel_clicked();
  if (sender == dialog->_wnd) dialog->_button_cancel_clicked();
}

//---------------------------------------------------------------

void CTLDialogChannels::_grid_channels_callback()
{
  unsigned int i;
  stringstream str;

  switch (_grid_channels->current_event())
  {
    case flgContent:
      i = _grid_channels->current_record();

      if (_grid_channels->current_col() == "name")
      {
        _grid_channels->current_content(_channels[i].name());
      }
      else if (_grid_channels->current_col() == "unit")
      {
        _grid_channels->current_content(_channels[i].unit());
      }
      else if (_grid_channels->current_col() == "freq")
      {
        str << _channels[i].frequency();
        _grid_channels->current_content(str.str());
      }
      break;

    case flgSelect:
      _button_ok->activate();
      break;

    case flgDeSelect:
      _button_ok->deactivate();
      break;

    case flgDoubleClick:
      _button_ok_clicked();
      break;

    default:
      break;
  }
}

//---------------------------------------------------------------

void CTLDialogChannels::_button_ok_clicked()
{
  list<unsigned int>::const_iterator sel_i;

  _selected.clear();

  sel_i = _grid_channels->selected_list()->begin();
  while (sel_i != _grid_channels->selected_list()->end())
  {
    _selected.push_back(_channels[*sel_i]);
    sel_i++;
  }

  _wnd->hide();
}

//---------------------------------------------------------------

void CTLDialogChannels::_button_cancel_clicked()
{
  if (_thread_running)
  {
    cout << "cancelling thread..." << endl;
    pthread_cancel(_thread);
  }

  _selected.clear();
  _wnd->hide();
}

//---------------------------------------------------------------

void CTLDialogChannels::show()
{
  _error = "";

  if (pthread_create(&_thread, 0, _static_thread_function, this) == 0)
  {
    _wnd->show();

    while (_wnd->shown()) Fl::wait();
  }
  else
  {
    fl_alert("error: could not start thread!");
  }
}

//---------------------------------------------------------------

const list<COMRealChannel> *CTLDialogChannels::channels() const
{
  return &_selected;
}

//---------------------------------------------------------------

void *CTLDialogChannels::_static_thread_function(void *data)
{
  CTLDialogChannels *dialog = (CTLDialogChannels *) data;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);

  Fl::lock();
  dialog->_thread_running = true;
  Fl::unlock();

  dialog->_thread_function();

  Fl::lock();
  dialog->_thread_running = false;
  dialog->_thread_finished();
  Fl::unlock();

  return 0;
}

//---------------------------------------------------------------

void CTLDialogChannels::_thread_function()
{
  int socket;
  struct sockaddr_in address;
  struct hostent *hp;
  fd_set read_fds, write_fds;
  int select_ret, recv_ret, send_ret;
  COMXMLParser xml;
  const COMXMLTag *tag;
  COMRealChannel channel;
  string to_send;
  COMRingBufferT<char, unsigned int> ring(10000);
  char *write_pointer;
  unsigned int write_size;
  bool exit_thread = false;

  // Socket öffnen
  if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    _error = "could not create socket!";
    return;
  }

  // Socket geöffnet, Adresse übersetzen/auflösen
  address.sin_family = AF_INET;
  if ((hp = gethostbyname(_source.c_str())) == NULL)
  {
    _error = "could not resolve address \"" + _source + "\"!";
    cout << _error << endl;
    close(socket);
    return;
  }

  // Adresse in sockaddr-Struktur kopieren
  memcpy((char *) &address.sin_addr, (char *) hp->h_addr, hp->h_length);
  address.sin_port = htons(MSRD_PORT);
    
  // Verbinden
  if ((::connect(socket, (struct sockaddr *) &address, sizeof(address))) == -1)
  {
    _error = "could not connect to \"" + _source + "\"!";
    close(socket);
    return;
  }

  to_send = "<rk>\n";

  while (!exit_thread)
  {
    // File-Descriptor-Sets nullen und mit Client-FD vorbesetzen
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(socket, &read_fds);
    if (to_send.length() > 0) FD_SET(socket, &write_fds);

    // Warten auf Änderungen oder Timeout
    if ((select_ret = select(socket + 1, &read_fds, &write_fds, 0, 0)) > 0)
    {
      // Eingehende Daten?
      if (FD_ISSET(socket, &read_fds))
      {
        ring.write_info(&write_pointer, &write_size); 

	// Daten abholen...
        if ((recv_ret = recv(socket, write_pointer, write_size, 0)) > 0)
        {
          ring.written(recv_ret);
          
          while (1)
          {
            try
            {      
              tag = xml.parse(&ring);
            }
            catch (ECOMXMLParserEOF &e) // Tag noch nicht komplett
            {
              break;
            }
            catch (ECOMXMLParser &e) // Anderer Parsing-Fehler
            {
              continue;
            }

            try
            {
              if (tag->title() == "channel")
              {
                try
                {
                  channel.read_from_tag(tag);
                  _channels.push_back(channel);
                }
                catch (ECOMRealChannel &e)
                {
                  _error = "reading channel: " + e.msg;
                  exit_thread = true;
                  break;
                }
              }
              else if (tag->title() == "channels" && tag->type() == dxttEnd)
              {
                exit_thread = true;
                break;
              }
            }
            catch (ECOMXMLTag &e)
            {
              _error = "parser: " + e.msg;
              exit_thread = true;
              break;
            }
          }
        }
        else if (recv_ret == -1)
        {
          _error = "error in recv()";
          break;
        }
      }

      // Bereit zum Senden?
      if (FD_ISSET(socket, &write_fds))
      {
        // Daten senden
        if ((send_ret = send(socket, to_send.c_str(), to_send.length(), 0)) > 0)
        {
          to_send.erase(0, send_ret); // Gesendetes entfernen     
        }
        else if (send_ret == -1)
        {
          _error = "error in send()";
          break;
        }
      }
    }
	
    // Select-Fehler
    else if (select_ret == -1)
    {
      _error = "error in select()";
      break;
    }
  }

  close(socket);
}

//---------------------------------------------------------------

void CTLDialogChannels::_thread_finished()
{
  if (_channels.size() > 0)
  {
    _grid_channels->record_count(_channels.size());

    _box_message->hide();
    _grid_channels->show();
    _grid_channels->take_focus();
  }
  else if (_error != "")
  {
    _box_message->labelcolor(FL_RED);
    _box_message->label(_error.c_str());
  }
}

//---------------------------------------------------------------
