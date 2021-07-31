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
extern int jack_dirty;
extern pthread_mutex_t registration_callback_lock;
extern pthread_mutex_t shutdown_callback_lock;
extern int jack_success;


void jack_port_registration (jack_port_id_t port, int reg, void *arg) {
    /* This callback is called when new ports appear. */
    if(reg){
        pthread_mutex_lock( &registration_callback_lock );
        jack_dirty++;
        pthread_mutex_unlock( &registration_callback_lock );
    }
}

void jack_shutdown (void * jackc) {
    fprintf(stdout, "aj-snapshot: Jack server has been shut down.\n");

    jack_client_t** tmp;
    tmp = (jack_client_t**)jackc;

    pthread_mutex_lock( &shutdown_callback_lock );
    *tmp = NULL;
    pthread_mutex_unlock( &shutdown_callback_lock );    

}


void jack_initialize( jack_client_t** jackc, int callbacks_on )
{
    int result;
    jack_options_t options = JackNoStartServer;
    *jackc = jack_client_open("aj-snapshot", options, NULL);

    if (*jackc == NULL) {
        if (verbose && !daemon_running) fprintf(stderr, "aj-snapshot: Jack server is not running.\n");
        return;
    }

    jack_on_shutdown(*jackc, jack_shutdown, jackc);

    if (callbacks_on) {
        result = jack_set_port_registration_callback(*jackc, jack_port_registration, 0);
        if(result < 0) fprintf(stderr, "aj-snapshot: Failed to set the port registration callback.\n");
        if (jack_activate (*jackc)) {
            fprintf (stderr, "aj-snapshot: Jack server seems to be running but is not responding.\n");
            jack_client_close(*jackc); 
            *jackc = NULL;
            return;
        }
    }
    
    pthread_mutex_lock( &registration_callback_lock );
    jack_dirty = 1;
    pthread_mutex_unlock( &registration_callback_lock );

    if (verbose && daemon_running) fprintf(stdout, "aj-snapshot: Jack server was started.\n");

    return;
}

void jack_restore_connections( jack_client_t** jackc, const char* client_name, const char* port_name, mxml_node_t* port_node )
{
    mxml_node_t* connection_node;
    const char* dest_port;
    unsigned int s = strlen(client_name) + strlen(port_name) + 2;
    char src_port[s];
    char tmp_str[jack_port_name_size()];
    char* dest_client_name;

    snprintf(src_port, s, "%s:%s", client_name, port_name);

    connection_node = mxmlFindElement(port_node, port_node, "connection", NULL, NULL, MXML_DESCEND_FIRST);

    while (connection_node){
        dest_port = mxmlElementGetAttr(connection_node, "port");

        strcpy(tmp_str, dest_port); // dest_port is 'const char*'
        dest_client_name = strtok(tmp_str, ":");

        if(!is_ignored_client(dest_client_name)){
            int err;

            pthread_mutex_lock( &shutdown_callback_lock );
            if (*jackc == NULL) {
                err = ENOENT;
            } else {
                err = jack_connect(*jackc, src_port, dest_port);
            }
            pthread_mutex_unlock( &shutdown_callback_lock );

            if(verbose){
                if (err == 0) {
                    fprintf(stdout, "Connecting port '%s' with '%s'\n", src_port, dest_port);
                } 
                else if (!daemon_running) {
                    if (err == EEXIST) {
                        fprintf(stdout, "Port '%s' is already connected to '%s'\n", src_port, dest_port);
                    } 
                    else {
                        fprintf(stdout, "Failed to connect port '%s' to '%s'!\n", src_port, dest_port);
                        jack_success = 0;
                    }
                }
            }
        }
        else if(verbose && !daemon_running){
            fprintf(stdout, "Ignoring connection to JACK client: %s\n", dest_client_name);
        }
        connection_node = mxmlFindElement(connection_node, port_node, "connection", NULL, NULL, MXML_NO_DESCEND);
    }
}

