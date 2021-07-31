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

#include "aj-snapshot.h"

void alsa_remove_connection(snd_seq_t* seq, snd_seq_port_info_t* pinfo)
{
	snd_seq_query_subscribe_t *query;
	snd_seq_query_subscribe_alloca(&query);
	snd_seq_query_subscribe_set_root(query, snd_seq_port_info_get_addr(pinfo));
	snd_seq_query_subscribe_set_type(query, SND_SEQ_QUERY_SUBS_READ);
	snd_seq_query_subscribe_set_index(query, 0);

	while(snd_seq_query_port_subscribers(seq, query) == 0)
	{
		snd_seq_port_info_t *port;
		snd_seq_port_info_alloca(&port);

		const snd_seq_addr_t *sender = snd_seq_query_subscribe_get_root(query);
		const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(query);

		if ((snd_seq_get_any_port_info(seq, dest->client, dest->port, port) < 0) ||
			!(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_SUBS_WRITE) ||
			(snd_seq_port_info_get_capability(port) & SND_SEQ_PORT_CAP_NO_EXPORT))
		{
			snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1);
			continue;
		}

		snd_seq_port_subscribe_t *subs;
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_queue(subs, snd_seq_query_subscribe_get_queue(query));
		snd_seq_port_subscribe_set_sender(subs, sender);
		snd_seq_port_subscribe_set_dest(subs, dest);

		if(snd_seq_unsubscribe_port(seq, subs) < 0){
			snd_seq_query_subscribe_set_index(query, snd_seq_query_subscribe_get_index(query) + 1);
		}
	}
}

void alsa_remove_connections(snd_seq_t *seq)
{
	snd_seq_client_info_t *cinfo;
	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_t *pinfo;
	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_set_client(cinfo, -1);

	while (snd_seq_query_next_client(seq, cinfo) >= 0) 
	{
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);

		while (snd_seq_query_next_port(seq, pinfo) >= 0)
		{
			alsa_remove_connection(seq, pinfo);
		}
	}
}

void jack_remove_connections(jack_client_t* jackc)
{
	const char **jack_output_ports = jack_get_ports(jackc, NULL, NULL, JackPortIsOutput);
	const char **jack_input_ports = jack_get_ports(jackc, NULL, NULL, JackPortIsInput);
	unsigned int i , j;

	if (jack_output_ports == NULL || jack_input_ports == NULL)
		return;

	for (i = 0; jack_output_ports[i] != NULL; ++i) {
		for (j = 0; jack_input_ports[j] != NULL; ++j) {
			jack_disconnect(jackc, jack_output_ports[i], jack_input_ports[j]);
		}
	}
}

