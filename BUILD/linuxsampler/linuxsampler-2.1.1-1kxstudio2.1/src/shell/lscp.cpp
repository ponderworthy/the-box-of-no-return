/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>

#include "lscp.h"
#include "LSCPClient.h"
#include "KeyboardReader.h"
#include "TerminalCtrl.h"
#include "CFmt.h"
#include "CCursor.h"
#include "TerminalPrinter.h"
#if DEBUG_LSCP_SHELL
# include <stdarg.h>
# include <sys/timeb.h>
#endif // DEBUG_LSCP_SHELL
#include "../common/global.h"
#include "../common/global_private.h"
#include "../common/Condition.h"

#define LSCP_DEFAULT_HOST "localhost"
#define LSCP_DEFAULT_PORT 8888

using namespace std;
using namespace LinuxSampler;

static LSCPClient* g_client = NULL;
static KeyboardReader* g_keyboardReader = NULL;
static Condition g_todo;
static String g_goodPortion;
static String g_badPortion;
static String g_suggestedPortion;
static int g_linesActive = 0;
static const String g_prompt = "lscp=# ";
static std::vector<String> g_commandHistory;
static int g_commandHistoryIndex = -1;
static String g_doc;
static bool g_quitAppRequested = false;
static int g_exitCode = 0;
#if DEBUG_LSCP_SHELL
static FILE* g_df; ///< handle to output log file (just for debugging)
#endif

/**
 * Log the given debug message to a log file. This works only if
 * DEBUG_LSCP_SHELL was turned on in lscp.h. Otherwise this function does
 * nothing. Usage of this function is equivalent to printf().
 */
void lscpLog(const char* format, ...) {
    #if DEBUG_LSCP_SHELL
    // assemble variable argument list given to this function call
    va_list arg;
    va_start(arg, format);
    // write current timestamp to log file
    struct timeb tp;
    ftime(&tp);
    fprintf(g_df, "[%d.%03d] ", tp.time, tp.millitm);
    // write actual debug message to log file
    vfprintf(g_df, format, arg);
    fflush(g_df);
    va_end(arg);
    #endif // DEBUG_LSCP_SHELL
}

static void printUsage() {
    cout << "lscp - The LinuxSampler Control Protocol (LSCP) Shell." << endl;
    cout << endl;
    cout << "Usage: lscp [-h HOSTNAME] [-p PORT]" << endl;
    cout << endl;
    cout << "   -h  Host name of LSCP server (default \"" << LSCP_DEFAULT_HOST << "\")." << endl;
    cout << endl;
    cout << "   -p  TCP port number of LSCP server (default " << LSCP_DEFAULT_PORT << ")." << endl;
    cout << endl;
    cout << "   --no-auto-correct  Don't perform auto correction of obvious syntax errors." << endl;
    cout << endl;
    cout << "   --no-doc  Don't show LSCP reference documentation on screen." << endl;
    cout << endl;
}

static void printWelcome() {
    cout << "Welcome to lscp " << VERSION << ", the LinuxSampler Control Protocol (LSCP) shell." << endl;
    cout << endl;
}

static void printPrompt() {
    cout << g_prompt << flush;
}

static int promptOffset() {
    return g_prompt.size();
}

static void quitApp(int code = 0) {
    //lscpLog("[quit app]\n");
    g_exitCode = code;
    g_quitAppRequested = true;
}

// Called by the network reading thread, whenever new data arrived from the
// network connection.
static void onLSCPClientNewInputAvailable(LSCPClient* client) {
    g_todo.Set(true);
}

// Called by the keyboard reading thread, whenever a key stroke was received.
static void onNewKeyboardInputAvailable(KeyboardReader* reader) {
    g_todo.Set(true);
}

// Called on network error or when server side closed the TCP connection.
static void onLSCPClientErrorOccured(LSCPClient* client) {
    //lscpLog("[client error callback]\n");
    quitApp();
}

