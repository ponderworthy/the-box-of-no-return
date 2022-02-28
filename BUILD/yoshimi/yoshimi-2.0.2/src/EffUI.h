// EffUI.h
// generated by Fast Light User Interface Designer (fluid) version 1.0304

#ifndef EffUI_h
#define EffUI_h
#include <FL/Fl.H>
// Original ZynAddSubFX author Nasca Octavian Paul
// Copyright (C) 2002-2005 Nasca Octavian Paul
// Copyright 2009-2010, Alan Calvert
// Copyright 2016-2021, Will Godfrey & others

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

// This file is a derivative of the ZynAddSubFX original


#include <string>
#include <iostream>
#include "UI/MiscGui.h"
#include "UI/WidgetPDial.h"
#include "EnvelopeUI.h"
#include "FilterUI.h"
#include "Misc/SynthEngine.h"
#include "Effects/EffectMgr.h"
#include "PresetsUI.h"

class EQGraph : public Fl_Box {
public:
  EQGraph(int x,int y, int w, int h, const char *label=0);
  void init(EffectMgr *eff_);
  void draw_freq_line(float freq, int type);
  void draw();
  int getresponse(int maxy,float freq);
  float getfreqx(float x);
  float getfreqpos(float freq);
private:
  int oldx,oldy; 
public:
  float khzval; 
private:
  EffectMgr *eff; 
  int maxdB; 
  SynthEngine *synth; 
};
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>

