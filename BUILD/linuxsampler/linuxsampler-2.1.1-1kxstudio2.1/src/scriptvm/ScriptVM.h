/*
 * Copyright (c) 2014-2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_SCRIPTVM_H
#define LS_SCRIPTVM_H

#include <iostream>
#include <vector>

#include "../common/global.h"
#include "common.h"

namespace LinuxSampler {

    class ParserContext;
    class ExecContext;

    /** @brief Core virtual machine for real-time instrument scripts.
     *
     * This is the core of the virtual machine and main entry class, used for
     * running real-time instrument scripts. This VM core encompasses the
     * instrument script parser, generalized virtual machine and very generic
     * built-in script functions. Thus this class only provides functionalities
     * which are yet independent of the actual purpose the virtual machine is
     * going to be used for.
     *
     * The actual use case specific functionalites (i.e. MIDI processing) is
     * then implemented by sampler engines' VM classes which are derived from
     * this generalized ScriptVM class.
     *
     * Typical usage of this class:
     *
     * - 1. Create an instance of this ScriptVM class (or of one of its deriving
     *      classes).
     * - 2. Load a script by passing its source code to method loadScript(),
     *      which will return the parsed representation of the script.
     * - 3. Create a VM execution context by calling createExecContext().
     * - 4. Execute the script by calling method exec().
     *
     * This class is re-entrant safe, but not thread safe. So you can share one
     * instance of this class between multiple (native) threads, but you @b must
     * @b not execute methods of the same class instance simultaniously from
     * different (native) threads. If you want to execute scripts simultaniously
     * multi threaded, then create a separate ScriptVM instance for each
     * (native) thread. Also note that one VMParserContext instance is tied to
     * exactly one ScriptVM instance. So you @b must @b not create a
     * VMParserContext with one ScriptVM instance and run it with a different
     * ScriptVM instance!
     */
    class ScriptVM : public VMFunctionProvider {
    public:
        ScriptVM();
        virtual ~ScriptVM();

        /**
         * Loads a script given by its source code (passed as argument @a s to
         * this method) and returns the parsed representation of that script.
         * After calling this method you must check the returned VMParserContext
         * object whether there had been any parser errors. If there were no
         * parser errors, you may pass the VMParserContext object to method
         * exec() for actually executing the script.
         *
         * It is your responsibility to free the returned VMParserContext
         * object once you don't need it anymore.
         *
         * @param s - entire source code of the script to be loaded
         * @returns parsed representation of the script
         */
        VMParserContext* loadScript(const String& s);

        /**
         * Same as above's loadScript() method, but this one reads the script's
         * source code from an input stream object (i.e. stdin or a file).
         *
         * @param is - input stream from which the entire source code of the
         *             script is to be read and loaded from
         * @returns parsed representation of the script
         */
        VMParserContext* loadScript(std::istream* is);

        /**
         * Parses a script's source code (passed as argument @a s to this
         * method), splits that input up in its individual tokens (i.e.
         * keyword, variable name, event name, etc.) and returns all those
         * tokens, for the purpose that the caller can provide syntax syntax
         * highlighting for the passed script.
         *
         * This method is actually not used by the sampler at all, it is rather
         * provided for external script editor applications, to provide them a
         * convenient backend for parsing scripts and providing syntax
         * highlighting.
         *
         * @returns recognized tokens of passed script's source code
         */
        std::vector<VMSourceToken> syntaxHighlighting(const String& s);

        /**
         * Same as above's syntaxHighlighting() method, but this one reads the
         * script's source code from an input stream object (i.e. stdin or a
         * file).
         *
         * @param is - input stream from which the entire source code of the
         *             script is to be read and loaded from
         * @returns recognized tokens of passed script's source code
         */
        std::vector<VMSourceToken> syntaxHighlighting(std::istream* is);

        /**
         * Dumps the translated tree of the already parsed script, given by
         * argument @a context, to stdout. This method is for debugging purposes
         * only.
         *
         * @param context - parsed representation of the script
         * @see loadScript()
         */
        void dumpParsedScript(VMParserContext* context);

        /**
         * Creates a so called VM exceution context for a specific, already
         * parsed script (provided by argument @a parserContext). Due to the
         * general real-time design of this virtual machine, the VM execution
         * context differs for every script. So you must (re)create the
         * execution context for each script being loaded.
         *
         * @param parserContext - parsed representation of the script
         * @see loadScript()
         */
        VMExecContext* createExecContext(VMParserContext* parserContext);

        /**
         * Execute a script by virtual machine. Since scripts are event-driven,
         * you actually execute only one specific event handler block (i.e. a
         * "on note ... end on" code block) by calling this method (not the
         * entire script), and hence you must provide one precise handler of the
         * script to be executed by this method.
         *
         * This method usually blocks until the entire script event handler
         * block has been executed completely. It may however also return before
         * completion if either a) a script runtime error occurred or b) the
         * script was suspended by the VM (either because script execution
         * exceeded a certain limit of time or the script called the built-in
         * wait() function). You must check the return value of this method to
         * find out which case applies.
         *
         * @param parserContext - parsed representation of the script (see loadScript())
         * @param execContext - VM execution context (see createExecContext())
         * @param handler - precise event handler (i.e. "on note ... end on"
         *                  code block) to be executed
         *                  (see VMParserContext::eventHandlerByName())
         * @returns current status of the vitual machine (i.e. script succeeded,
         *          script runtime error occurred or script was suspended for
         *          some reason).
         */
        VMExecStatus_t exec(VMParserContext* parserContext, VMExecContext* execContext, VMEventHandler* handler);

        /**
         * Returns built-in script function for the given function @a name. To
         * get the implementation of the built-in message() script function for
         * example, you would pass "message" here).
         *
         * This method is re-implemented by deriving classes to add more use
         * case specific built-in functions.
         *
         * @param name - name of the function to be retrieved (i.e. "wait" for the 
         *               built-in wait() function).
         */
        VMFunction* functionByName(const String& name) OVERRIDE;

        /**
         * Whether the passed built-in function is disabled and should thus be
         * ignored by the parser at the passed parser context (parser state
         * where the built-in function call occurs).
         *
         * @param fn - built-in function to be checked
         * @param ctx - parser context at the position where the built-in
         *              function call is located within the script
         */
        bool isFunctionDisabled(VMFunction* fn, VMParserContext* ctx) OVERRIDE;

        /**
         * Returns all built-in integer script variables. This method returns a
         * STL map, where the map's key is the variable name and the map's value
         * is the native pointer to the actual built-in variable.
         *
         * This method is re-implemented by deriving classes to add more use
         * case specific built-in variables.
         */
        std::map<String,VMIntRelPtr*> builtInIntVariables() OVERRIDE;

        /**
         * Returns all built-in (8 bit) integer array script variables. This
         * method returns a STL map, where the map's key is the array variable
         * name and the map's value is the native pointer to the actual built-in
         * array variable.
         *
         * This method is re-implemented by deriving classes to add more use
         * case specific built-in array variables.
         */
        std::map<String,VMInt8Array*> builtInIntArrayVariables() OVERRIDE;

        /**
         * Returns all built-in constant integer script variables, which are
         * constant and their final data is already available at parser time
         * and won't change during runtime. Providing your built-in constants
         * this way may lead to performance benefits compared to using other
         * ways of providing built-in variables, because the script parser
         * can perform optimizations when the script is refering to such
         * constants.
         *
         * This type of built-in variable can only be read, but not be altered
         * by scripts. This method returns a STL map, where the map's key is
         * the variable name and the map's value is the final constant data.
         *
         * This method is re-implemented by deriving classes to add more use
         * case specific built-in constant integers.
         *
         * @b Note: In case your built-in variable should be read-only but its
         * value is not already available at parser time (i.e. because its
         * value may change at runtime), then you should add it to
         * builtInIntVariables() instead and use the macro
         * DECLARE_VMINT_READONLY() to define the variable for read-only
         * access by scripts.
         */
        std::map<String,int> builtInConstIntVariables() OVERRIDE;

        /**
         * Returns all built-in dynamic variables. This method returns a STL
         * map, where the map's key is the dynamic variable's name and the
         * map's value is the pointer to the actual object implementing the
         * behavior which is actually generating the content of the dynamic
         * variable.
         *
         * This method is re-implemented by deriving classes to add more use
         * case specific built-in dynamic variables.
         */
        std::map<String,VMDynVar*> builtInDynamicVariables() OVERRIDE;

        /**
         * Enables or disables automatic suspension of scripts by the VM.
         * If automatic suspension is enabled then scripts are monitored
         * regarding their execution time and in case they are execution
         * for too long, then they are automatically suspended by the VM for
         * a certain amount of time in order to avoid any RT instablity
         * issues caused by bugs in the script, i.e. endless while() loops
         * or very large scripts.
         *
         * Automatic suspension is enabled by default due to the aimed
         * real-time context of this virtual machine.
         *
         * @param b - true: enable auto suspension [default],
         *            false: disable auto suspension
         */
        void setAutoSuspendEnabled(bool b = true);

        /**
         * Returns true in case automatic suspension of scripts by the VM is
         * enabled. See setAutoSuspendEnabled() for details.
         *
         * Automatic suspension is enabled by default due to the aimed
         * real-time context of this virtual machine.
         */
        bool isAutoSuspendEnabled() const;

        VMEventHandler* currentVMEventHandler(); //TODO: should be protected (only usable during exec() calls, intended only for VMFunctions)
        VMParserContext* currentVMParserContext(); //TODO: should be protected (only usable during exec() calls, intended only for VMFunctions)
        VMExecContext* currentVMExecContext(); //TODO: should be protected (only usable during exec() calls, intended only for VMFunctions)

    protected:
        VMEventHandler* m_eventHandler;
        ParserContext* m_parserContext;
        bool m_autoSuspend;
        class CoreVMFunction_message* m_fnMessage;
        class CoreVMFunction_exit* m_fnExit;
        class CoreVMFunction_wait* m_fnWait;
        class CoreVMFunction_abs* m_fnAbs;
        class CoreVMFunction_random* m_fnRandom;
        class CoreVMFunction_num_elements* m_fnNumElements;
        class CoreVMFunction_inc* m_fnInc;
        class CoreVMFunction_dec* m_fnDec;
        class CoreVMFunction_in_range* m_fnInRange;
        class CoreVMFunction_sh_left* m_fnShLeft;
        class CoreVMFunction_sh_right* m_fnShRight;
        class CoreVMFunction_min* m_fnMin;
        class CoreVMFunction_max* m_fnMax;
        class CoreVMFunction_array_equal* m_fnArrayEqual;
        class CoreVMFunction_search* m_fnSearch;
        class CoreVMFunction_sort* m_fnSort;
        class CoreVMDynVar_NKSP_REAL_TIMER* m_varRealTimer;
        class CoreVMDynVar_NKSP_PERF_TIMER* m_varPerfTimer;
    };

} // namespace LinuxSampler

#endif // LS_INSTRUMENTSCRIPTVM_H
