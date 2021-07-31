/*****************************************************************************
 *                                                                           *
 *  LSCP documentation reference built into LSCP shell.                      *
 *                                                                           *
 *  Copyright (c) 2014 Christian Schoenebeck                                 *
 *                                                                           *
 *  This program is part of LinuxSampler and released under the same terms.  *
 *                                                                           *
 *****************************************************************************/

#ifndef LSCP_SHELL_REFERENCE_H
#define LSCP_SHELL_REFERENCE_H

/**
 * Encapsulates the reference documentation for exactly one LSCP command.
 */
struct lscp_ref_entry_t {
    const char* name; ///< shortened name of the LSCP command, acting as unique key (i.e. "CREATE AUDIO_OUTPUT_DEVICE").
    const char* section; ///< Multi-line text block, being the actual documentation section of that LSCP command.
};

/**
 * Returns the LSCP documentation reference section for the LSCP command given
 * by @a cmd. A non complete LSCP command may be passed to this function. In the
 * latter case this function will try to "guess" the LSCP command. If there is
 * no unique match for the given LSCP command, then NULL is returned.
 *
 * @param cmd - LSCP command
 * @returns LSCP reference section for given LSCP command or NULL if there is
 *          no unique match
 */
lscp_ref_entry_t* lscp_reference_for_command(const char* cmd);

#endif // LSCP_SHELL_REFERENCE_H
