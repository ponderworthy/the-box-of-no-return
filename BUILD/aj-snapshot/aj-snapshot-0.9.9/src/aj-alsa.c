/************************************************************************/
/* Copyright (C) 2009-2012 Lieven Moors and Jari Suominen               */
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

extern int verbose;
extern int daemon_running;
extern int alsa_success;

void alsa_store_connections( snd_seq_t* seq, const snd_seq_addr_t *addr, mxml_node_t* port_node )
{
    snd_seq_query_subscribe_t *subs;
    snd_seq_query_subscribe_alloca(&subs);
    snd_seq_query_subscribe_set_root(subs, addr);
    snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_READ);
    snd_seq_query_subscribe_set_index(subs, 0);

    snd_seq_client_info_t* connected_cinfo; //to get the names of connected clients
    snd_seq_client_info_alloca(&connected_cinfo);
    snd_seq_port_info_t* connected_pinfo; //to get port capabilities of connected clients
    snd_seq_port_info_alloca(&connected_pinfo);

    const char* client_name;
    char port_id[4];
    unsigned int caps = SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE;

    for (; snd_seq_query_port_subscribers(seq, subs) >= 0; 
        snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1) )
    {
        const snd_seq_addr_t *dest = snd_seq_query_subscribe_get_addr(subs);

        snd_seq_get_any_port_info(seq, dest->client, dest->port, connected_pinfo);

        if ((snd_seq_port_info_get_capability(connected_pinfo) & caps) != caps)
            continue;
        if (snd_seq_port_info_get_capability(connected_pinfo) & SND_SEQ_PORT_CAP_NO_EXPORT)
            continue;

        snd_seq_get_any_client_info(seq, dest->client, connected_cinfo);
        client_name = snd_seq_client_info_get_name(connected_cinfo);

        if(is_ignored_client(client_name)){
            if(verbose) fprintf(stdout, "Ignoring connection to ALSA client %s\n", client_name);
            continue;
        }

        snprintf(port_id, 4, "%i", dest->port);

        mxml_node_t* connection_node;
        connection_node = mxmlNewElement(port_node, "connection");

        mxmlElementSetAttr(connection_node, "client", client_name);
        mxmlElementSetAttr(connection_node, "port", port_id);
    }        
}

void alsa_store_ports( snd_seq_t* seq, snd_seq_client_info_t* cinfo, snd_seq_port_info_t* pinfo, mxml_node_t* client_node )
{
    char port_id[3];
    unsigned int caps = SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ;

    snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
    snd_seq_port_info_set_port(pinfo, -1);

    while (snd_seq_query_next_port(seq, pinfo) >= 0)
    {
        if ((snd_seq_port_info_get_capability(pinfo) & caps) != caps)
            continue;
        if (snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_NO_EXPORT)
            continue;

        int id = snd_seq_port_info_get_port( pinfo );
        snprintf(port_id, 3, "%i", id);

        mxml_node_t* port_node;
        port_node = mxmlNewElement(client_node, "port");
        mxmlElementSetAttr(port_node, "id", port_id);    

        alsa_store_connections(seq, snd_seq_port_info_get_addr(pinfo), port_node);
    }
}

void alsa_store_clients( snd_seq_t* seq, mxml_node_t* alsa_node )
{
    const char* name;

    snd_seq_client_info_t* cinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo); // allocate once, and reuse for all clients

    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq, cinfo) >= 0) 
    {
        if(snd_seq_client_info_get_num_ports(cinfo) == 0)
        continue;

        name = snd_seq_client_info_get_name(cinfo);

        if(is_ignored_client(name)){
            if(verbose) fprintf(stdout, "Ignoring ALSA client %s\n", name);
            continue;
        }

        mxml_node_t* client_node;
        client_node = mxmlNewElement(alsa_node, "client");
        mxmlElementSetAttr(client_node, "name", name);

        alsa_store_ports( seq, cinfo, pinfo, client_node );
    }
}

void alsa_store( snd_seq_t* seq, mxml_node_t* root_node )
{
    mxml_node_t* alsa_node;

    alsa_node = mxmlNewElement(root_node, "alsa");

    alsa_store_clients( seq, alsa_node );

}