/// Will be called when the user hits tab key to trigger auto completion.
static void autoComplete() {
    if (g_suggestedPortion.empty()) return;
    String s;
    // let the server delete mistaken characters first
    for (int i = 0; i < g_badPortion.size(); ++i) s += '\b';
    // now add the suggested, correct characters
    s += g_suggestedPortion;
    g_suggestedPortion.clear();
    g_client->send(s);
}

static void commandFromHistory(int offset) {
    if (g_commandHistoryIndex + offset < 0 ||
        g_commandHistoryIndex + offset >= g_commandHistory.size()) return;
    g_commandHistoryIndex += offset;
    int len = g_goodPortion.size() + g_badPortion.size();
    String command;
    // erase current active line
    for (int i = 0; i < len; ++i) command += '\b';
    // transmit new/old line to LSCP server
    command += g_commandHistory[g_commandHistory.size() - g_commandHistoryIndex - 1];
    g_client->send(command);
}

/// Will be called when the user hits arrow up key, to iterate to an older command line.
static void previousCommand() {
    commandFromHistory(1);
}

/// Will be called when the user hits arrow down key, to iterate to a more recent command line.
static void nextCommand() {
    commandFromHistory(-1);
}

/// Will be called whenever the user hits ENTER, to store the line in the command history.
static void storeCommandInHistory(const String& sCommand) {
    g_commandHistoryIndex = -1; // reset history index
    // don't save the command if the previous one was the same
    if (g_commandHistory.empty() || g_commandHistory.back() != sCommand)
        g_commandHistory.push_back(sCommand);
}

/// Splits the given string into individual lines for the given screen resolution.
static std::vector<String> splitForScreen(const String& s, int cols, int rows) {
    std::vector<String> lines;
    if (rows <= 0 || cols <= 0) return lines;
    String line;
    for (int i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\r') continue;
        if (c == '\n') {
            lines.push_back(line);
            if (lines.size() >= rows) return lines;
            line.clear();
            continue;
        }
        line += c;
        if (line.size() >= cols) {
            lines.push_back(line);
            if (lines.size() >= rows) return lines;
            line.clear();
        }
    }
    return lines;
}

/**
 * Will be called whenever the LSCP documentation reference to be shown, has
 * been changed. This call will accordingly update the screen with the new
 * documentation text received from LSCP server.
 */
static void updateDoc() {
    const int vOffset = 2;

    CCursor originalCursor = CCursor::now();
    CCursor cursor = originalCursor;
    cursor.toColumn(0).down(vOffset);

    // wipe out current documentation off screen
    cursor.clearVerticalToBottom();

    if (g_doc.empty()) {
        // restore original cursor position
        cursor.up(vOffset).toColumn(originalCursor.column());
        return;
    }

    // get screen size (in characters)
    const int cols = TerminalCtrl::columns();
    const int rows = TerminalCtrl::rows();

    // convert the string block into individual lines according to screen resolution
    std::vector<String> lines = splitForScreen(g_doc, cols - 1, rows);

    // print lines onto screen
    for (int row = 0; row < lines.size(); ++row)
        std::cout << lines[row] << std::endl;

    // restore original cursor position
    cursor.up(vOffset + lines.size()).toColumn(originalCursor.column());
}

