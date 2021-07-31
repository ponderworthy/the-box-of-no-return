/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2009 Grigor Iliev                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include <AudioUnit/AudioUnit.r>
#include <PluginAUVersion.h>

#define kAudioUnitResID_LinuxSamplerUnit 1000

#define RES_ID			kAudioUnitResID_LinuxSamplerUnit
#define COMP_TYPE		LINUXSAMPLER_COMP_TYPE
#define COMP_SUBTYPE		LINUXSAMPLER_COMP_SUBTYPE
#define COMP_MANUF		LINUXSAMPLER_COMP_MANUF

#define VERSION			kPluginAUVersion
#define NAME			"LinuxSampler: LinuxSampler Unit"
#define DESCRIPTION		"LinuxSampler AU plugin"
#define ENTRY_POINT		"PluginAUEntry"

#include "AUResources.r"
