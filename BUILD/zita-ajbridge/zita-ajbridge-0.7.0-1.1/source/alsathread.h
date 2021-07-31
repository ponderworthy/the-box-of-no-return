// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012-2016 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#ifndef __ALSATHREAD_H
#define __ALSATHREAD_H


#include <zita-alsa-pcmi.h>
#include <jack/jack.h>
#include "pxthread.h"
#include "lfqueue.h"


class Alsathread : public Pxthread
{
public:

    enum { INIT, WAIT, PROC, TERM };
    enum { PLAY, CAPT };

    Alsathread (Alsa_pcmi  *alsadev, int mode);
    virtual ~Alsathread (void);
    virtual void thr_main (void);

    int start (Lfq_audio *audioq, Lfq_int32 *commq, Lfq_adata *alsaq, int rtprio);

private:

    void send (int k, double t);
    int capture (void);
    int playback (void);

    double modtime (double d)
    {
	if (d < -_jtmod / 2) d += _jtmod;
	if (d >  _jtmod / 2) d -= _jtmod;
	return d;
    }

    double jacktime (jack_time_t t)
    {
	int32_t u = (int32_t)(t & 0x0FFFFFFFLL); 
	if (u & 0x08000000) u -= 0x10000000;
	return 1e-6 * u;
    }
    
    Alsa_pcmi    *_alsadev;
    int           _mode;
    int           _state;
    int           _fsize;
    Lfq_audio    *_audioq;
    Lfq_int32    *_commq;
    Lfq_adata    *_alsaq;
    bool          _first;
    double        _jtmod;
    double        _t0;
    double        _t1;
    double        _dt;
    double        _w1;
    double        _w2;
};


#endif