/**
 * This LSCP shell application is designed as thin client. That means the heavy
 * LSCP grammar evaluation tasks are peformed by the LSCP server and the shell
 * application's task are simply limited to forward individual characters typed
 * by the user to the LSCP server and showing the result of the LSCP server's
 * evaluation to the user on the screen. This has the big advantage that the
 * shell works perfectly with any machine running (some minimum recent version
 * of) LinuxSampler, no matter which precise LSCP version the server side
 * is using. Which reduces the maintenance efforts for the shell application
 * development tremendously.
 *
 * As soon as this application established a TCP connection to a LSCP server, it
 * sends this command to the LSCP server:
 * @code
 * SET SHELL INTERACT 1
 * @endcode
 * which will inform the LSCP server that this LSCP client is actually a LSCP
 * shell application. The shell will then simply forward every single character
 * typed by the user immediately to the LSCP server. The LSCP server in turn
 * will evaluate every single character received and will return immediately a
 * specially formatted string to the shell application like (assuming the user's
 * current command line was "CREATE AUasdf"):
 * @code
 * SHU:1:CREATE AU{{GF}}asdf{{CU}}{{SB}}DIO_OUTPUT_DEVICE
 * @endcode
 * which informs this shell application about the result of the LSCP grammar
 * evaluation and allows the shell to easily show that result of the evaluation
 * to the user on the screen. In the example reply above, the prefix "SHU:" just
 * indicates to the shell application that this response line is the result
 * of the latest grammar evaluation, the number followed (here 1) indicates the
 * semantic status of the current command line:
 *
 * - 0: Command line is complete, thus ENTER key may be hit by the user now.
 * - 1: Current command line contains syntax error(s).
 * - 2: Command line is incomplete, but contains no syntax errors so far.
 *
 * Then the actual current command line follows, with special markers:
 *
 * - Left of "{{GF}}" the command line is syntactically correct, right of that
 *   marker the command line is syntactically wrong.
 *
 * - Marker "{{CU}}" indicates the current cursor position of the command line.
 *
 * - Right of "{{SB}}" follows the current auto completion suggestion, so that
 *   string portion was not typed by the user yet, but is expected to be typed
 *   by him next, to retain syntax correctness. The auto completion portion is
 *   added by the LSCP server only if there is one unique way to add characters
 *   to the current command line. If there are multiple possibilities, than this
 *   portion is missing due to ambiguity.
 *
 * - Optionally there might also be a "{{PB}" marker on right hand side of the
 *   line. The text right to that marker reflects all possibilities at the
 *   user's current input position (which cannot be auto completed) due to
 *   ambiguity, including abstract (a.k.a. "non-terminal") symbols like:
 *   @code
 *   <digit>, <text>, <number>, etc.
 *   @endcode
 *   This portion is added by the LSCP server only if there is not a unique way
 *   to add characters to the current command line.
 */
