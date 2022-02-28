// Copyright 2011, Alan Calvert
// Copyright 2015-2020, Will Godfrey

// This file is part of yoshimi, which is free software: you can
// redistribute it and/or modify it under the terms of the GNU General
// Public License as published by the Free Software Foundation, either
// version 2 of the License, or (at your option) any later version.

// yoshimi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.


// generated by Fast Light User Interface Designer (fluid) version 1.0304

#ifndef ConsoleUI_h
#define ConsoleUI_h
#include <FL/Fl.H>
#include <set>
#include <string>
#include "FL/Fl_Text_Display.H"
#include "UI/MiscGui.h"
using namespace std;

class ConsoleTextDisplay : public Fl_Text_Display {
public:
  ConsoleTextDisplay(int x, int y, int w, int h, char* l = 0) ;
  void scroll_to_last_line();
};
#include <FL/Fl_Double_Window.H>

class ConsoleUI {
public:
  Fl_Double_Window* make_window(void);
  Fl_Double_Window *logConsole;
private:
  inline void cb_logConsole_i(Fl_Double_Window*, void*);
  static void cb_logConsole(Fl_Double_Window*, void*);
public:
  ConsoleTextDisplay *logText;
  ConsoleUI(SynthEngine *_synth);
  ~ConsoleUI();
  void log(string msg);
  void Show(SynthEngine *synth);
  void consoleRtext();
private:
  Fl_Text_Buffer *txtbuf; 
  int bufsize; 
  float logDW; 
  float logDH; 
  SynthEngine *synth; 
  int lastlogW; 
};
#endif