/************************************************************************/
/* Copyright (C) 2009-2011 Lieven Moors and Jari Suominen               */
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


static void usage(void)
{
    fprintf(stdout, " -------------------------------------------------------------------------------------\n");
    fprintf(stdout, " aj-snapshot: Store/restore JACK and/or ALSA midi connections to/from an xml file     \n");
    fprintf(stdout, "              Copyright (C) 2009-2012 Lieven Moors and Jari Suominen                  \n");
    fprintf(stdout, "                                                                                      \n");
    fprintf(stdout, " Without options -a or -j, all actions apply to both ALSA and JACK connections        \n");
    fprintf(stdout, " Without option -r, and with 'file', aj-snapshot will store connections to file.      \n");
    fprintf(stdout, "                                                                                      \n");
    fprintf(stdout, " Usage: aj-snapshot [-options] [-p milliseconds] [-i client_name] [file]              \n");
    fprintf(stdout, "                                                                                      \n");
    fprintf(stdout, "  -a,--alsa     Only store/restore ALSA midi connections.                             \n");
    fprintf(stdout, "  -j,--jack     Only store/restore JACK audio and midi connections.                   \n");
    fprintf(stdout, "  -r,--restore  Restore ALSA and/or JACK connections.                                 \n");
    fprintf(stdout, "  -d,--daemon   Restore ALSA and/or JACK connections until terminated.                \n");
    fprintf(stdout, "  -f,--force    Don't ask before overwriting an existing file.                        \n");
    fprintf(stdout, "  -i,--ignore   Specify a name of a client you want to ignore.                        \n");
    fprintf(stdout, "                Note: You can ignore multiple clients by repeating this option.       \n");
    fprintf(stdout, "  -p,--poll     Specify a value in ms that should be used as polling interval         \n");
    fprintf(stdout, "  -q,--quiet    Be quiet about what happens when storing/restoring connections.       \n");
    fprintf(stdout, "  -x,--remove   With 'file': remove all ALSA and/or JACK connections before restoring.\n");
    fprintf(stdout, "  -x,--remove   Without 'file': only remove ALSA and/or JACK connections.             \n");
    fprintf(stdout, "                                                                                      \n");
} 

enum sys {
    NONE, ALSA, JACK, ALSA_JACK // Bitwise OR of ALSA and JACK is ALSA_JACK
};

enum act {
    REMOVE_ONLY, STORE, RESTORE, DAEMON
};

static const struct option long_option[] = {
    {"help", 0, NULL, 'h'},
    {"alsa", 0, NULL, 'a'},
    {"jack", 0, NULL, 'j'},
    {"restore", 0, NULL, 'r'},
    {"daemon", 0, NULL, 'd'},
    {"remove", 0, NULL, 'x'},
    {"force", 0, NULL, 'f'},
    {"ignore", 1, NULL, 'i'},
    {"quiet", 0, NULL, 'q'},
    {"poll", 1, NULL, 'p'},
    {NULL, 0, NULL, 0}
};

// Nasty globals

// First two are used in signal handlers, so need to be protected with sig_atomic_t.
volatile sig_atomic_t daemon_running = 0; // volatile to prevent compiler optimization in while loop
sig_atomic_t reload_xml = 0;

int verbose = 1;
int jack_dirty = 0;
int ic_n = 0; // number of ignored clients.
char *ignored_clients[IGNORED_CLIENTS_MAX]; // array to store names of ignored clients
int alsa_success = 1; 
int jack_success = 1; 

pthread_mutex_t registration_callback_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shutdown_callback_lock = PTHREAD_MUTEX_INITIALIZER;

//////////////////////

int is_ignored_client(const char *name) // function to check if string is name of ignored client.
{
    int i;
    for(i = 0; i < ic_n; i++){
        if (fnmatch(ignored_clients[i], name, 0) == 0) return 1;
    }
    return 0;
}

void exit_cli(int sig) {
    if (verbose) {
        fprintf(stdout, "\raj-snapshot: Exiting!\n");
    }
    daemon_running = 0;
}

