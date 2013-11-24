#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#define OMXPLAYER_DBUS_NAME "org.mpris.MediaPlayer2.omxplayer"
#define OMXPLAYER_DBUS_PATH_SERVER "/org/mpris/MediaPlayer2"  
#define OMXPLAYER_DBUS_INTERFACE_ROOT "org.mpris.MediaPlayer2"
#define OMXPLAYER_DBUS_INTERFACE_PLAYER "org.mpris.MediaPlayer2.Player"

#include "OMXThread.h"
#include "OMXControl.h"
#include <map>

 class Keyboard : public OMXThread
 {
 protected:
  struct termios orig_termios;
  int orig_fl;
  DBusConnection *conn;
  std::map<int,int> m_keymap;
  OMXControl *m_omxcontrol;
 public:
  Keyboard();
  ~Keyboard();
  void Close();
  void Process();
  void setKeymap(std::map<int,int> keymap);
  void setConsoleControl(OMXControl * ctrl);
  void Sleep(unsigned int dwMilliSeconds);
 private:
  int poll_char();
  void restore_term();
  void send_action(int action);
  int dbus_connect();
  void dbus_disconnect();
 };

#endif
