/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2014 Christian Schoenebeck                              *
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

#ifndef __LSCP_H__
#define __LSCP_H__

//TODO: obvious ;) all error and warning codes have to be defined

// Error Codes

#define LSCP_ERR_UNKNOWN		0  ///< unknown error type


// Warning Codes

#define LSCP_WRN_UNKNOWN		0  ///< unknown warning type


// LSCP Shell Codes

#define LSCP_SHU_COMPLETE		0
#define LSCP_SHU_SYNTAX_ERR		1
#define LSCP_SHU_INCOMPLETE		2

#define LSCP_SHD_NO_MATCH	0
#define LSCP_SHD_MATCH		1

// LSCP Shell Keywords

#define LSCP_SHK_GOOD_FRONT		"{{GF}}"
#define LSCP_SHK_CURSOR			"{{CU}}"
#define LSCP_SHK_SUGGEST_BACK	"{{SB}}"
#define LSCP_SHK_POSSIBILITIES_BACK "{{PB}}"
#define LSCP_SHK_EXPECT_MULTI_LINE  "SHE:MLINE"

#endif // __LSCP_H__
