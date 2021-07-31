/************************************************************************/
/* Copyright (C) 2009 Lieven Moors                                      */
/*                                                                      */
/* This file is part of aj-snapshot.                                    */
/*                                                                      */
/* aj-snapshot is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* aj-snapshot is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with aj-snapshot.  If not, see <http://www.gnu.org/licenses/>. */
/************************************************************************/

#ifndef AJ_FILE_H
#define AJ_FILE_H

const char* xml_whitespace_cb( mxml_node_t *node, int where );
mxml_node_t* read_xml( const char* filename, mxml_node_t* xml );
int write_xml( const char* filename, mxml_node_t* xml, int force );

#endif