class EffUI : public Fl_Group,public PresetsUI_ {
public:
  EffUI(int x,int y, int w, int h, const char *label=0);
  ~EffUI();
  Fl_Group* make_null_window();
  Fl_Group *effnullwindow;
  Fl_Text_Display *noeffect;
  Fl_Group* make_reverb_window();
  Fl_Group *effreverbwindow;
  Fl_Text_Display *revname;
  Fl_Choice *revp;
private:
  inline void cb_revp_i(Fl_Choice*, void*);
  static void cb_revp(Fl_Choice*, void*);
public:
  Fl_Choice *revp10;
private:
  inline void cb_revp10_i(Fl_Choice*, void*);
  static void cb_revp10(Fl_Choice*, void*);
public:
  WidgetPDial *revp0;
private:
  inline void cb_revp0_i(WidgetPDial*, void*);
  static void cb_revp0(WidgetPDial*, void*);
public:
  WidgetPDial *revp1;
private:
  inline void cb_revp1_i(WidgetPDial*, void*);
  static void cb_revp1(WidgetPDial*, void*);
public:
  WidgetPDial *revp2;
private:
  inline void cb_revp2_i(WidgetPDial*, void*);
  static void cb_revp2(WidgetPDial*, void*);
public:
  WidgetPDial *revp3;
private:
  inline void cb_revp3_i(WidgetPDial*, void*);
  static void cb_revp3(WidgetPDial*, void*);
public:
  WidgetPDial *revp4;
private:
  inline void cb_revp4_i(WidgetPDial*, void*);
  static void cb_revp4(WidgetPDial*, void*);
public:
  WidgetPDial *revp12;
private:
  inline void cb_revp12_i(WidgetPDial*, void*);
  static void cb_revp12(WidgetPDial*, void*);
public:
  WidgetPDial *revp6;
private:
  inline void cb_revp6_i(WidgetPDial*, void*);
  static void cb_revp6(WidgetPDial*, void*);
public:
  WidgetPDial *revp7;
private:
  inline void cb_revp7_i(WidgetPDial*, void*);
  static void cb_revp7(WidgetPDial*, void*);
public:
  WidgetPDial *revp8;
private:
  inline void cb_revp8_i(WidgetPDial*, void*);
  static void cb_revp8(WidgetPDial*, void*);
public:
  WidgetPDial *revp9;
private:
  inline void cb_revp9_i(WidgetPDial*, void*);
  static void cb_revp9(WidgetPDial*, void*);
public:
  WidgetPDial *revp11;
private:
  inline void cb_revp11_i(WidgetPDial*, void*);
  static void cb_revp11(WidgetPDial*, void*);
public:
  Fl_Group* make_echo_window();
  Fl_Group *effechowindow;
  Fl_Text_Display *echoname;
  Fl_Choice *echop;
private:
  inline void cb_echop_i(Fl_Choice*, void*);
  static void cb_echop(Fl_Choice*, void*);
public:
  WidgetPDial *echop0;
private:
  inline void cb_echop0_i(WidgetPDial*, void*);
  static void cb_echop0(WidgetPDial*, void*);
public:
  WidgetPDial *echop1;
private:
  inline void cb_echop1_i(WidgetPDial*, void*);
  static void cb_echop1(WidgetPDial*, void*);
public:
  WidgetPDial *echop2;
private:
  inline void cb_echop2_i(WidgetPDial*, void*);
  static void cb_echop2(WidgetPDial*, void*);
public:
  WidgetPDial *echop3;
private:
  inline void cb_echop3_i(WidgetPDial*, void*);
  static void cb_echop3(WidgetPDial*, void*);
public:
  WidgetPDial *echop4;
private:
  inline void cb_echop4_i(WidgetPDial*, void*);
  static void cb_echop4(WidgetPDial*, void*);
public:
  WidgetPDial *echop5;
private:
  inline void cb_echop5_i(WidgetPDial*, void*);
  static void cb_echop5(WidgetPDial*, void*);
public:
  WidgetPDial *echop6;
private:
  inline void cb_echop6_i(WidgetPDial*, void*);
  static void cb_echop6(WidgetPDial*, void*);
public:
  Fl_Group* make_chorus_window();
  Fl_Group *effchoruswindow;
  Fl_Text_Display *chorusname;
  Fl_Choice *chorusp;
private:
  inline void cb_chorusp_i(Fl_Choice*, void*);
  static void cb_chorusp(Fl_Choice*, void*);
public:
  Fl_Choice *chorusp4;
private:
  inline void cb_chorusp4_i(Fl_Choice*, void*);
  static void cb_chorusp4(Fl_Choice*, void*);
public:
  WidgetPDial *chorusp0;
private:
  inline void cb_chorusp0_i(WidgetPDial*, void*);
  static void cb_chorusp0(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp1;
private:
  inline void cb_chorusp1_i(WidgetPDial*, void*);
  static void cb_chorusp1(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp2;
private:
  inline void cb_chorusp2_i(WidgetPDial*, void*);
  static void cb_chorusp2(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp3;
private:
  inline void cb_chorusp3_i(WidgetPDial*, void*);
  static void cb_chorusp3(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp5;
private:
  inline void cb_chorusp5_i(WidgetPDial*, void*);
  static void cb_chorusp5(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp6;
private:
  inline void cb_chorusp6_i(WidgetPDial*, void*);
  static void cb_chorusp6(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp7;
private:
  inline void cb_chorusp7_i(WidgetPDial*, void*);
  static void cb_chorusp7(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp8;
private:
  inline void cb_chorusp8_i(WidgetPDial*, void*);
  static void cb_chorusp8(WidgetPDial*, void*);
public:
  WidgetPDial *chorusp9;
private:
  inline void cb_chorusp9_i(WidgetPDial*, void*);
  static void cb_chorusp9(WidgetPDial*, void*);
public:
  Fl_Check_Button2 *chorusflange;
private:
  inline void cb_chorusflange_i(Fl_Check_Button2*, void*);
  static void cb_chorusflange(Fl_Check_Button2*, void*);
public:
  Fl_Check_Button2 *chorusp11;
private:
  inline void cb_chorusp11_i(Fl_Check_Button2*, void*);
  static void cb_chorusp11(Fl_Check_Button2*, void*);
public:
  Fl_Group* make_phaser_window();
  Fl_Group *effphaserwindow;
  Fl_Text_Display *phasername;
  Fl_Choice *phaserp;
private:
  inline void cb_phaserp_i(Fl_Choice*, void*);
  static void cb_phaserp(Fl_Choice*, void*);
public:
  Fl_Choice *phaserp4;
private:
  inline void cb_phaserp4_i(Fl_Choice*, void*);
  static void cb_phaserp4(Fl_Choice*, void*);
public:
  WidgetPDial *phaserp0;
private:
  inline void cb_phaserp0_i(WidgetPDial*, void*);
  static void cb_phaserp0(WidgetPDial*, void*);
public:
  WidgetPDial *phaserp1;
private:
  inline void cb_phaserp1_i(WidgetPDial*, void*);
  static void cb_phaserp1(WidgetPDial*, void*);
public:
  WidgetPDial *phaserp2;
private:
  inline void cb_phaserp2_i(WidgetPDial*, void*);
  static void cb_phaserp2(WidgetPDial*, void*);
public:
  WidgetPDial *phaserp3;
private:
  inline void cb_phaserp3_i(WidgetPDial*, void*);
  static void cb_phaserp3(WidgetPDial*, void*);
public:
  WidgetPDial *phaserp5;
private:
  inline void cb_phaserp5_i(WidgetPDial*, void*);
  static void cb_phaserp5(WidgetPDial*, void*);
public:
  WidgetPDial *phaserp6;
private:
  inline void cb_phaserp6_i(WidgetPDial*, void*);
  static void cb_phaserp6(WidgetPDial*, void*);
public:
  WidgetPDial *phaserp7;
private:
  inline void cb_phaserp7_i(WidgetPDial*, void*);
  static void cb_phaserp7(WidgetPDial*, void*);
public:
  Fl_Counter *phaserp8;
private:
  inline void cb_phaserp8_i(Fl_Counter*, void*);
  static void cb_phaserp8(Fl_Counter*, void*);
public:
  WidgetPDial *phaserp9;
private:
  inline void cb_phaserp9_i(WidgetPDial*, void*);
  static void cb_phaserp9(WidgetPDial*, void*);
public:
  Fl_Check_Button2 *phaserp10;
private:
  inline void cb_phaserp10_i(Fl_Check_Button2*, void*);
  static void cb_phaserp10(Fl_Check_Button2*, void*);
public:
  WidgetPDial *phaserp11;
private:
  inline void cb_phaserp11_i(WidgetPDial*, void*);
  static void cb_phaserp11(WidgetPDial*, void*);
public:
  Fl_Check_Button2 *phaserp12;
private:
  inline void cb_phaserp12_i(Fl_Check_Button2*, void*);
  static void cb_phaserp12(Fl_Check_Button2*, void*);
public:
  WidgetPDial *phaserp13;
private:
  inline void cb_phaserp13_i(WidgetPDial*, void*);
  static void cb_phaserp13(WidgetPDial*, void*);
public:
  Fl_Check_Button2 *phaserp14;
private:
  inline void cb_phaserp14_i(Fl_Check_Button2*, void*);
  static void cb_phaserp14(Fl_Check_Button2*, void*);
public:
  Fl_Group* make_alienwah_window();
  Fl_Group *effalienwahwindow;
  Fl_Text_Display *alienname;
  Fl_Choice *awp;
private:
  inline void cb_awp_i(Fl_Choice*, void*);
  static void cb_awp(Fl_Choice*, void*);
public:
  Fl_Choice *awp4;
private:
  inline void cb_awp4_i(Fl_Choice*, void*);
  static void cb_awp4(Fl_Choice*, void*);
public:
  WidgetPDial *awp0;
private:
  inline void cb_awp0_i(WidgetPDial*, void*);
  static void cb_awp0(WidgetPDial*, void*);
public:
  WidgetPDial *awp1;
private:
  inline void cb_awp1_i(WidgetPDial*, void*);
  static void cb_awp1(WidgetPDial*, void*);
public:
  WidgetPDial *awp2;
private:
  inline void cb_awp2_i(WidgetPDial*, void*);
  static void cb_awp2(WidgetPDial*, void*);
public:
  WidgetPDial *awp3;
private:
  inline void cb_awp3_i(WidgetPDial*, void*);
  static void cb_awp3(WidgetPDial*, void*);
public:
  WidgetPDial *awp5;
private:
  inline void cb_awp5_i(WidgetPDial*, void*);
  static void cb_awp5(WidgetPDial*, void*);
public:
  WidgetPDial *awp6;
private:
  inline void cb_awp6_i(WidgetPDial*, void*);
  static void cb_awp6(WidgetPDial*, void*);
public:
  WidgetPDial *awp7;
private:
  inline void cb_awp7_i(WidgetPDial*, void*);
  static void cb_awp7(WidgetPDial*, void*);
public:
  WidgetPDial *awp9;
private:
  inline void cb_awp9_i(WidgetPDial*, void*);
  static void cb_awp9(WidgetPDial*, void*);
public:
  WidgetPDial *awp10;
private:
  inline void cb_awp10_i(WidgetPDial*, void*);
  static void cb_awp10(WidgetPDial*, void*);
public:
  Fl_Counter *awp8;
private:
  inline void cb_awp8_i(Fl_Counter*, void*);
  static void cb_awp8(Fl_Counter*, void*);
public:
  Fl_Group* make_distorsion_window();
  Fl_Group *effdistorsionwindow;
  Fl_Text_Display *distname;
  Fl_Choice *distp;
private:
  inline void cb_distp_i(Fl_Choice*, void*);
  static void cb_distp(Fl_Choice*, void*);
public:
  Fl_Choice *distp5;
private:
  inline void cb_distp5_i(Fl_Choice*, void*);
  static void cb_distp5(Fl_Choice*, void*);
public:
  WidgetPDial *distp0;
private:
  inline void cb_distp0_i(WidgetPDial*, void*);
  static void cb_distp0(WidgetPDial*, void*);
public:
  WidgetPDial *distp1;
private:
  inline void cb_distp1_i(WidgetPDial*, void*);
  static void cb_distp1(WidgetPDial*, void*);
public:
  WidgetPDial *distp2;
private:
  inline void cb_distp2_i(WidgetPDial*, void*);
  static void cb_distp2(WidgetPDial*, void*);
public:
  WidgetPDial *distp3;
private:
  inline void cb_distp3_i(WidgetPDial*, void*);
  static void cb_distp3(WidgetPDial*, void*);
public:
  WidgetPDial *distp4;
private:
  inline void cb_distp4_i(WidgetPDial*, void*);
  static void cb_distp4(WidgetPDial*, void*);
public:
  WidgetPDial *distp7;
private:
  inline void cb_distp7_i(WidgetPDial*, void*);
  static void cb_distp7(WidgetPDial*, void*);
public:
  WidgetPDial *distp8;
private:
  inline void cb_distp8_i(WidgetPDial*, void*);
  static void cb_distp8(WidgetPDial*, void*);
public:
  Fl_Check_Button2 *distp6;
private:
  inline void cb_distp6_i(Fl_Check_Button2*, void*);
  static void cb_distp6(Fl_Check_Button2*, void*);
public:
  Fl_Check_Button2 *distp9;
private:
  inline void cb_distp9_i(Fl_Check_Button2*, void*);
  static void cb_distp9(Fl_Check_Button2*, void*);
public:
  Fl_Check_Button2 *distp10;
private:
  inline void cb_distp10_i(Fl_Check_Button2*, void*);
  static void cb_distp10(Fl_Check_Button2*, void*);
public:
  Fl_Group* make_eq_window();
  Fl_Group *effeqwindow;
  Fl_Text_Display *eqname;
  WidgetPDial *eqp0;
private:
  inline void cb_eqp0_i(WidgetPDial*, void*);
  static void cb_eqp0(WidgetPDial*, void*);
public:
  Fl_Counter *bandcounter;
private:
  inline void cb_bandcounter_i(Fl_Counter*, void*);
  static void cb_bandcounter(Fl_Counter*, void*);
public:
  Fl_Group *bandgroup;
  WidgetPDial *freqdial;
private:
  inline void cb_freqdial_i(WidgetPDial*, void*);
  static void cb_freqdial(WidgetPDial*, void*);
public:
  WidgetPDial *gaindial;
private:
  inline void cb_gaindial_i(WidgetPDial*, void*);
  static void cb_gaindial(WidgetPDial*, void*);
public:
  WidgetPDial *qdial;
private:
  inline void cb_qdial_i(WidgetPDial*, void*);
  static void cb_qdial(WidgetPDial*, void*);
public:
  Fl_Counter *stagescounter;
private:
  inline void cb_stagescounter_i(Fl_Counter*, void*);
  static void cb_stagescounter(Fl_Counter*, void*);
public:
  Fl_Choice *typechoice;
private:
  inline void cb_typechoice_i(Fl_Choice*, void*);
  static void cb_typechoice(Fl_Choice*, void*);
public:
  EQGraph *eqgraph;
  Fl_Group* make_dynamicfilter_window();
  Fl_Group *effdynamicfilterwindow;
  Fl_Text_Display *dfname;
  Fl_Choice *dfp;
private:
  inline void cb_dfp_i(Fl_Choice*, void*);
  static void cb_dfp(Fl_Choice*, void*);
public:
  Fl_Choice *dfp4;
private:
  inline void cb_dfp4_i(Fl_Choice*, void*);
  static void cb_dfp4(Fl_Choice*, void*);
public:
  WidgetPDial *dfp0;
private:
  inline void cb_dfp0_i(WidgetPDial*, void*);
  static void cb_dfp0(WidgetPDial*, void*);
public:
  WidgetPDial *dfp1;
private:
  inline void cb_dfp1_i(WidgetPDial*, void*);
  static void cb_dfp1(WidgetPDial*, void*);
public:
  WidgetPDial *dfp2;
private:
  inline void cb_dfp2_i(WidgetPDial*, void*);
  static void cb_dfp2(WidgetPDial*, void*);
public:
  WidgetPDial *dfp3;
private:
  inline void cb_dfp3_i(WidgetPDial*, void*);
  static void cb_dfp3(WidgetPDial*, void*);
public:
  WidgetPDial *dfp5;
private:
  inline void cb_dfp5_i(WidgetPDial*, void*);
  static void cb_dfp5(WidgetPDial*, void*);
public:
  WidgetPDial *dfp6;
private:
  inline void cb_dfp6_i(WidgetPDial*, void*);
  static void cb_dfp6(WidgetPDial*, void*);
public:
  Fl_Button *filter;
private:
  inline void cb_filter_i(Fl_Button*, void*);
  static void cb_filter(Fl_Button*, void*);
public:
  WidgetPDial *dfp7;
private:
  inline void cb_dfp7_i(WidgetPDial*, void*);
  static void cb_dfp7(WidgetPDial*, void*);
public:
  WidgetPDial *dfp9;
private:
  inline void cb_dfp9_i(WidgetPDial*, void*);
  static void cb_dfp9(WidgetPDial*, void*);
public:
  Fl_Check_Button2 *dfp8;
private:
  inline void cb_dfp8_i(Fl_Check_Button2*, void*);
  static void cb_dfp8(Fl_Check_Button2*, void*);
public:
  Fl_Double_Window* make_filter_window();
  Fl_Double_Window *filterwindow;
private:
  inline void cb_filterwindow_i(Fl_Double_Window*, void*);
  static void cb_filterwindow(Fl_Double_Window*, void*);
public:
  FilterUI *fwin_filterui;
  Fl_Button *filterclose;
private:
  inline void cb_filterclose_i(Fl_Button*, void*);
  static void cb_filterclose(Fl_Button*, void*);
public:
  void send_data(int action, int control, float value, int group, int type);
  float fetchData(float value, int control, int part, int kititem = UNUSED, int engine = UNUSED, int insert = UNUSED, int parameter = UNUSED, int offset = UNUSED, int miscmsg = UNUSED, int request = UNUSED);
  void returns_update(CommandBlock *getData);
  void init(EffectMgr *eff_, int npart_, int neff_);
  void refresh(EffectMgr *eff_, int npart_, int neff_);
  void refresh();
  void UpdatePresetColour(int changed, int efftype);
  void effRtext(float dScale, int efftype);
  void Showfilt();
  void filtRtext();
  void EQbandUpdate();
private:
  EffectMgr *eff; 
  SynthEngine *synth; 
  int eqband; 
  int npart; 
  int neff; 
  float filtDW; 
  float filtDH; 
  int lastfiltW; 
};
#endif