void jack_restore_ports( jack_client_t** jackc, const char* client_name, mxml_node_t* client_node)
{
    mxml_node_t* port_node;
    const char* port_name;

    port_node = mxmlFindElement(client_node, client_node, "port", NULL, NULL, MXML_DESCEND_FIRST);

    while (port_node){
        port_name = mxmlElementGetAttr(port_node, "name");
        jack_restore_connections(jackc, client_name, port_name, port_node);
        port_node = mxmlFindElement(port_node, client_node, "port", NULL, NULL, MXML_NO_DESCEND);
    }
}

void jack_restore_clients( jack_client_t** jackc, mxml_node_t* jack_node )
{
    mxml_node_t* client_node;
    const char* client_name;

    client_node = mxmlFindElement(jack_node, jack_node, "client", NULL, NULL, MXML_DESCEND_FIRST);

    while (client_node){
        client_name = mxmlElementGetAttr(client_node, "name");

        if( !is_ignored_client(client_name) ){
            jack_restore_ports(jackc, client_name, client_node);
        }
        else if(verbose  && !daemon_running){
                fprintf(stdout, "Ignoring JACK client %s\n", client_name);
        }
        client_node = mxmlFindElement(client_node, jack_node, "client", NULL, NULL, MXML_NO_DESCEND);
    }
}

void jack_restore( jack_client_t** jackc, mxml_node_t* root_node )
{
   
    mxml_node_t* jack_node;
    jack_node = mxmlFindElement(root_node, root_node, "jack", NULL, NULL, MXML_DESCEND_FIRST);
    jack_restore_clients(jackc, jack_node);
}

void jack_store_connections( jack_client_t* jackc, const char* port_name, mxml_node_t* port_node )
{

    mxml_node_t* connection_node;

    jack_port_t* port = jack_port_by_name(jackc, port_name);
    const char** connected_ports = jack_port_get_all_connections(jackc, port);

    if(connected_ports != NULL)
    {
        unsigned int i = 0;
        const char* connected_port = connected_ports[i];
        char tmp_str[jack_port_name_size()];
        char* dest_client_name;

        while (connected_port) {
            strcpy(tmp_str, connected_port); // otherwise we change the names of ports in jack !!
            dest_client_name = strtok(tmp_str, ":");

            if(!is_ignored_client(dest_client_name)){
                connection_node = mxmlNewElement(port_node, "connection");
                mxmlElementSetAttr(connection_node, "port", connected_port);
            } 
            else if(verbose){
                    fprintf(stdout, "Ignoring connection to JACK client: %s\n", dest_client_name);
            }
            connected_port = connected_ports[++i];
        }
    }
}

void jack_store( jack_client_t* jackc, mxml_node_t* root_node )
{
    mxml_node_t* jack_node = mxmlNewElement(root_node, "jack");
    mxml_node_t* client_node = NULL;
    mxml_node_t* port_node = NULL;

    char tmp_str[jack_port_name_size()];
    char client_name_prev[jack_port_name_size()];
    const char* client_name;
    char* port_name;

    if (jackc == NULL) return;

    const char **jack_output_ports = jack_get_ports(jackc, NULL, NULL, JackPortIsOutput);

    if (jack_output_ports == NULL) return;

    unsigned int i = 0;
    const char* full_name = jack_output_ports[i];

    while (full_name) {
        strcpy(tmp_str, full_name); // otherwise we change the names of ports in jack !!

        client_name = strtok(tmp_str, ":");
        port_name = strtok(NULL, "\0");

        if( !is_ignored_client(client_name) ){
            if ( strcmp(client_name_prev, client_name) ){
                client_node = mxmlNewElement(jack_node, "client");
                mxmlElementSetAttr(client_node, "name", client_name);
                strcpy(client_name_prev, client_name);
            }
            port_node = mxmlNewElement(client_node, "port");
            mxmlElementSetAttr(port_node, "name", port_name);
            jack_store_connections(jackc, full_name, port_node);
        }
        else {
            if( strcmp(client_name_prev, client_name) ){
                if(verbose) 
                    fprintf(stdout, "Ignoring JACK client: %s\n", client_name);
                strcpy(client_name_prev, client_name);
            }
        }

        full_name = jack_output_ports[++i];
    }
}
