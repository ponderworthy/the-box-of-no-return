// PresetsUI.h
// generated by Fast Light User Interface Designer (fluid) version 1.0304

#ifndef PresetsUI_h
#define PresetsUI_h
#include <FL/Fl.H>
// Original ZynAddSubFX author Nasca Octavian Paul
// Copyright (C) 2002-2005 Nasca Octavian Paul
// Copyright 2009-2010, Alan Calvert
// Copyright 2017-2020, Will Godfrey

// This file is part of yoshimi, which is free software: you can redistribute
// it and/or modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.

// yoshimi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.   See the GNU General Public License (version 2 or
// later) for more details.

// You should have received a copy of the GNU General Public License along with
// yoshimi; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA.

// This file is a derivative of the ZynAddSubFX original.


#include <FL/fl_ask.H>
#include <string>
#include <unistd.h>
using namespace std;
#include "Params/Presets.h"
#include "Misc/SynthEngine.h"
extern SynthEngine *firstSynth;

class PresetsUI_ {
public:
  virtual void refresh();
  ~PresetsUI_();
};
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>

class PresetsUI {
public:
  PresetsUI(SynthEngine *_synth);
  ~PresetsUI();
  Fl_Double_Window* make_window();
  Fl_Double_Window *copywin;
private:
  inline void cb_copywin_i(Fl_Double_Window*, void*);
  static void cb_copywin(Fl_Double_Window*, void*);
public:
  Fl_Browser *copybrowse;
private:
  inline void cb_copybrowse_i(Fl_Browser*, void*);
  static void cb_copybrowse(Fl_Browser*, void*);
public:
  Fl_Button *copypbutton;
private:
  inline void cb_copypbutton_i(Fl_Button*, void*);
  static void cb_copypbutton(Fl_Button*, void*);
public:
  Fl_Button *copybutton;
private:
  inline void cb_copybutton_i(Fl_Button*, void*);
  static void cb_copybutton(Fl_Button*, void*);
public:
  Fl_Button *copycancel;
private:
  inline void cb_copycancel_i(Fl_Button*, void*);
  static void cb_copycancel(Fl_Button*, void*);
public:
  Fl_Box *copytype;
  Fl_Box *copytypetext;
  Fl_Input *presetname;
private:
  inline void cb_presetname_i(Fl_Input*, void*);
  static void cb_presetname(Fl_Input*, void*);
public:
  Fl_Double_Window *pastewin;
private:
  inline void cb_pastewin_i(Fl_Double_Window*, void*);
  static void cb_pastewin(Fl_Double_Window*, void*);
public:
  Fl_Browser *pastebrowse;
private:
  inline void cb_pastebrowse_i(Fl_Browser*, void*);
  static void cb_pastebrowse(Fl_Browser*, void*);
public:
  Fl_Button *pastepbutton;
private:
  inline void cb_pastepbutton_i(Fl_Button*, void*);
  static void cb_pastepbutton(Fl_Button*, void*);
public:
  Fl_Button *pastebutton;
private:
  inline void cb_pastebutton_i(Fl_Button*, void*);
  static void cb_pastebutton(Fl_Button*, void*);
public:
  Fl_Button *pastecancel;
private:
  inline void cb_pastecancel_i(Fl_Button*, void*);
  static void cb_pastecancel(Fl_Button*, void*);
public:
  Fl_Box *pastetypetext;
  Fl_Box *pastetype;
  Fl_Button *deletepbutton;
private:
  inline void cb_deletepbutton_i(Fl_Button*, void*);
  static void cb_deletepbutton(Fl_Button*, void*);
public:
  void copy(Presets *p);
  void paste(Presets *p,PresetsUI_ *pui);
  void copy(Presets *p,int n);
  void paste(Presets *p,PresetsUI_ *pui,int n);
  void rescan();
  void Hide();
  void presetsRtext();
  Presets *p; 
  PresetsUI_ *pui; 
private:
  SynthEngine *synth; 
  int presetsDW; 
  int presetsDH; 
  int copyW; 
  int pasteW; 
};
#endif