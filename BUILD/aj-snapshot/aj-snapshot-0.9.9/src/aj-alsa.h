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

#ifndef AJ_ALSA_H
#define AJ_ALSA_H


snd_seq_t* alsa_initialize( snd_seq_t* seq );
void alsa_store( snd_seq_t* seq, mxml_node_t* xml_node );
void alsa_restore( snd_seq_t* seq, mxml_node_t* xml_node );
int alsa_compare_clients ( snd_seq_t* seq, char ***alsa_client_list, unsigned int *acl_size);


#endif