void alsa_restore_connections( snd_seq_t* seq, const char* client_name, int port_id, mxml_node_t* port_node)
{
    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_addr_t sender, dest;
    mxml_node_t* connection_node;
    const char* dest_client_name;
    const char* dest_id;
    int dest_port_id;

    connection_node = mxmlFindElement(port_node, port_node, "connection", NULL, NULL, MXML_DESCEND_FIRST);
    
    while (connection_node){
        dest_client_name = mxmlElementGetAttr(connection_node, "client");
        dest_id = mxmlElementGetAttr(connection_node, "port");
        dest_port_id = atoi(dest_id);

        if( !is_ignored_client(dest_client_name) ){
            if (snd_seq_parse_address(seq, &sender, client_name) >= 0) {
                sender.port = port_id;
                if (snd_seq_parse_address(seq, &dest, dest_client_name) >= 0) {
                    dest.port = dest_port_id;
                    snd_seq_port_subscribe_set_sender(subs, &sender);
                    snd_seq_port_subscribe_set_dest(subs, &dest);
                    if (snd_seq_subscribe_port(seq, subs) >= 0) {
                        if(verbose) fprintf(stdout, "Connecting port '%s':%i to '%s':%i\n", 
                            client_name, port_id, dest_client_name, dest_port_id);
                    }
                    else if(verbose && !daemon_running){
                        if (snd_seq_get_port_subscription(seq, subs) == 0) {
                            fprintf(stdout, "Port '%s':%i is already connected to '%s':%i\n", 
                                client_name, port_id, dest_client_name, dest_port_id);
                        }
                        else  { 
                            fprintf(stdout, "Failed to connect port '%s':%i to '%s':%i !\n", 
                                client_name, port_id, dest_client_name, dest_port_id);
                            alsa_success = 0;
                        }
                    }
                }
                else if(verbose && !daemon_running) {
                    fprintf(stdout, "Client '%s' is not active, so failed to subscribe from '%s':%i\n", 
                            dest_client_name, client_name, port_id);
                    alsa_success = 0;
                }
            }
            else if(verbose  && !daemon_running) { 
                fprintf(stdout, "Client '%s' is not active, so failed to subscribe to '%s':%i\n", 
                        client_name, dest_client_name, dest_port_id);
                alsa_success = 0;
            }
        }
        else if(verbose){
            fprintf(stdout, "Ignoring connection to ALSA client %s\n", dest_client_name);
        }

        connection_node = mxmlFindElement(connection_node, port_node, "connection", NULL, NULL, MXML_NO_DESCEND);
    }
}

void alsa_restore_ports( snd_seq_t* seq, const char* client_name, mxml_node_t* client_node)
{
    mxml_node_t* port_node;
    const char* id;
    int port_id;

    port_node = mxmlFindElement(client_node, client_node, "port", NULL, NULL, MXML_DESCEND_FIRST);

    while (port_node)
    {
        id = mxmlElementGetAttr(port_node, "id");
        port_id = atoi(id);

        alsa_restore_connections(seq, client_name, port_id, port_node);

        port_node = mxmlFindElement(port_node, client_node, "port", NULL, NULL, MXML_NO_DESCEND);
    }
}

void alsa_restore_clients( snd_seq_t* seq, mxml_node_t* alsa_node )
{
    mxml_node_t* client_node;
    const char* client_name;

    client_node = mxmlFindElement(alsa_node, alsa_node, "client", NULL, NULL, MXML_DESCEND_FIRST);

    while (client_node)
    {
        client_name = mxmlElementGetAttr(client_node, "name");

        if( !is_ignored_client(client_name) ){
            alsa_restore_ports(seq, client_name, client_node);
        }
        else if(verbose){
            fprintf(stdout, "Ignoring ALSA client %s\n", client_name);
        }
        client_node = mxmlFindElement(client_node, alsa_node, "client", NULL, NULL, MXML_NO_DESCEND);
    }
}

void alsa_restore( snd_seq_t* seq, mxml_node_t* root_node )
{
    mxml_node_t* alsa_node;

    alsa_node = mxmlFindElement(root_node, root_node, "alsa", NULL, NULL, MXML_DESCEND_FIRST);

    alsa_restore_clients( seq, alsa_node );
}

snd_seq_t* alsa_initialize( snd_seq_t* seq )
{
    int client;

    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        fprintf(stderr, "can't open ALSA sequencer\n");
        return NULL;
    }

    if ((client = snd_seq_client_id(seq)) < 0) {
        snd_seq_close(seq);
        fprintf(stderr, "can't get ALSA client id\n");
        return NULL;
    }

    if (snd_seq_set_client_name(seq, "aj-snapshot") < 0) {
        snd_seq_close(seq);
        fprintf(stderr, "can't set ALSA client info\n");
        return NULL;
    }
    return seq;
}