int main(int argc, char *argv[]) {
    #if DEBUG_LSCP_SHELL
    g_df = fopen("lscp.log", "w");
    if (!g_df) {
        std::cerr << "Could not open lscp.log for writing!\n";
        exit(-1);
    }
    #endif // DEBUG_LSCP_SHELL

    String host = LSCP_DEFAULT_HOST;
    int port    = LSCP_DEFAULT_PORT;
    bool autoCorrect = true;
    bool showDoc = true;

    // parse command line arguments
    for (int i = 0; i < argc; ++i) {
        String s = argv[i];
        if (s == "-h" || s == "--host") {
            if (++i >= argc) {
                printUsage();
                return -1;
            }
            host = argv[i];
        } else if (s == "-p" || s == "--port") {
            if (++i >= argc) {
                printUsage();
                return -1;
            }
            port = atoi(argv[i]);
            if (port <= 0) {
                cerr << "Error: invalid port argument \"" << argv[i] << "\"\n";
                return -1;
            }
        } else if (s == "--no-auto-correct") {
            autoCorrect = false;
        } else if (s == "--no-doc") {
            showDoc = false;
        } else if (s[0] == '-') { // invalid / unknown command line argument ...
            printUsage();
            return -1;
        }
    }

    // try to connect to the sampler's LSCP server and start a thread for
    // receiving incoming network data from the sampler's LSCP server
    g_client = new LSCPClient;
    g_client->setErrorCallback(onLSCPClientErrorOccured);
    if (!g_client->connect(host, port)) return -1;
    g_client->sendCommandSync(
        (showDoc) ? "SET SHELL DOC 1" : "SET SHELL DOC 0"
    );
    String sResponse = g_client->sendCommandSync(
        (autoCorrect) ? "SET SHELL AUTO_CORRECT 1" : "SET SHELL AUTO_CORRECT 0"
    );
    sResponse = g_client->sendCommandSync("SET SHELL INTERACT 1");
    if (sResponse.substr(0, 2) != "OK") {
        cerr << "Error: sampler too old, it does not support shell instructions\n";
        return -1;
    }
    g_client->setCallback(onLSCPClientNewInputAvailable);

    printWelcome();
    printPrompt();

    // start a thread for reading from the local text input keyboard
    // (keyboard echo will be disabled as well to have a clean control on what
    // is appearing on the screen)
    g_keyboardReader = new KeyboardReader;
    g_keyboardReader->setCallback(onNewKeyboardInputAvailable);
    g_keyboardReader->startReading();

    int iKbdEscapeCharsExpected = 0;
    char kbdPrevEscapeChar;

    // main thread's loop
    //
    // This application runs 3 threads:
    //
    // - Keyboard thread: reads constantly on stdin for new characters (which
    //   will block this keyboard thread until new character(s) were typed by
    //   the user) and pushes the typed characters into a FIFO buffer.
    //
    // - Network thread: reads constantly on the TCP connection for new bytes
    //   being sent by the LSCP server (which will block this network thread
    //   until new bytes were received) and pushes the received bytes into a
    //   FIFO buffer.
    //
    // - Main thread: this thread runs in the loop below. The main thread sleeps
    //   (by using the "g_todo" condition variable) until either new keys on the
    //   keyboard were stroke by the user or until new bytes were received from
    //   the LSCP server. The main thread will then accordingly send the typed
    //   characters to the LSCP server and/or show the result of the LSCP
    //   server's latest evaluation to the user on the screen (by pulling those
    //   data from the other two thread's FIFO buffers).
    while (!g_quitAppRequested) {
        // sleep until either new data from the network or from keyboard arrived
        g_todo.WaitIf(false);
        // immediately unset the condition variable and unlock it
        g_todo.Set(false);
        g_todo.Unlock();

        // did network data arrive?
        while (g_client->messageComplete()) {
            String line = *g_client->popLine();
            //lscpLog("[client] '%s'\n", line.c_str());
            if (line.substr(0,4) == "SHU:") {
                int code = 0, n = 0;
                int res = sscanf(line.c_str(), "SHU:%d:%n", &code, &n);
                if (res >= 1) {                    
                    String s = line.substr(n);

                    // extract portion that is already syntactically correct
                    size_t iGood = s.find(LSCP_SHK_GOOD_FRONT);
                    String sGood = s.substr(0, iGood);
                    if (sGood.find(LSCP_SHK_CURSOR) != string::npos)
                        sGood.erase(sGood.find(LSCP_SHK_CURSOR), strlen(LSCP_SHK_CURSOR)); // erase cursor marker

                    // extract portion that was written syntactically incorrect
                    String sBad = s.substr(iGood + strlen(LSCP_SHK_GOOD_FRONT));
                    if (sBad.find(LSCP_SHK_CURSOR) != string::npos)
                        sBad.erase(sBad.find(LSCP_SHK_CURSOR), strlen(LSCP_SHK_CURSOR)); // erase cursor marker
                    if (sBad.find(LSCP_SHK_SUGGEST_BACK) != string::npos)
                        sBad.erase(sBad.find(LSCP_SHK_SUGGEST_BACK)); // erase auto suggestion portion
                    if (sBad.find(LSCP_SHK_POSSIBILITIES_BACK) != string::npos)
                        sBad.erase(sBad.find(LSCP_SHK_POSSIBILITIES_BACK)); // erase possibilities portion

                    // extract portion that is suggested for auto completion
                    String sSuggest;
                    if (s.find(LSCP_SHK_SUGGEST_BACK) != string::npos) {
                        sSuggest = s.substr(s.find(LSCP_SHK_SUGGEST_BACK) + strlen(LSCP_SHK_SUGGEST_BACK));
                        if (sSuggest.find(LSCP_SHK_CURSOR) != string::npos)
                            sSuggest.erase(sSuggest.find(LSCP_SHK_CURSOR), strlen(LSCP_SHK_CURSOR)); // erase cursor marker
                        if (sSuggest.find(LSCP_SHK_POSSIBILITIES_BACK) != string::npos)
                            sSuggest.erase(sSuggest.find(LSCP_SHK_POSSIBILITIES_BACK)); // erase possibilities portion
                    }

                    // extract portion that provides all current possibilities
                    // (that is all branches in the current grammar tree)
                    String sPossibilities;
                    if (s.find(LSCP_SHK_POSSIBILITIES_BACK) != string::npos) {
                        sPossibilities = s.substr(s.find(LSCP_SHK_POSSIBILITIES_BACK) + strlen(LSCP_SHK_POSSIBILITIES_BACK));
                    }

                    // extract current cursor position
                    int cursorColumn = sGood.size();
                    String sCursor = s;
                    if (sCursor.find(LSCP_SHK_GOOD_FRONT) != string::npos)
                        sCursor.erase(sCursor.find(LSCP_SHK_GOOD_FRONT), strlen(LSCP_SHK_GOOD_FRONT)); // erase good/bad marker
                    if (sCursor.find(LSCP_SHK_SUGGEST_BACK) != string::npos)
                        sCursor.erase(sCursor.find(LSCP_SHK_SUGGEST_BACK), strlen(LSCP_SHK_SUGGEST_BACK)); // erase suggestion marker
                    if (sCursor.find(LSCP_SHK_CURSOR) != string::npos)
                        cursorColumn = sCursor.find(LSCP_SHK_CURSOR);

                    // store those informations globally for the auto-completion
                    // feature
                    g_goodPortion      = sGood;
                    g_badPortion       = sBad;
                    g_suggestedPortion = sSuggest;

                    //printf("line '%s' good='%s' bad='%s' suggested='%s' cursor=%d\n", line.c_str(), sGood.c_str(), sBad.c_str(), sSuggest.c_str(), cursorColumn);

                    // clear current command line on screen
                    // (which may have been printed over several lines)
                    CCursor cursor = CCursor::now().toColumn(0).clearLine();
                    for (int i = 0; i < g_linesActive; ++i)
                        cursor = cursor.down().clearLine();
                    if (g_linesActive) cursor = cursor.up(g_linesActive).toColumn(0);
                    printPrompt();

                    // print out the gathered informations on the screen
                    TerminalPrinter p;
                    CFmt cfmt;
                    if (code == LSCP_SHU_COMPLETE) cfmt.bold().green();
                    else cfmt.bold().white();
                    p << sGood;
                    cfmt.reset().red();
                    p << sBad;
                    cfmt.bold().yellow();
                    p << sSuggest;
                    if (!sPossibilities.empty())
                        p << " <- " << sPossibilities;

                    // move cursor back to the appropriate input position in
                    // the command line (which may be several lines above)
                    g_linesActive = p.linesAdvanced();
                    if (p.linesAdvanced()) cursor.up(p.linesAdvanced());
                    cursor.toColumn(cursorColumn + promptOffset());
                }
            } else if (line.substr(0,4) == "SHD:") { // new LSCP doc reference section received ...
                int code = LSCP_SHD_NO_MATCH;
                int res = sscanf(line.c_str(), "SHD:%d", &code);
                g_doc.clear();
                if (code == LSCP_SHD_MATCH) {
                    while (true) { // multi-line response expected (terminated by dot line) ...
                       if (line.substr(0, 1) == ".") break;
                       if (!g_client->lineAvailable()) break;
                       line = *g_client->popLine();
                       g_doc += line;
                    }
                }
                updateDoc();
            } else if (line.substr(0,2) == "OK") { // single-line response expected ...
                cout << endl << flush;

                // wipe out potential current documentation off screen
                CCursor cursor = CCursor::now();
                cursor.clearVerticalToBottom();

                CFmt cfmt;
                cfmt.green();
                cout << line.substr(0,2) << flush;
                cfmt.reset();
                cout << line.substr(2) << endl << flush;
                printPrompt();
            } else if (line.substr(0,3) == "WRN") { // single-line response expected ...
                cout << endl << flush;

                // wipe out potential current documentation off screen
                CCursor cursor = CCursor::now();
                cursor.clearVerticalToBottom();

                CFmt cfmt;
                cfmt.yellow();
                cout << line.substr(0,3) << flush;
                cfmt.reset();
                cout << line.substr(3) << endl << flush;
                printPrompt();
            } else if (line.substr(0,3) == "ERR") { // single-line response expected ...
                cout << endl << flush;

                // wipe out potential current documentation off screen
                CCursor cursor = CCursor::now();
                cursor.clearVerticalToBottom();

                CFmt cfmt;
                cfmt.bold().red();
                cout << line.substr(0,3) << flush;
                cfmt.reset();
                cout << line.substr(3) << endl << flush;
                printPrompt();
            } else if (g_client->multiLine()) { // multi-line response expected ...
                cout << endl << flush;

                // wipe out potential current documentation off screen
                CCursor cursor = CCursor::now();
                cursor.clearVerticalToBottom();

                while (true) {                   
                   cout << line << endl << flush;
                   if (line.substr(0, 1) == ".") break;
                   if (!g_client->lineAvailable()) break;
                   line = *g_client->popLine();
                }
                printPrompt();
            } else {
                cout << endl << flush;

                // wipe out potential current documentation off screen
                CCursor cursor = CCursor::now();
                cursor.clearVerticalToBottom();

                cout << line << endl << flush;
                printPrompt();
            }
        }

        // did keyboard input arrive?
        while (g_keyboardReader->charAvailable()) {
            char c = g_keyboardReader->popChar();
            //lscpLog("[keyboard] '%c' (dec %d%s)'\n", c, (int)c, iKbdEscapeCharsExpected ? " ESC SEQ" : "");

            //std::cout << c << "(" << int(c) << ")" << std::endl << std::flush;
            if (iKbdEscapeCharsExpected) { // escape sequence (still) expected now ...
                iKbdEscapeCharsExpected--;
                if (iKbdEscapeCharsExpected) kbdPrevEscapeChar = c;
                else { // escape sequence is complete ...
                    if (kbdPrevEscapeChar == 91 && c == 65) // up key
                        previousCommand();
                    else if (kbdPrevEscapeChar == 91 && c == 66) // down key
                        nextCommand();
                    else if (kbdPrevEscapeChar == 91 && c == 68) // left key
                        g_client->send(2); // custom usage of this ASCII code
                    else if (kbdPrevEscapeChar == 91 && c == 67) // right key
                        g_client->send(3); // custom usage of this ASCII code
                }
                continue; // don't send this escape sequence character to LSCP server
            } else if (c == KBD_ESCAPE) { // escape sequence for special keys expected next ...
                iKbdEscapeCharsExpected = 2;
                continue; // don't send ESC character to LSCP server
            } else if (c == KBD_BACKSPACE) {
                c = '\b';
            } else if (c == '\t') { // auto completion ...
                autoComplete();
                continue; // don't send tab character to LSCP server
            } else if (c == '\n') {
                storeCommandInHistory(g_goodPortion + g_badPortion);
            }

            g_client->send(c);
        }
    }

    // Application is going to exit (due to user request or server
    // disconnection). Clean up everything ...
    std::cout << std::endl << std::flush;
    if (g_client) delete g_client;
    if (g_keyboardReader) delete g_keyboardReader;

    return g_exitCode;
}