void reload_xml_file(int sig) {
    reload_xml = 1;
}


int main(int argc, char **argv)
{ 
    int c;
    int try_remove = 0;
    int remove_connections = 0;
    int force = 0;
    enum sys system = NONE; // The system(s) we asked for.
    enum sys system_ready = NONE; // When we asked for a system and got it (bitwise or'd).
    enum act action = STORE;
    static const char *filename;
    snd_seq_t* seq = NULL;
    jack_client_t* jackc = NULL;
    mxml_node_t* xml_node = NULL;
    mxml_node_t* root_node = NULL;
    int exit_success = 1;
    unsigned int polling_interval_ms = 1000;

    struct pollfd *pfds = NULL; // To check for new clients or ports in ALSA.
    int npfds = 0;
    int alsa_dirty = 1;

    while ((c = getopt_long(argc, argv, "ajrdxfi:p:qh", long_option, NULL)) != -1) {

        switch (c){

        case 'a':
            system |= ALSA;
            break;
        case 'j':
            system |= JACK;
            break;
        case 'r':
            action = RESTORE;
            break;
        case 'd':
            action = DAEMON;
            break;
        case 'x':
            try_remove = 1;
            break;
        case 'f':
            force = 1;
            break;
        case 'i':
            if( ic_n < (IGNORED_CLIENTS_MAX - 1) )
                ignored_clients[ic_n++] = optarg;
            else {
                fprintf(stderr, "aj-snapshot: ERROR: you have more then %i ignored clients\n", IGNORED_CLIENTS_MAX);
                exit(EXIT_FAILURE);
            }
            break;
        case 'p':
            polling_interval_ms = atoi(optarg);
            break;
        case 'q':
            verbose = 0;
            break;
        case 'h':
            usage();
            return 1;
        default:
            usage();
            return 1;
        }
    }

    if (system == NONE) system = ALSA_JACK; // Default when no specific system has been specified.

    if (argc == (optind + 1)) { // If something is left after parsing options, it must be the file
        filename = argv[optind];
    }
    else if ((argc == optind) && try_remove){ 
        action = REMOVE_ONLY; // To make sure action is not STORE or RESTORE
    }
    else {
        fprintf(stderr, " -------------------------------------------------------------------\n");
        fprintf(stderr, " aj-snapshot: Please specify one file to store/restore the snapshot,\n");
        fprintf(stderr, " or use option -x to remove connections only\n");
        usage();
        return 1;
    }
	
    if(try_remove){
        if (action != STORE){
            remove_connections = 1;
        }
        else {
            fprintf(stderr, "aj-snapshot: Will not remove connections before storing connections\n");
        }
    }

    if (action == DAEMON) {
        struct sigaction sig_int_handler;
        struct sigaction sig_hup_handler;

        sig_int_handler.sa_handler = exit_cli;
        sigemptyset(&sig_int_handler.sa_mask);
        sig_int_handler.sa_flags = 0;
        sig_hup_handler.sa_handler = reload_xml_file;
        sigemptyset(&sig_hup_handler.sa_mask);
        sig_hup_handler.sa_flags = 0;

        sigaction(SIGINT, &sig_int_handler, NULL);
        sigaction(SIGTERM, &sig_int_handler, NULL);
        sigaction(SIGHUP, &sig_hup_handler, NULL);
    }

    // Get XML node first:

    switch (action){
        case STORE:
            xml_node = mxmlNewXML("1.0");
            root_node = mxmlNewElement(xml_node, "aj-snapshot");
            break;
        case RESTORE:
        case DAEMON:
            xml_node = read_xml(filename, xml_node);
            root_node = mxmlFindElement(xml_node, xml_node, "aj-snapshot", NULL, NULL, MXML_DESCEND_FIRST);
            if(!root_node){ // Still read older aj-snapshot files, that didn't have the aj-snapshot root node.
                root_node = xml_node;
            }
            break;
        default:
            break;
    }

    // Initialize clients with ALSA and JACK.

    if ((system & ALSA) == ALSA) {
        seq = alsa_initialize(seq);
        if (seq){
            system_ready |= ALSA;
        } 
        else {
            switch (action){
                case STORE:
                    VERBOSE("aj-snapshot: will NOT store ALSA connections!\n");
                    break;
                case RESTORE:
                case DAEMON:
                    VERBOSE("aj-snapshot: will NOT restore ALSA connections!\n");
                    break;
                case REMOVE_ONLY:
                    VERBOSE("aj-snapshot: will NOT remove ALSA connections!\n");
                    break;
                default:
                    break;
            }
            exit_success = 0;
        }
    }
    if ((system & JACK) == JACK) {
        jack_initialize(&jackc, (action == DAEMON));
        if(jackc){
            system_ready |= JACK;
        } 
        else {
            switch (action){
                case STORE:
                    VERBOSE("aj-snapshot: will NOT store JACK connections!\n");
                    break;
                case RESTORE:
                case DAEMON:
                    VERBOSE("aj-snapshot: will NOT restore JACK connections!\n");
                    break;
                case REMOVE_ONLY:
                    VERBOSE("aj-snapshot: will NOT remove JACK connections!\n");
                    break;
                default:
                    break;
            }
            exit_success = 0;
        }
    }

    // We set daemon_running here because in initialize_jack, we don't want it
    // to be on (because we check it to see if we have to say that the JACK server
    // was started while the daemon was running). And when restoring after this, 
    // we want to be less verbose, and daemon_running should be on.
    // This is the result of sharing the first restore with daemon mode.

    if(action == DAEMON) daemon_running = 1;

    // STORE/RESTORE CONNECTIONS

    if ((system_ready & ALSA) == ALSA) {
        if (remove_connections){
            alsa_remove_connections(seq);
            if(verbose && !daemon_running) 
                fprintf(stdout, "aj-snapshot: all ALSA connections removed.\n");
        } 
        switch (action){
            case STORE:
                alsa_store(seq, root_node);
                VERBOSE("aj-snapshot: ALSA connections stored!\n");
                break;
            case DAEMON:
                alsa_restore(seq, root_node);
                break;
            case RESTORE:
                alsa_restore(seq, root_node);
                if(verbose){
                    if(alsa_success){ 
                        fprintf(stdout, "aj-snapshot: ALSA connections restored!\n");
                    } else {
                        fprintf(stdout, "aj-snapshot: all ALSA connections could not be restored!\n");
                        exit_success = 0;
                    }
                }
                break;
            default:
                break;
        }
    }

    if ((system_ready & JACK) == JACK) {
        if (remove_connections){
            jack_remove_connections(jackc);
            if(verbose && !daemon_running) 
                fprintf(stdout, "aj-snapshot: all JACK connections removed.\n");
        }
        switch (action){
            case STORE:
                jack_store(jackc, root_node);
                VERBOSE("aj-snapshot: JACK connections stored!\n");
                break;
            case DAEMON:
                jack_restore(&jackc, root_node);
                break;
            case RESTORE:
                jack_restore(&jackc, root_node);
                if(verbose){
                    if(jack_success){ 
                        fprintf(stdout, "aj-snapshot: JACK connections restored!\n");
                    } else {
                        fprintf(stdout, "aj-snapshot: all JACK connections could not be restored!\n");
                        exit_success = 0;
                    }
                }
                break;
            default:
                break;
        }
    }

    // Write file when storing:
    if(action == STORE){
        write_xml(filename, xml_node, force);
    }

    if(action == DAEMON) {
        // Run Daemon
        // initialize poll file descriptors if we use ALSA
        // and connect a port to the system announce port.
        if ((system_ready & ALSA) == ALSA) {
            int err;
            err = snd_seq_create_simple_port(seq, "aj-snapshot",
                SND_SEQ_PORT_CAP_WRITE, 
                SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
            if(err < 0) 
                fprintf(stderr, "Cannot create port: %s", snd_strerror(err));
            err = snd_seq_connect_from(seq, 0, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);
            if(err < 0)
                fprintf(stderr, "Cannot connect to ALSA system announce port: %s", snd_strerror(err));
            snd_seq_nonblock(seq, 1);
            npfds = snd_seq_poll_descriptors_count(seq, POLLIN);
            pfds = alloca(sizeof(*pfds) * npfds);
        }

        while (daemon_running) {
            // Check if JACK got active while running in the daemon loop,
            // and connect to it if necessary.
            if ((system & JACK) == JACK) {
                if (jackc == NULL) { // Make sure jack is up.
                    jack_initialize(&jackc, (action==DAEMON));
                    if (jackc) system_ready |= JACK;
                } 
            }
            // Reload XML if triggered with HUP signal,
            // and remove connections if necessary.
            if (reload_xml > 0) {
                reload_xml = 0;
                if(verbose) fprintf(stdout, "aj-snapshot: reloading XML file: %s\n", filename);
                xml_node = read_xml(filename, xml_node);
                root_node = mxmlFindElement(xml_node, xml_node, "aj-snapshot", NULL, NULL, MXML_DESCEND_FIRST);
                if(!root_node){ // Still read older aj-snapshot files, that didn't have the aj-snapshot root node.
                    root_node = xml_node;
                }

                if ((system_ready & ALSA) == ALSA) {
                    if(remove_connections){ 
                        alsa_remove_connections(seq);
                    }
                }
                if ((system_ready & JACK) == JACK) {
                    if(remove_connections){ 
                        jack_remove_connections(jackc);
                    }
                }
                // Mark both ALSA and JACK as dirty
                alsa_dirty++;
                pthread_mutex_lock( &registration_callback_lock );
                jack_dirty++;
                pthread_mutex_unlock( &registration_callback_lock );
            }
            // Poll ALSA for new ports and mark ALSA dirty if necessary.
            if ((system_ready & ALSA) == ALSA) {
                snd_seq_poll_descriptors(seq, pfds, npfds, POLLIN);
                if (poll(pfds, npfds, 0) >= 0){
                    snd_seq_event_t *event;
                    while(snd_seq_event_input(seq, &event) > 0){
                        if (event && (event->type == SND_SEQ_EVENT_PORT_START))
                            alsa_dirty++;
                        event = NULL;
                    }
                }
                else perror("poll call failed");
            }

            switch (system_ready){
                case JACK:
                    pthread_mutex_lock( &registration_callback_lock );
                    if (jack_dirty > 0){
                        jack_dirty = 0;
                        pthread_mutex_unlock( &registration_callback_lock );
                        jack_restore(&jackc, root_node);
                    }
                    else pthread_mutex_unlock( &registration_callback_lock );
                    break;
                case ALSA:
                    if(alsa_dirty > 0){
                        alsa_dirty = 0;
                        alsa_restore(seq, root_node);
                    }
                    break;
                case ALSA_JACK:
                    // If running with ALSA and JACK, restore ports of both
                    // if one of them is marked dirty.
                    pthread_mutex_lock( &registration_callback_lock );
                    if ((alsa_dirty > 0) || (jack_dirty > 0)){
                        jack_dirty = 0;
                        pthread_mutex_unlock( &registration_callback_lock );
                        alsa_dirty = 0;

                        alsa_restore(seq, root_node);
                        jack_restore(&jackc, root_node);
                    }
                    else pthread_mutex_unlock( &registration_callback_lock );
               default:
                   break;
            }

            usleep(polling_interval_ms * 1000);
        }
    }

    // Cleanup xml_node:
    if(xml_node) mxmlDelete(xml_node);

    // Close ALSA and JACK clients if necessary.
    if (seq != NULL) snd_seq_close(seq);
    if (jackc != NULL) jack_client_close(jackc);

    // Exit

    if (action == DAEMON || exit_success) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

