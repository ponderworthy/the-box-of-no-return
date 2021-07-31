/*
 * Copyright (c) 2014-2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// This header defines data types shared between the VM core implementation
// (inside the current source directory) and other parts of the sampler
// (located at other source directories). It also acts as public API of the
// Real-Time script engine for other applications.

#ifndef LS_INSTR_SCRIPT_PARSER_COMMON_H
#define LS_INSTR_SCRIPT_PARSER_COMMON_H

#include "../common/global.h"
#include <vector>
#include <map>
#include <stddef.h> // offsetof()

namespace LinuxSampler {

    /**
     * Identifies the type of a noteworthy issue identified by the script
     * parser. That's either a parser error or parser warning.
     */
    enum ParserIssueType_t {
        PARSER_ERROR, ///< Script parser encountered an error, the script cannot be executed.
        PARSER_WARNING ///< Script parser encountered a warning, the script may be executed if desired, but the script may not necessarily behave as originally intended by the script author.
    };

    /** @brief Expression's data type.
     *
     * Identifies to which data type an expression within a script evaluates to.
     * This can for example reflect the data type of script function arguments,
     * script function return values, but also the resulting data type of some
     * mathematical formula within a script.
     */
    enum ExprType_t {
        EMPTY_EXPR, ///< i.e. on invalid expressions or i.e. a function call that does not return a result value (the built-in wait() or message() functions for instance)
        INT_EXPR, ///< integer (scalar) expression
        INT_ARR_EXPR, ///< integer array expression
        STRING_EXPR, ///< string expression
        STRING_ARR_EXPR, ///< string array expression
    };

    /** @brief Result flags of a script statement or script function call.
     *
     * A set of bit flags which provide informations about the success or
     * failure of a statement within a script. That's also especially used for
     * providing informations about success / failure of a call to a built-in
     * script function. The virtual machine evaluates these flags during runtime
     * to decide whether it should i.e. stop or suspend execution of a script.
     *
     * Since these are bit flags, these constants are bitwise combined.
     */
    enum StmtFlags_t {
       STMT_SUCCESS = 0, ///< Function / statement was executed successfully, no error occurred.
       STMT_ABORT_SIGNALLED = 1, ///< VM should stop the current callback execution (usually because of an error, but might also be without an error reason, i.e. when the built-in script function exit() was called).
       STMT_SUSPEND_SIGNALLED = (1<<1), ///< VM supended execution, either because the script called the built-in wait() script function or because the script consumed too much execution time and was forced by the VM to be suspended for some time
       STMT_ERROR_OCCURRED = (1<<2), ///< VM stopped execution due to some script runtime error that occurred 
    };

    /** @brief Virtual machine execution status.
     *
     * A set of bit flags which reflect the current overall execution status of
     * the virtual machine concerning a certain script execution instance.
     *
     * Since these are bit flags, these constants are bitwise combined.
     */
    enum VMExecStatus_t {
        VM_EXEC_NOT_RUNNING = 0, ///< Script is currently not executed by the VM.
        VM_EXEC_RUNNING = 1, ///< The VM is currently executing the script.
        VM_EXEC_SUSPENDED = (1<<1), ///< Script is currently suspended by the VM, either because the script called the built-in wait() script function or because the script consumed too much execution time and was forced by the VM to be suspended for some time.
        VM_EXEC_ERROR = (1<<2), ///< A runtime error occurred while executing the script (i.e. a call to some built-in script function failed).
    };

    /** @brief Script event handler type.
     *
     * Identifies one of the possible event handler callback types defined by
     * the NKSP script language.
     */
    enum VMEventHandlerType_t {
        VM_EVENT_HANDLER_INIT, ///< Initilization event handler, that is script's "on init ... end on" code block.
        VM_EVENT_HANDLER_NOTE, ///< Note event handler, that is script's "on note ... end on" code block.
        VM_EVENT_HANDLER_RELEASE, ///< Release event handler, that is script's "on release ... end on" code block.
        VM_EVENT_HANDLER_CONTROLLER, ///< Controller event handler, that is script's "on controller ... end on" code block.
    };

    // just symbol prototyping
    class VMIntExpr;
    class VMStringExpr;
    class VMIntArrayExpr;
    class VMStringArrayExpr;
    class VMParserContext;

    /** @brief Virtual machine expression
     *
     * This is the abstract base class for all expressions of scripts.
     * Deriving classes must implement the abstract method exprType().
     *
     * An expression within a script is translated into one instance of this
     * class. It allows a high level access for the virtual machine to evaluate
     * and handle expressions appropriately during execution. Expressions are
     * for example all kinds of formulas, function calls, statements or a
     * combination of them. Most of them evaluate to some kind of value, which
     * might be further processed as part of encompassing expressions to outer
     * expression results and so forth.
     */
    class VMExpr {
    public:
        /**
         * Identifies the data type to which the result of this expression
         * evaluates to. This abstract method must be implemented by deriving
         * classes.
         */
        virtual ExprType_t exprType() const = 0;

        /**
         * In case this expression is an integer expression, then this method
         * returns a casted pointer to that VMIntExpr object. It returns NULL
         * if this expression is not an integer expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. actually a string expression like "12",
         * calling asInt() will @b not cast that numerical string expression to
         * an integer expression 12 for you, instead this method will simply
         * return NULL!
         *
         * @see exprType()
         */
        VMIntExpr* asInt() const;

        /**
         * In case this expression is a string expression, then this method
         * returns a casted pointer to that VMStringExpr object. It returns NULL
         * if this expression is not a string expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. actually an integer expression like 120,
         * calling asString() will @b not cast that integer expression to a
         * string expression "120" for you, instead this method will simply
         * return NULL!
         *
         * @see exprType()
         */
        VMStringExpr* asString() const;

        /**
         * In case this expression is an integer array expression, then this
         * method returns a casted pointer to that VMIntArrayExpr object. It
         * returns NULL if this expression is not an integer array expression.
         *
         * @b Note: type casting performed by this method is strict! That means
         * if this expression is i.e. an integer expression or a string
         * expression, calling asIntArray() will @b not cast those scalar
         * expressions to an array expression for you, instead this method will
         * simply return NULL!
         *
         * @b Note: this method is currently, and in contrast to its other
         * counter parts, declared as virtual method. Some deriving classes are
         * currently using this to override this default implementation in order
         * to implement an "evaluate now as integer array" behavior. This has
         * efficiency reasons, however this also currently makes this part of
         * the API less clean and should thus be addressed in future with
         * appropriate changes to the API.
         *
         * @see exprType()
         */
        virtual VMIntArrayExpr* asIntArray() const;

        /**
         * Returns true in case this expression can be considered to be a
         * constant expression. A constant expression will retain the same
         * value throughout the entire life time of a script and the
         * expression's constant value may be evaluated already at script
         * parse time, which may result in performance benefits during script
         * runtime.
         *
         * @b NOTE: A constant expression is per se always also non modifyable.
         * But a non modifyable expression may not necessarily be a constant
         * expression!
         *
         * @see isModifyable()
         */
        virtual bool isConstExpr() const = 0;

        /**
         * Returns true in case this expression is allowed to be modified.
         * If this method returns @c false then this expression must be handled
         * as read-only expression, which means that assigning a new value to it
         * is either not possible or not allowed.
         *
         * @b NOTE: A constant expression is per se always also non modifyable.
         * But a non modifyable expression may not necessarily be a constant
         * expression!
         *
         * @see isConstExpr()
         */
        bool isModifyable() const;
    };

    /** @brief Virtual machine integer expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to an integer (scalar) value. Deriving classes implement the
     * abstract method evalInt() to return the actual integer result value of
     * the expression.
     */
    class VMIntExpr : virtual public VMExpr {
    public:
        /**
         * Returns the result of this expression as integer (scalar) value.
         * This abstract method must be implemented by deriving classes.
         */
        virtual int evalInt() = 0;

        /**
         * Returns always INT_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return INT_EXPR; }
    };

    /** @brief Virtual machine string expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to a string value. Deriving classes implement the abstract
     * method evalStr() to return the actual string result value of the
     * expression.
     */
    class VMStringExpr : virtual public VMExpr {
    public:
        /**
         * Returns the result of this expression as string value. This abstract
         * method must be implemented by deriving classes.
         */
        virtual String evalStr() = 0;

        /**
         * Returns always STRING_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return STRING_EXPR; }
    };

    /** @brief Virtual Machine Array Value Expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to some kind of array value. Deriving classes implement the
     * abstract method arraySize() to return the amount of elements within the
     * array.
     */
    class VMArrayExpr : virtual public VMExpr {
    public:
        /**
         * Returns amount of elements in this array. This abstract method must
         * be implemented by deriving classes.
         */
        virtual int arraySize() const = 0;
    };

    /** @brief Virtual Machine Integer Array Expression
     *
     * This is the abstract base class for all expressions inside scripts which
     * evaluate to an array of integer values. Deriving classes implement the
     * abstract methods arraySize(), evalIntElement() and assignIntElement() to
     * access the individual integer array values.
     */
    class VMIntArrayExpr : virtual public VMArrayExpr {
    public:
        /**
         * Returns the (scalar) integer value of the array element given by
         * element index @a i.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         */
        virtual int evalIntElement(uint i) = 0;

        /**
         * Changes the current value of an element (given by array element
         * index @a i) of this integer array.
         *
         * @param i - array element index (must be between 0 .. arraySize() - 1)
         * @param value - new integer scalar value to be assigned to that array element
         */
        virtual void assignIntElement(uint i, int value) = 0;

        /**
         * Returns always INT_ARR_EXPR for instances of this class.
         */
        ExprType_t exprType() const OVERRIDE { return INT_ARR_EXPR; }
    };

    /** @brief Arguments (parameters) for being passed to a built-in script function.
     *
     * An argument or a set of arguments passed to a script function are
     * translated by the parser to an instance of this class. This abstract
     * interface class is used by implementations of built-in functions to
     * obtain the individual function argument values being passed to them at
     * runtime.
     */
    class VMFnArgs {
    public:
        /**
         * Returns the amount of arguments going to be passed to the script
         * function.
         */
        virtual int argsCount() const = 0;

        /**
         * Returns the respective argument (requested by argument index @a i) of
         * this set of arguments. This method is called by implementations of
         * built-in script functions to obtain the value of each function
         * argument passed to the function at runtime.
         *
         * @param i - function argument index (indexed from left to right)
         */
        virtual VMExpr* arg(int i) = 0;
    };

    /** @brief Result value returned from a call to a built-in script function.
     *
     * Implementations of built-in script functions return an instance of this
     * object to let the virtual machine obtain the result value of the function
     * call, which might then be further processed by the virtual machine
     * according to the script. It also provides informations about the success
     * or failure of the function call.
     */
    class VMFnResult {
    public:
        /**
         * Returns the result value of the function call, represented by a high
         * level expression object.
         */
        virtual VMExpr* resultValue() = 0;

        /**
         * Provides detailed informations of the success / failure of the
         * function call. The virtual machine is evaluating the flags returned
         * here to decide whether it must abort or suspend execution of the
         * script at this point.
         */
        virtual StmtFlags_t resultFlags() { return STMT_SUCCESS; }
    };

    /** @brief Virtual machine built-in function.
     *
     * Abstract base class for built-in script functions, defining the interface
     * for all built-in script function implementations. All built-in script
     * functions are deriving from this abstract interface class in order to
     * provide their functionality to the virtual machine with this unified
     * interface.
     *
     * The methods of this interface class provide two purposes:
     *
     * 1. When a script is loaded, the script parser uses the methods of this
     *    interface to check whether the script author was calling the
     *    respective built-in script function in a correct way. For example
     *    the parser checks whether the required amount of parameters were
     *    passed to the function and whether the data types passed match the
     *    data types expected by the function. If not, loading the script will
     *    be aborted with a parser error, describing to the user (i.e. script
     *    author) the precise misusage of the respective function.
     * 2. After the script was loaded successfully and the script is executed,
     *    the virtual machine calls the exec() method of the respective built-in
     *    function to provide the actual functionality of the built-in function
     *    call.
     */
    class VMFunction {
    public:
        /**
         * Script data type of the function's return value. If the function does
         * not return any value (void), then it returns EMPTY_EXPR here.
         */
        virtual ExprType_t returnType() = 0;

        /**
         * Minimum amount of function arguments this function accepts. If a
         * script is calling this function with less arguments, the script
         * parser will throw a parser error.
         */
        virtual int minRequiredArgs() const = 0;

        /**
         * Maximum amount of function arguments this functions accepts. If a
         * script is calling this function with more arguments, the script
         * parser will throw a parser error.
         */
        virtual int maxAllowedArgs() const = 0;

        /**
         * Script data type of the function's @c iArg 'th function argument.
         * The information provided here is less strong than acceptsArgType().
         * The parser will compare argument data types provided in scripts by
         * calling acceptsArgType(). The return value of argType() is used by the
         * parser instead to show an appropriate parser error which data type
         * this function usually expects as "default" data type. Reason: a
         * function may accept multiple data types for a certain function
         * argument and would automatically cast the passed argument value in
         * that case to the type it actually needs.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         */
        virtual ExprType_t argType(int iArg) const = 0;

        /**
         * This method is called by the parser to check whether arguments
         * passed in scripts to this function are accepted by this function. If
         * a script calls this function with an argument's data type not
         * accepted by this function, the parser will throw a parser error. On
         * such errors the data type returned by argType() will be used to
         * assemble an appropriate error message regarding the precise misusage
         * of the built-in function.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         * @param type - script data type used for this function argument by
         *               currently parsed script
         * @return true if the given data type would be accepted for the
         *         respective function argument by the function
         */
        virtual bool acceptsArgType(int iArg, ExprType_t type) const = 0;

        /**
         * This method is called by the parser to check whether some arguments
         * (and if yes which ones) passed to this script function will be
         * modified by this script function. Most script functions simply use
         * their arguments as inputs, that is they only read the argument's
         * values. However some script function may also use passed
         * argument(s) as output variables. In this case the function
         * implementation must return @c true for the respective argument
         * index here.
         *
         * @param iArg - index of the function argument in question
         *               (must be between 0 .. maxAllowedArgs() - 1)
         */
        virtual bool modifiesArg(int iArg) const = 0;

        /**
         * Implements the actual function execution. This exec() method is
         * called by the VM whenever this function implementation shall be
         * executed at script runtime. This method blocks until the function
         * call completed.
         *
         * @param args - function arguments for executing this built-in function
         * @returns function's return value (if any) and general status
         *          informations (i.e. whether the function call caused a
         *          runtime error)
         */
        virtual VMFnResult* exec(VMFnArgs* args) = 0;

        /**
         * Convenience method for function implementations to show warning
         * messages during actual execution of the built-in function.
         *
         * @param txt - runtime warning text to be shown to user
         */
        void wrnMsg(const String& txt);

        /**
         * Convenience method for function implementations to show error
         * messages during actual execution of the built-in function.
         *
         * @param txt - runtime error text to be shown to user
         */
        void errMsg(const String& txt);
    };

    /** @brief Virtual machine relative pointer.
     *
     * POD base of VMIntRelPtr and VMInt8RelPtr structures. Not intended to be
     * used directly. Use VMIntRelPtr or VMInt8RelPtr instead.
     *
     * @see VMIntRelPtr, VMInt8RelPtr
     */
    struct VMRelPtr {
        void** base; ///< Base pointer.
        int offset;  ///< Offset (in bytes) relative to base pointer.
        bool readonly; ///< Whether the pointed data may be modified or just be read.
    };

    /** @brief Pointer to built-in VM integer variable (of C/C++ type int).
     *
     * Used for defining built-in 32 bit integer script variables.
     *
     * @b CAUTION: You may only use this class for pointing to C/C++ variables
     * of type "int" (which on most systems is 32 bit in size). If the C/C++ int
     * variable you want to reference is only 8 bit in size, then you @b must
     * use VMInt8RelPtr instead!
     *
     * For efficiency reasons the actual native C/C++ int variable is referenced
     * by two components here. The actual native int C/C++ variable in memory
     * is dereferenced at VM run-time by taking the @c base pointer dereference
     * and adding @c offset bytes. This has the advantage that for a large
     * number of built-in int variables, only one (or few) base pointer need
     * to be re-assigned before running a script, instead of updating each
     * built-in variable each time before a script is executed.
     *
     * Refer to DECLARE_VMINT() for example code.
     *
     * @see VMInt8RelPtr, DECLARE_VMINT()
     */
    struct VMIntRelPtr : VMRelPtr {
        VMIntRelPtr() {
            base   = NULL;
            offset = 0;
            readonly = false;
        }
        VMIntRelPtr(const VMRelPtr& data) {
            base   = data.base;
            offset = data.offset;
            readonly = false;
        }
        virtual int evalInt() { return *(int*)&(*(uint8_t**)base)[offset]; }
        virtual void assign(int i) { *(int*)&(*(uint8_t**)base)[offset] = i; }
    };

    /** @brief Pointer to built-in VM integer variable (of C/C++ type int8_t).
     *
     * Used for defining built-in 8 bit integer script variables.
     *
     * @b CAUTION: You may only use this class for pointing to C/C++ variables
     * of type "int8_t" (8 bit integer). If the C/C++ int variable you want to
     * reference is an "int" type (which is 32 bit on most systems), then you
     * @b must use VMIntRelPtr instead!
     *
     * For efficiency reasons the actual native C/C++ int variable is referenced
     * by two components here. The actual native int C/C++ variable in memory
     * is dereferenced at VM run-time by taking the @c base pointer dereference
     * and adding @c offset bytes. This has the advantage that for a large
     * number of built-in int variables, only one (or few) base pointer need
     * to be re-assigned before running a script, instead of updating each
     * built-in variable each time before a script is executed.
     *
     * Refer to DECLARE_VMINT() for example code.
     *
     * @see VMIntRelPtr, DECLARE_VMINT()
     */
    struct VMInt8RelPtr : VMIntRelPtr {
        VMInt8RelPtr() : VMIntRelPtr() {}
        VMInt8RelPtr(const VMRelPtr& data) : VMIntRelPtr(data) {}
        virtual int evalInt() OVERRIDE {
            return *(uint8_t*)&(*(uint8_t**)base)[offset];
        }
        virtual void assign(int i) OVERRIDE {
            *(uint8_t*)&(*(uint8_t**)base)[offset] = i;
        }
    };

    #if HAVE_CXX_EMBEDDED_PRAGMA_DIAGNOSTICS
    # define COMPILER_DISABLE_OFFSETOF_WARNING                    \
        _Pragma("GCC diagnostic push")                            \
        _Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"")
    # define COMPILER_RESTORE_OFFSETOF_WARNING \
        _Pragma("GCC diagnostic pop")
    #else
    # define COMPILER_DISABLE_OFFSETOF_WARNING
    # define COMPILER_RESTORE_OFFSETOF_WARNING
    #endif

    /**
     * Convenience macro for initializing VMIntRelPtr and VMInt8RelPtr
     * structures. Usage example:
     * @code
     * struct Foo {
     *   uint8_t a; // native representation of a built-in integer script variable
     *   int b; // native representation of another built-in integer script variable
     *   int c; // native representation of another built-in integer script variable
     *   uint8_t d; // native representation of another built-in integer script variable
     * };
     *
     * // initializing the built-in script variables to some values
     * Foo foo1 = (Foo) { 1, 2000, 3000, 4 };
     * Foo foo2 = (Foo) { 5, 6000, 7000, 8 };
     *
     * Foo* pFoo;
     *
     * VMInt8RelPtr varA = DECLARE_VMINT(pFoo, class Foo, a);
     * VMIntRelPtr  varB = DECLARE_VMINT(pFoo, class Foo, b);
     * VMIntRelPtr  varC = DECLARE_VMINT(pFoo, class Foo, c);
     * VMInt8RelPtr varD = DECLARE_VMINT(pFoo, class Foo, d);
     *
     * pFoo = &foo1;
     * printf("%d\n", varA->evalInt()); // will print 1
     * printf("%d\n", varB->evalInt()); // will print 2000
     * printf("%d\n", varC->evalInt()); // will print 3000
     * printf("%d\n", varD->evalInt()); // will print 4
     *
     * // same printf() code, just with pFoo pointer being changed ...
     *
     * pFoo = &foo2;
     * printf("%d\n", varA->evalInt()); // will print 5
     * printf("%d\n", varB->evalInt()); // will print 6000
     * printf("%d\n", varC->evalInt()); // will print 7000
     * printf("%d\n", varD->evalInt()); // will print 8
     * @endcode
     * As you can see above, by simply changing one single pointer, you can
     * remap a huge bunch of built-in integer script variables to completely
     * different native values/native variables. Which especially reduces code
     * complexity inside the sampler engines which provide the actual script
     * functionalities.
     */
    #define DECLARE_VMINT(basePtr, T_struct, T_member) (          \
        /* Disable offsetof warning, trust us, we are cautios. */ \
        COMPILER_DISABLE_OFFSETOF_WARNING                         \
        (VMRelPtr) {                                              \
            (void**) &basePtr,                                    \
            offsetof(T_struct, T_member),                         \
            false                                                 \
        }                                                         \
        COMPILER_RESTORE_OFFSETOF_WARNING                         \
    )                                                             \

    /**
     * Same as DECLARE_VMINT(), but this one defines the VMIntRelPtr and
     * VMInt8RelPtr structures to be of read-only type. That means the script
     * parser will abort any script at parser time if the script is trying to
     * modify such a read-only built-in variable.
     *
     * @b NOTE: this is only intended for built-in read-only variables that
     * may change during runtime! If your built-in variable's data is rather
     * already available at parser time and won't change during runtime, then
     * you should rather register a built-in constant in your VM class instead!
     *
     * @see ScriptVM::builtInConstIntVariables()
     */
    #define DECLARE_VMINT_READONLY(basePtr, T_struct, T_member) ( \
        /* Disable offsetof warning, trust us, we are cautios. */ \
        COMPILER_DISABLE_OFFSETOF_WARNING                         \
        (VMRelPtr) {                                              \
            (void**) &basePtr,                                    \
            offsetof(T_struct, T_member),                         \
            true                                                  \
        }                                                         \
        COMPILER_RESTORE_OFFSETOF_WARNING                         \
    )                                                             \

    /** @brief Built-in VM 8 bit integer array variable.
     *
     * Used for defining built-in integer array script variables (8 bit per
     * array element). Currently there is no support for any other kind of array
     * type. So all integer arrays of scripts use 8 bit data types.
     */
    struct VMInt8Array {
        int8_t* data;
        int size;
        bool readonly; ///< Whether the array data may be modified or just be read.

        VMInt8Array() : data(NULL), size(0), readonly(false) {}
    };

    /** @brief Virtual machine script variable.
     *
     * Common interface for all variables accessed in scripts.
     */
    class VMVariable : virtual public VMExpr {
    public:
        /**
         * Whether a script may modify the content of this variable by
         * assigning a new value to it.
         *
         * @see isConstExpr(), assign()
         */
        virtual bool isAssignable() const = 0;

        /**
         * In case this variable is assignable, this method will be called to
         * perform the value assignment to this variable with @a expr
         * reflecting the new value to be assigned.
         *
         * @param expr - new value to be assigned to this variable
         */
        virtual void assignExpr(VMExpr* expr) = 0;
    };
    
    /** @brief Dynamically executed variable (abstract base class).
     *
     * Interface for the implementation of a dynamically generated content of
     * a built-in script variable. Most built-in variables are simply pointers
     * to some native location in memory. So when a script reads them, the
     * memory location is simply read to get the value of the variable. A
     * dynamic variable however is not simply a memory location. For each access
     * to a dynamic variable some native code is executed to actually generate
     * and provide the content (value) of this type of variable.
     */
    class VMDynVar : public VMVariable {
    public:
        /**
         * Returns true in case this dynamic variable can be considered to be a
         * constant expression. A constant expression will retain the same value
         * throughout the entire life time of a script and the expression's
         * constant value may be evaluated already at script parse time, which
         * may result in performance benefits during script runtime.
         *
         * However due to the "dynamic" behavior of dynamic variables, almost
         * all dynamic variables are probably not constant expressions. That's
         * why this method returns @c false by default. If you are really sure
         * that your dynamic variable implementation can be considered a
         * constant expression then you may override this method and return
         * @c true instead. Note that when you return @c true here, your
         * dynamic variable will really just be executed once; and exectly
         * already when the script is loaded!
         *
         * As an example you may implement a "constant" built-in dynamic
         * variable that checks for a certain operating system feature and
         * returns the result of that OS feature check as content (value) of
         * this dynamic variable. Since the respective OS feature might become
         * available/unavailable after OS updates, software migration, etc. the
         * OS feature check should at least be performed once each time the
         * application is launched. And since the OS feature check might take a
         * certain amount of execution time, it might make sense to only
         * perform the check if the respective variable name is actually
         * referenced at all in the script to be loaded. Note that the dynamic
         * variable will still be evaluated again though if the script is
         * loaded again. So it is up to you to probably cache the result in the
         * implementation of your dynamic variable.
         *
         * On doubt, please rather consider to use a constant built-in script
         * variable instead of implementing a "constant" dynamic variable, due
         * to the runtime overhead a dynamic variable may cause.
         *
         * @see isAssignable()
         */
        bool isConstExpr() const OVERRIDE { return false; }

        /**
         * In case this dynamic variable is assignable, the new value (content)
         * to be assigned to this dynamic variable.
         *
         * By default this method does nothing. Override and implement this
         * method in your subclass in case your dynamic variable allows to
         * assign a new value by script.
         *
         * @param expr - new value to be assigned to this variable
         */
        void assignExpr(VMExpr* expr) OVERRIDE {}

        virtual ~VMDynVar() {}
    };

    /** @brief Dynamically executed variable (of integer data type).
     *
     * This is the base class for all built-in integer script variables whose
     * variable content needs to be provided dynamically by executable native
     * code on each script variable access.
     */
    class VMDynIntVar : virtual public VMDynVar, virtual public VMIntExpr {
    public:
    };

    /** @brief Dynamically executed variable (of string data type).
     *
     * This is the base class for all built-in string script variables whose
     * variable content needs to be provided dynamically by executable native
     * code on each script variable access.
     */
    class VMDynStringVar : virtual public VMDynVar, virtual public VMStringExpr {
    public:
    };

    /** @brief Dynamically executed variable (of integer array data type).
     *
     * This is the base class for all built-in integer array script variables
     * whose variable content needs to be provided dynamically by executable
     * native code on each script variable access.
     */
    class VMDynIntArrayVar : virtual public VMDynVar, virtual public VMIntArrayExpr {
    public:
    };

    /** @brief Provider for built-in script functions and variables.
     *
     * Abstract base class defining the high-level interface for all classes
     * which add and implement built-in script functions and built-in script
     * variables.
     */
    class VMFunctionProvider {
    public:
        /**
         * Returns pointer to the built-in function with the given function
         * @a name, or NULL if there is no built-in function with that function
         * name.
         *
         * @param name - function name (i.e. "wait" or "message" or "exit", etc.)
         */
        virtual VMFunction* functionByName(const String& name) = 0;

        /**
         * Returns @c true if the passed built-in function is disabled and
         * should be ignored by the parser. This method is called by the
         * parser on preprocessor level for each built-in function call within
         * a script. Accordingly if this method returns @c true, then the
         * respective function call is completely filtered out on preprocessor
         * level, so that built-in function won't make into the result virtual
         * machine representation, nor would expressions of arguments passed to
         * that built-in function call be evaluated, nor would any check
         * regarding correct usage of the built-in function be performed.
         * In other words: a disabled function call ends up as a comment block.
         *
         * @param fn - built-in function to be checked
         * @param ctx - parser context at the position where the built-in
         *              function call is located within the script
         */
        virtual bool isFunctionDisabled(VMFunction* fn, VMParserContext* ctx) = 0;

        /**
         * Returns a variable name indexed map of all built-in script variables
         * which point to native "int" scalar (usually 32 bit) variables.
         */
        virtual std::map<String,VMIntRelPtr*> builtInIntVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in script integer
         * array variables with array element type "int8_t" (8 bit).
         */
        virtual std::map<String,VMInt8Array*> builtInIntArrayVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in constant script
         * variables, which never change their value at runtime.
         */
        virtual std::map<String,int> builtInConstIntVariables() = 0;

        /**
         * Returns a variable name indexed map of all built-in dynamic variables,
         * which are not simply data stores, rather each one of them executes
         * natively to provide or alter the respective script variable data.
         */
        virtual std::map<String,VMDynVar*> builtInDynamicVariables() = 0;
    };

    /** @brief Execution state of a virtual machine.
     *
     * An instance of this abstract base class represents exactly one execution
     * state of a virtual machine. This encompasses most notably the VM
     * execution stack, and VM polyphonic variables. It does not contain global
     * variables. Global variables are contained in the VMParserContext object.
     * You might see a VMExecContext object as one virtual thread of the virtual
     * machine.
     *
     * In contrast to a VMParserContext, a VMExecContext is not tied to a
     * ScriptVM instance. Thus you can use a VMExecContext with different
     * ScriptVM instances, however not concurrently at the same time.
     *
     * @see VMParserContext
     */
    class VMExecContext {
    public:
        virtual ~VMExecContext() {}

        /**
         * In case the script was suspended for some reason, this method returns
         * the amount of microseconds before the script shall continue its
         * execution. Note that the virtual machine itself does never put its
         * own execution thread(s) to sleep. So the respective class (i.e. sampler
         * engine) which is using the virtual machine classes here, must take
         * care by itself about taking time stamps, determining the script
         * handlers that shall be put aside for the requested amount of
         * microseconds, indicated by this method by comparing the time stamps in
         * real-time, and to continue passing the respective handler to
         * ScriptVM::exec() as soon as its suspension exceeded, etc. Or in other
         * words: all classes in this directory never have an idea what time it
         * is.
         *
         * You should check the return value of ScriptVM::exec() to determine
         * whether the script was actually suspended before calling this method
         * here.
         *
         * @see ScriptVM::exec()
         */
        virtual int suspensionTimeMicroseconds() const = 0;

        /**
         * Causes all polyphonic variables to be reset to zero values. A
         * polyphonic variable is expected to be zero when entering a new event
         * handler instance. As an exception the values of polyphonic variables
         * shall only be preserved from an note event handler instance to its
         * correspending specific release handler instance. So in the latter
         * case the script author may pass custom data from the note handler to
         * the release handler, but only for the same specific note!
         */
        virtual void resetPolyphonicData() = 0;

        /**
         * Returns amount of virtual machine instructions which have been
         * performed the last time when this execution context was executing a
         * script. So in case you need the overall amount of instructions
         * instead, then you need to add them by yourself after each
         * ScriptVM::exec() call.
         */
        virtual size_t instructionsPerformed() const = 0;

        /**
         * Sends a signal to this script execution instance to abort its script
         * execution as soon as possible. This method is called i.e. when one
         * script execution instance intends to stop another script execution
         * instance.
         */
        virtual void signalAbort() = 0;

        /**
         * Copies the current entire execution state from this object to the
         * given object. So this can be used to "fork" a new script thread which
         * then may run independently with its own polyphonic data for instance.
         */
        virtual void forkTo(VMExecContext* ectx) const = 0;
    };

    /** @brief Script callback for a certain event.
     *
     * Represents a script callback for a certain event, i.e.
     * "on note ... end on" code block.
     */
    class VMEventHandler {
    public:
        /**
         * Type of this event handler, which identifies its purpose. For example
         * for a "on note ... end on" script callback block,
         * @c VM_EVENT_HANDLER_NOTE would be returned here.
         */
        virtual VMEventHandlerType_t eventHandlerType() const = 0;

        /**
         * Name of the event handler which identifies its purpose. For example
         * for a "on note ... end on" script callback block, the name "note"
         * would be returned here.
         */
        virtual String eventHandlerName() const = 0;

        /**
         * Whether or not the event handler makes any use of so called
         * "polyphonic" variables.
         */
        virtual bool isPolyphonic() const = 0;
    };

    /**
     * Reflects the precise position and span of a specific code block within
     * a script. This is currently only used for the locations of commented
     * code blocks due to preprocessor statements, and for parser errors and
     * parser warnings.
     *
     * @see ParserIssue for code locations of parser errors and parser warnings
     *
     * @see VMParserContext::preprocessorComments() for locations of code which
     *      have been filtered out by preprocessor statements
     */
    struct CodeBlock {
        int firstLine; ///< The first line number of this code block within the script (indexed with 1 being the very first line).
        int lastLine; ///< The last line number of this code block within the script.
        int firstColumn; ///< The first column of this code block within the script (indexed with 1 being the very first column).
        int lastColumn; ///< The last column of this code block within the script.
    };

    /**
     * Encapsulates a noteworty parser issue. This encompasses the type of the
     * issue (either a parser error or parser warning), a human readable
     * explanation text of the error or warning and the location of the
     * encountered parser issue within the script.
     *
     * @see VMSourceToken for processing syntax highlighting instead.
     */
    struct ParserIssue : CodeBlock {
        String txt; ///< Human readable explanation text of the parser issue.
        ParserIssueType_t type; ///< Whether this issue is either a parser error or just a parser warning.

        /**
         * Print this issue out to the console (stdio).
         */
        inline void dump() {
            switch (type) {
                case PARSER_ERROR:
                    printf("[ERROR] line %d, column %d: %s\n", firstLine, firstColumn, txt.c_str());
                    break;
                case PARSER_WARNING:
                    printf("[Warning] line %d, column %d: %s\n", firstLine, firstColumn, txt.c_str());
                    break;
            }
        }

        /**
         * Returns true if this issue is a parser error. In this case the parsed
         * script may not be executed!
         */
        inline bool isErr() const { return type == PARSER_ERROR;   }

        /**
         * Returns true if this issue is just a parser warning. A parsed script
         * that only raises warnings may be executed if desired, however the
         * script may not behave exactly as intended by the script author.
         */
        inline bool isWrn() const { return type == PARSER_WARNING; }
    };

    /**
     * Convenience function used for converting an ExprType_t constant to a
     * string, i.e. for generating error message by the parser.
     */
    inline String typeStr(const ExprType_t& type) {
        switch (type) {
            case EMPTY_EXPR: return "empty";
            case INT_EXPR: return "integer";
            case INT_ARR_EXPR: return "integer array";
            case STRING_EXPR: return "string";
            case STRING_ARR_EXPR: return "string array";
        }
        return "invalid";
    }

    /** @brief Virtual machine representation of a script.
     *
     * An instance of this abstract base class represents a parsed script,
     * translated into a virtual machine tree. You should first check if there
     * were any parser errors. If there were any parser errors, you should
     * refrain from executing the virtual machine. Otherwise if there were no
     * parser errors (i.e. only warnings), then you might access one of the
     * script's event handlers by i.e. calling eventHandlerByName() and pass the
     * respective event handler to the ScriptVM class (or to one of the ScriptVM
     * descendants) for execution.
     *
     * @see VMExecContext, ScriptVM
     */
    class VMParserContext {
    public:
        virtual ~VMParserContext() {}

        /**
         * Returns all noteworthy issues encountered when the script was parsed.
         * These are parser errors and parser warnings.
         */
        virtual std::vector<ParserIssue> issues() const = 0;

        /**
         * Same as issues(), but this method only returns parser errors.
         */
        virtual std::vector<ParserIssue> errors() const = 0;

        /**
         * Same as issues(), but this method only returns parser warnings.
         */
        virtual std::vector<ParserIssue> warnings() const = 0;

        /**
         * Returns all code blocks of the script which were filtered out by the
         * preprocessor.
         */
        virtual std::vector<CodeBlock> preprocessorComments() const = 0;

        /**
         * Returns the translated virtual machine representation of an event
         * handler block (i.e. "on note ... end on" code block) within the
         * parsed script. This translated representation of the event handler
         * can be executed by the virtual machine.
         *
         * @param index - index of the event handler within the script
         */
        virtual VMEventHandler* eventHandler(uint index) = 0;

        /**
         * Same as eventHandler(), but this method returns the event handler by
         * its name. So for a "on note ... end on" code block of the parsed
         * script you would pass "note" for argument @a name here.
         *
         * @param name - name of the event handler (i.e. "init", "note",
         *               "controller", "release")
         */
        virtual VMEventHandler* eventHandlerByName(const String& name) = 0;
    };

    class SourceToken;

    /** @brief Recognized token of a script's source code.
     *
     * Represents one recognized token of a script's source code, for example
     * a keyword, variable name, etc. and it provides further informations about
     * that particular token, i.e. the precise location (line and column) of the
     * token within the original script's source code.
     *
     * This class is not actually used by the sampler itself. It is rather
     * provided for external script editor applications. Primary purpose of
     * this class is syntax highlighting for external script editors.
     *
     * @see ParserIssue for processing compile errors and warnings instead.
     */
    class VMSourceToken {
    public:
        VMSourceToken();
        VMSourceToken(SourceToken* ct);
        VMSourceToken(const VMSourceToken& other);
        virtual ~VMSourceToken();

        // original text of this token as it is in the script's source code
        String text() const;

        // position of token in script
        int firstLine() const; ///< First line this source token is located at in script source code (indexed with 0 being the very first line). Most source code tokens are not spanning over multiple lines, the only current exception are comments, in the latter case you need to process text() to get the last line and last column for the comment.
        int firstColumn() const; ///< First column on the first line this source token is located at in script source code (indexed with 0 being the very first column). To get the length of this token use text().length().

        // base types
        bool isEOF() const; ///< Returns true in case this source token represents the end of the source code file.
        bool isNewLine() const; ///< Returns true in case this source token represents a line feed character (i.e. "\n" on Unix systems).
        bool isKeyword() const; ///< Returns true in case this source token represents a language keyword (i.e. "while", "function", "declare", "on", etc.).
        bool isVariableName() const; ///< Returns true in case this source token represents a variable name (i.e. "$someIntVariable", "%someArrayVariable", "\@someStringVariable"). @see isIntegerVariable(), isStringVariable(), isArrayVariable() for the precise variable type.
        bool isIdentifier() const; ///< Returns true in case this source token represents an identifier, which currently always means a function name.
        bool isNumberLiteral() const; ///< Returns true in case this source token represents a number literal (i.e. 123).
        bool isStringLiteral() const; ///< Returns true in case this source token represents a string literal (i.e. "Some text").
        bool isComment() const; ///< Returns true in case this source token represents a source code comment.
        bool isPreprocessor() const; ///< Returns true in case this source token represents a preprocessor statement.
        bool isOther() const; ///< Returns true in case this source token represents anything else not covered by the token types mentioned above.

        // extended types
        bool isIntegerVariable() const; ///< Returns true in case this source token represents an integer variable name (i.e. "$someIntVariable").
        bool isStringVariable() const; ///< Returns true in case this source token represents an string variable name (i.e. "\@someStringVariable").
        bool isArrayVariable() const; ///< Returns true in case this source token represents an array variable name (i.e. "%someArryVariable").
        bool isEventHandlerName() const; ///< Returns true in case this source token represents an event handler name (i.e. "note", "release", "controller").

        VMSourceToken& operator=(const VMSourceToken& other);

    private:
        SourceToken* m_token;
    };

} // namespace LinuxSampler

#endif // LS_INSTR_SCRIPT_PARSER_COMMON_H
