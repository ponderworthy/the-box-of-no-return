/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.3.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         InstrScript_parse
#define yylex           InstrScript_lex
#define yyerror         InstrScript_error
#define yydebug         InstrScript_debug
#define yynerrs         InstrScript_nerrs


/* First part of user prologue.  */
#line 12 "parser.y" /* yacc.c:337  */

    #define YYERROR_VERBOSE 1
    #include "parser_shared.h"
    #include <string>
    #include <map>
    using namespace LinuxSampler;

    void InstrScript_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err);
    void InstrScript_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt);
    int InstrScript_tnamerr(char* yyres, const char* yystr);
    int InstrScript_lex(YYSTYPE* lvalp, YYLTYPE* llocp, void* scanner);
    #define scanner context->scanner
    #define PARSE_ERR(loc,txt)  yyerror(&loc, context, txt)
    #define PARSE_WRN(loc,txt)  InstrScript_warning(&loc, context, txt)
    #define PARSE_DROP(loc)     context->addPreprocessorComment(loc.first_line, loc.last_line, loc.first_column+1, loc.last_column+1);
    #define yytnamerr(res,str)  InstrScript_tnamerr(res, str)

#line 94 "y.tab.c" /* yacc.c:337  */
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_INSTRSCRIPT_Y_TAB_H_INCLUDED
# define YY_INSTRSCRIPT_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int InstrScript_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END_OF_FILE = 0,
    INTEGER = 258,
    STRING = 259,
    IDENTIFIER = 260,
    VARIABLE = 261,
    ON = 262,
    END = 263,
    INIT = 264,
    NOTE = 265,
    RELEASE = 266,
    CONTROLLER = 267,
    DECLARE = 268,
    ASSIGNMENT = 269,
    CONST_ = 270,
    POLYPHONIC = 271,
    WHILE = 272,
    SYNCHRONIZED = 273,
    IF = 274,
    ELSE = 275,
    SELECT = 276,
    CASE = 277,
    TO = 278,
    OR = 279,
    AND = 280,
    NOT = 281,
    BITWISE_OR = 282,
    BITWISE_AND = 283,
    BITWISE_NOT = 284,
    FUNCTION = 285,
    CALL = 286,
    MOD = 287,
    LE = 288,
    GE = 289,
    UNKNOWN_CHAR = 290
  };
#endif
/* Tokens.  */
#define END_OF_FILE 0
#define INTEGER 258
#define STRING 259
#define IDENTIFIER 260
#define VARIABLE 261
#define ON 262
#define END 263
#define INIT 264
#define NOTE 265
#define RELEASE 266
#define CONTROLLER 267
#define DECLARE 268
#define ASSIGNMENT 269
#define CONST_ 270
#define POLYPHONIC 271
#define WHILE 272
#define SYNCHRONIZED 273
#define IF 274
#define ELSE 275
#define SELECT 276
#define CASE 277
#define TO 278
#define OR 279
#define AND 280
#define NOT 281
#define BITWISE_OR 282
#define BITWISE_AND 283
#define BITWISE_NOT 284
#define FUNCTION 285
#define CALL 286
#define MOD 287
#define LE 288
#define GE 289
#define UNKNOWN_CHAR 290

/* Value type.  */

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int InstrScript_parse (LinuxSampler::ParserContext* context);

#endif /* !YY_INSTRSCRIPT_Y_TAB_H_INCLUDED  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  13
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   177

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  77
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  171

#define YYUNDEFTOK  2
#define YYMAXUTOK   290

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    46,     2,     2,    42,     2,
      38,    39,    48,    47,    40,    41,     2,    49,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      43,    45,    44,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    36,     2,    37,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    91,    91,    96,   100,   106,   109,   114,   120,   126,
     132,   140,   152,   155,   160,   167,   176,   179,   182,   195,
     208,   243,   267,   313,   369,   399,   402,   410,   413,   416,
     419,   429,   433,   439,   444,   452,   467,   503,   524,   547,
     551,   557,   560,   574,   599,   602,   605,   615,   636,   639,
     642,   645,   653,   663,   666,   667,   680,   681,   696,   699,
     714,   715,   730,   733,   748,   749,   762,   775,   788,   801,
     804,   809,   810,   823,   838,   839,   852,   865
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "$undefined", "\"integer literal\"",
  "\"string literal\"", "\"function name\"", "\"variable name\"",
  "\"keyword 'on'\"", "\"keyword 'end'\"", "\"keyword 'init'\"",
  "\"keyword 'note'\"", "\"keyword 'release'\"",
  "\"keyword 'controller'\"", "\"keyword 'declare'\"", "\"operator ':='\"",
  "\"keyword 'const'\"", "\"keyword 'polyphonic'\"", "\"keyword 'while'\"",
  "\"keyword 'synchronized'\"", "\"keyword 'if'\"", "\"keyword 'else'\"",
  "\"keyword 'select'\"", "\"keyword 'case'\"", "\"keyword 'to'\"",
  "\"operator 'or'\"", "\"operator 'and'\"", "\"operator 'not'\"",
  "\"bitwise operator '.or.'\"", "\"bitwise operator '.and.'\"",
  "\"bitwise operator '.not.'\"", "\"keyword 'function'\"",
  "\"keyword 'call'\"", "\"operator 'mod'\"", "\"operator '<='\"",
  "\"operator '>='\"", "\"unknown character\"", "'['", "']'", "'('", "')'",
  "','", "'-'", "'&'", "'<'", "'>'", "'='", "'#'", "'+'", "'*'", "'/'",
  "$accept", "script", "sections", "section", "eventhandler",
  "function_declaration", "opt_statements", "statements", "statement",
  "caseclauses", "caseclause", "userfunctioncall", "functioncall", "args",
  "arg", "assignment", "unary_expr", "expr", "concat_expr",
  "logical_or_expr", "logical_and_expr", "bitwise_or_expr",
  "bitwise_and_expr", "rel_expr", "add_expr", "mul_expr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,    91,    93,    40,    41,
      44,    45,    38,    60,    62,    61,    35,    43,    42,    47
};
# endif

#define YYPACT_NINF -107

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-107)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       6,   121,     9,    25,     6,  -107,  -107,  -107,    59,    59,
      59,    59,    59,  -107,  -107,    18,     5,    79,    22,    59,
      33,    78,    34,     2,    59,  -107,  -107,  -107,  -107,    36,
      39,    50,    71,    28,    78,    78,     7,    24,    87,    78,
     101,    78,  -107,  -107,    75,    78,    78,    78,    78,  -107,
    -107,    93,    76,   102,   103,   100,   108,    55,     1,    38,
    -107,   130,  -107,   131,   132,   133,   111,  -107,    52,  -107,
    -107,  -107,   105,    78,    78,    10,  -107,   104,   126,   106,
      78,  -107,  -107,   107,  -107,   144,     4,  -107,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,  -107,  -107,  -107,  -107,  -107,  -107,
      78,   136,  -107,   114,    78,    78,    59,  -107,    59,   117,
    -107,    32,   134,  -107,   102,   103,   100,   108,    55,     1,
       1,     1,     1,     1,     1,    38,    38,  -107,  -107,  -107,
    -107,    78,   142,  -107,   120,   150,    20,  -107,   156,  -107,
    -107,  -107,   122,   147,   145,   146,    59,    59,    78,   125,
    -107,  -107,   158,  -107,    57,    78,   148,  -107,    66,  -107,
    -107
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     2,     3,     6,     5,    12,    12,
      12,    12,    12,     1,     4,    38,     0,     0,     0,    12,
       0,     0,     0,     0,    13,    14,    17,    16,    25,     0,
       0,     0,     0,     0,     0,     0,    18,     0,     0,     0,
       0,     0,    44,    45,    46,     0,     0,     0,     0,    49,
      74,     0,    53,    54,    56,    58,    60,    62,    64,    71,
      35,     0,    15,     0,     0,     0,     0,    37,     0,    39,
      41,    42,     0,     0,     0,     0,    19,     0,     0,     0,
       0,    52,    51,     0,    50,     0,     0,    31,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     7,     9,    10,    11,    36,
       0,     0,    20,     0,     0,     0,    12,    27,    12,     0,
      48,    12,     0,    32,    55,    57,    59,    61,    63,    67,
      68,    65,    66,    69,    70,    73,    72,    77,    75,    76,
      40,     0,    21,    24,     0,     0,     0,    47,     0,    33,
      30,    43,     0,     0,     0,     0,    12,    12,     0,     0,
      26,    29,     0,    34,     0,     0,     0,    22,     0,    28,
      23
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -107,  -107,  -107,   160,  -107,  -107,    -4,  -107,   149,  -107,
      82,  -107,    -8,  -106,    60,  -107,   -28,   -12,  -107,    81,
      83,    84,    80,    85,    27,    35
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     3,     4,     5,     6,     7,    23,    24,    25,    86,
      87,    26,    49,    68,    69,    28,    50,    70,    52,    53,
      54,    55,    56,    57,    58,    59
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      27,    27,    27,    27,    27,    29,    30,    31,    32,    51,
      61,    27,   122,     1,    12,    40,    27,    81,    82,    34,
      84,    73,    71,    72,   114,    13,    85,    77,   155,    79,
      75,    42,    43,    15,    44,    83,     2,    15,    16,    60,
     156,    35,    99,    74,    63,    17,   115,    64,   100,    18,
      19,    20,   164,    21,    45,   148,    33,    46,    65,   168,
      39,   112,   113,    22,    15,    16,    47,    67,   119,    48,
     101,    41,    17,   137,   138,   139,    18,    19,    20,    66,
      21,    42,    43,    15,    44,    36,   102,   103,    93,    94,
      22,   109,   110,    76,    37,    38,   167,   110,    95,    96,
      97,    98,   143,   144,    45,   170,   110,    46,    27,    78,
      27,    80,   145,    27,   146,    85,    47,   149,    88,    48,
     129,   130,   131,   132,   133,   134,    89,    91,    90,   151,
       8,     9,    10,    11,   135,   136,    92,   104,   105,   106,
     107,   108,   111,   116,   117,   118,   120,   121,    27,    27,
     141,   142,   162,   163,   147,   150,   152,   153,   154,   157,
     158,   159,   160,   165,    14,   161,   166,   169,   123,   124,
     140,   127,   125,    62,   126,     0,     0,   128
};

static const yytype_int16 yycheck[] =
{
       8,     9,    10,    11,    12,     9,    10,    11,    12,    21,
       8,    19,     8,     7,     5,    19,    24,    45,    46,    14,
      48,    14,    34,    35,    14,     0,    22,    39,     8,    41,
       6,     3,     4,     5,     6,    47,    30,     5,     6,     5,
      20,    36,    41,    36,     8,    13,    36,     8,    47,    17,
      18,    19,   158,    21,    26,    23,    38,    29,     8,   165,
      38,    73,    74,    31,     5,     6,    38,    39,    80,    41,
      32,    38,    13,   101,   102,   103,    17,    18,    19,     8,
      21,     3,     4,     5,     6,     6,    48,    49,    33,    34,
      31,    39,    40,     6,    15,    16,    39,    40,    43,    44,
      45,    46,   114,   115,    26,    39,    40,    29,   116,     8,
     118,    36,   116,   121,   118,    22,    38,   121,    42,    41,
      93,    94,    95,    96,    97,    98,    24,    27,    25,   141,
       9,    10,    11,    12,    99,   100,    28,     7,     7,     7,
       7,    30,    37,    39,    18,    39,    39,     3,   156,   157,
      14,    37,   156,   157,    37,    21,    14,    37,     8,     3,
      38,    14,    17,    38,     4,    19,     8,    19,    86,    88,
     110,    91,    89,    24,    90,    -1,    -1,    92
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    30,    51,    52,    53,    54,    55,     9,    10,
      11,    12,     5,     0,    53,     5,     6,    13,    17,    18,
      19,    21,    31,    56,    57,    58,    61,    62,    65,    56,
      56,    56,    56,    38,    14,    36,     6,    15,    16,    38,
      56,    38,     3,     4,     6,    26,    29,    38,    41,    62,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
       5,     8,    58,     8,     8,     8,     8,    39,    63,    64,
      67,    67,    67,    14,    36,     6,     6,    67,     8,    67,
      36,    66,    66,    67,    66,    22,    59,    60,    42,    24,
      25,    27,    28,    33,    34,    43,    44,    45,    46,    41,
      47,    32,    48,    49,     7,     7,     7,     7,    30,    39,
      40,    37,    67,    67,    14,    36,    39,    18,    39,    67,
      39,     3,     8,    60,    69,    70,    71,    72,    73,    74,
      74,    74,    74,    74,    74,    75,    75,    66,    66,    66,
      64,    14,    37,    67,    67,    56,    56,    37,    23,    56,
      21,    67,    14,    37,     8,     8,    20,     3,    38,    14,
      17,    19,    56,    56,    63,    38,     8,    39,    63,    19,
      39
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    50,    51,    52,    52,    53,    53,    54,    54,    54,
      54,    55,    56,    56,    57,    57,    58,    58,    58,    58,
      58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
      58,    59,    59,    60,    60,    61,    62,    62,    62,    63,
      63,    64,    65,    65,    66,    66,    66,    66,    66,    66,
      66,    66,    66,    67,    68,    68,    69,    69,    70,    70,
      71,    71,    72,    72,    73,    73,    73,    73,    73,    73,
      73,    74,    74,    74,    75,    75,    75,    75
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     5,     5,     5,
       5,     5,     0,     1,     1,     2,     1,     1,     2,     3,
       4,     5,     9,    10,     5,     1,     7,     4,     9,     7,
       5,     1,     2,     3,     5,     2,     4,     3,     1,     1,
       3,     1,     3,     6,     1,     1,     1,     4,     3,     1,
       2,     2,     2,     1,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     3,     3,     3,     3,
       3,     1,     3,     3,     1,     3,     3,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, context, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, context); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, LinuxSampler::ParserContext* context)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (yylocationp);
  YYUSE (context);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, LinuxSampler::ParserContext* context)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyo, *yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yytype, yyvaluep, yylocationp, context);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, LinuxSampler::ParserContext* context)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , context);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, context); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, LinuxSampler::ParserContext* context)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (context);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (LinuxSampler::ParserContext* context)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 91 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nEventHandlers) = context->handlers = (yyvsp[0].nEventHandlers);
    }
#line 1533 "y.tab.c" /* yacc.c:1652  */
    break;

  case 3:
#line 96 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nEventHandlers) = new EventHandlers();
        if ((yyvsp[0].nEventHandler)) (yyval.nEventHandlers)->add((yyvsp[0].nEventHandler));
    }
#line 1542 "y.tab.c" /* yacc.c:1652  */
    break;

  case 4:
#line 100 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nEventHandlers) = (yyvsp[-1].nEventHandlers);
        if ((yyvsp[0].nEventHandler)) (yyval.nEventHandlers)->add((yyvsp[0].nEventHandler));
    }
#line 1551 "y.tab.c" /* yacc.c:1652  */
    break;

  case 5:
#line 106 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nEventHandler) = EventHandlerRef();
    }
#line 1559 "y.tab.c" /* yacc.c:1652  */
    break;

  case 6:
#line 109 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nEventHandler) = (yyvsp[0].nEventHandler);
    }
#line 1567 "y.tab.c" /* yacc.c:1652  */
    break;

  case 7:
#line 114 "parser.y" /* yacc.c:1652  */
    {
        if (context->onNote)
            PARSE_ERR((yylsp[-3]), "Redeclaration of 'note' event handler.");
        context->onNote = new OnNote((yyvsp[-2].nStatements));
        (yyval.nEventHandler) = context->onNote;
    }
#line 1578 "y.tab.c" /* yacc.c:1652  */
    break;

  case 8:
#line 120 "parser.y" /* yacc.c:1652  */
    {
        if (context->onInit)
            PARSE_ERR((yylsp[-3]), "Redeclaration of 'init' event handler.");
        context->onInit = new OnInit((yyvsp[-2].nStatements));
        (yyval.nEventHandler) = context->onInit;
    }
#line 1589 "y.tab.c" /* yacc.c:1652  */
    break;

  case 9:
#line 126 "parser.y" /* yacc.c:1652  */
    {
        if (context->onRelease)
            PARSE_ERR((yylsp[-3]), "Redeclaration of 'release' event handler.");
        context->onRelease = new OnRelease((yyvsp[-2].nStatements));
        (yyval.nEventHandler) = context->onRelease;
    }
#line 1600 "y.tab.c" /* yacc.c:1652  */
    break;

  case 10:
#line 132 "parser.y" /* yacc.c:1652  */
    {
        if (context->onController)
            PARSE_ERR((yylsp[-3]), "Redeclaration of 'controller' event handler.");
        context->onController = new OnController((yyvsp[-2].nStatements));
        (yyval.nEventHandler) = context->onController;
    }
#line 1611 "y.tab.c" /* yacc.c:1652  */
    break;

  case 11:
#line 140 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-3].sValue);
        if (context->functionProvider->functionByName(name)) {
            PARSE_ERR((yylsp[-3]), (String("There is already a built-in function with name '") + name + "'.").c_str());
        } else if (context->userFunctionByName(name)) {
            PARSE_ERR((yylsp[-3]), (String("There is already a user defined function with name '") + name + "'.").c_str());
        } else {
            context->userFnTable[name] = (yyvsp[-2].nStatements);
        }
    }
#line 1626 "y.tab.c" /* yacc.c:1652  */
    break;

  case 12:
#line 152 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatements) = new Statements();
    }
#line 1634 "y.tab.c" /* yacc.c:1652  */
    break;

  case 13:
#line 155 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatements) = (yyvsp[0].nStatements);
    }
#line 1642 "y.tab.c" /* yacc.c:1652  */
    break;

  case 14:
#line 160 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatements) = new Statements();
        if ((yyvsp[0].nStatement)) {
            if (!isNoOperation((yyvsp[0].nStatement))) (yyval.nStatements)->add((yyvsp[0].nStatement)); // filter out NoOperation statements
        } else 
            PARSE_WRN((yylsp[0]), "Not a statement.");
    }
#line 1654 "y.tab.c" /* yacc.c:1652  */
    break;

  case 15:
#line 167 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatements) = (yyvsp[-1].nStatements);
        if ((yyvsp[0].nStatement)) {
            if (!isNoOperation((yyvsp[0].nStatement))) (yyval.nStatements)->add((yyvsp[0].nStatement)); // filter out NoOperation statements
        } else
            PARSE_WRN((yylsp[0]), "Not a statement.");
    }
#line 1666 "y.tab.c" /* yacc.c:1652  */
    break;

  case 16:
#line 176 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatement) = (yyvsp[0].nFunctionCall);
    }
#line 1674 "y.tab.c" /* yacc.c:1652  */
    break;

  case 17:
#line 179 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatement) = (yyvsp[0].nStatements);
    }
#line 1682 "y.tab.c" /* yacc.c:1652  */
    break;

  case 18:
#line 182 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[0].sValue);
        //printf("declared var '%s'\n", name);
        if (context->variableByName(name))
            PARSE_ERR((yylsp[0]), (String("Redeclaration of variable '") + name + "'.").c_str());
        if (name[0] == '@') {
            context->vartable[name] = new StringVariable(context);
            (yyval.nStatement) = new NoOperation;
        } else {
            context->vartable[name] = new IntVariable(context);
            (yyval.nStatement) = new NoOperation;
        }
    }
#line 1700 "y.tab.c" /* yacc.c:1652  */
    break;

  case 19:
#line 195 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[0].sValue);
        //printf("declared polyphonic var '%s'\n", name);
        if (context->variableByName(name))
            PARSE_ERR((yylsp[0]), (String("Redeclaration of variable '") + name + "'.").c_str());
        if (name[0] != '$') {
            PARSE_ERR((yylsp[0]), "Polyphonic variables may only be declared as integers.");
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            context->vartable[name] = new PolyphonicIntVariable(context);
            (yyval.nStatement) = new NoOperation;
        }
    }
#line 1718 "y.tab.c" /* yacc.c:1652  */
    break;

  case 20:
#line 208 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-2].sValue);
        //printf("declared assign var '%s'\n", name);
        if (context->variableByName(name))
            PARSE_ERR((yylsp[-2]), (String("Redeclaration of variable '") + name + "'.").c_str());
        if ((yyvsp[0].nExpression)->exprType() == STRING_EXPR) {
            if (name[0] == '$')
                PARSE_WRN((yylsp[-2]), (String("Variable '") + name + "' declared as integer, string expression assigned though.").c_str());
            StringExprRef expr = (yyvsp[0].nExpression);
            if (expr->isConstExpr()) {
                const String s = expr->evalStr();
                StringVariableRef var = new StringVariable(context);
                context->vartable[name] = var;
                (yyval.nStatement) = new Assignment(var, new StringLiteral(s));
            } else {
                StringVariableRef var = new StringVariable(context);
                context->vartable[name] = var;
                (yyval.nStatement) = new Assignment(var, expr);
            }
        } else {
            if (name[0] == '@')
                PARSE_WRN((yylsp[-2]), (String("Variable '") + name + "' declared as string, integer expression assigned though.").c_str());
            IntExprRef expr = (yyvsp[0].nExpression);
            if (expr->isConstExpr()) {
                const int i = expr->evalInt();
                IntVariableRef var = new IntVariable(context);
                context->vartable[name] = var;
                (yyval.nStatement) = new Assignment(var, new IntLiteral(i));
            } else {
                IntVariableRef var = new IntVariable(context);
                context->vartable[name] = var;
                (yyval.nStatement) = new Assignment(var, expr);
            }
        }
    }
#line 1758 "y.tab.c" /* yacc.c:1652  */
    break;

  case 21:
#line 243 "parser.y" /* yacc.c:1652  */
    {
        //printf("declare array without args\n");
        const char* name = (yyvsp[-3].sValue);
        if (!(yyvsp[-1].nExpression)->isConstExpr()) {
            PARSE_ERR((yylsp[-1]), (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if ((yyvsp[-1].nExpression)->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-1]), (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if (context->variableByName(name)) {
            PARSE_ERR((yylsp[-3]), (String("Redeclaration of variable '") + name + "'.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            IntExprRef expr = (yyvsp[-1].nExpression);
            int size = expr->evalInt();
            if (size <= 0) {
                PARSE_ERR((yylsp[-1]), (String("Array variable '") + name + "' declared with array size " + ToString(size) + ".").c_str());
                (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
            } else {
                context->vartable[name] = new IntArrayVariable(context, size);
                (yyval.nStatement) = new NoOperation;
            }
        }
    }
#line 1787 "y.tab.c" /* yacc.c:1652  */
    break;

  case 22:
#line 267 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-7].sValue);
        if (!(yyvsp[-5].nExpression)->isConstExpr()) {
            PARSE_ERR((yylsp[-5]), (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if ((yyvsp[-5].nExpression)->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-5]), (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if (context->variableByName(name)) {
            PARSE_ERR((yylsp[-7]), (String("Redeclaration of variable '") + name + "'.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            IntExprRef sizeExpr = (yyvsp[-5].nExpression);
            ArgsRef args = (yyvsp[-1].nArgs);
            int size = sizeExpr->evalInt();
            if (size <= 0) {
                PARSE_ERR((yylsp[-5]), (String("Array variable '") + name + "' must be declared with positive array size.").c_str());
                (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
            } else if (args->argsCount() > size) {
                PARSE_ERR((yylsp[-1]), (String("Array variable '") + name +
                          "' was declared with size " + ToString(size) +
                          " but " + ToString(args->argsCount()) +
                          " values were assigned." ).c_str());
                (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever           
            } else {
                bool argsOK = true;
                for (int i = 0; i < args->argsCount(); ++i) {
                    if (args->arg(i)->exprType() != INT_EXPR) {
                        PARSE_ERR(
                            (yylsp[-1]), 
                            (String("Array variable '") + name +
                            "' declared with invalid assignment values. Assigned element " +
                            ToString(i+1) + " is not an integer expression.").c_str()
                        );
                        argsOK = false;
                        break;
                    }
                }
                if (argsOK) {
                    context->vartable[name] = new IntArrayVariable(context, size, args);
                    (yyval.nStatement) = new NoOperation;
                } else
                    (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
            }
        }
    }
#line 1838 "y.tab.c" /* yacc.c:1652  */
    break;

  case 23:
#line 313 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-7].sValue);
        if (!(yyvsp[-5].nExpression)->isConstExpr()) {
            PARSE_ERR((yylsp[-5]), (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if ((yyvsp[-5].nExpression)->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-5]), (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if (context->variableByName(name)) {
            PARSE_ERR((yylsp[-7]), (String("Redeclaration of variable '") + name + "'.").c_str());
            (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            IntExprRef sizeExpr = (yyvsp[-5].nExpression);
            ArgsRef args = (yyvsp[-1].nArgs);
            int size = sizeExpr->evalInt();
            if (size <= 0) {
                PARSE_ERR((yylsp[-5]), (String("Array variable '") + name + "' must be declared with positive array size.").c_str());
                (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
            } else if (args->argsCount() > size) {
                PARSE_ERR((yylsp[-1]), (String("Array variable '") + name +
                          "' was declared with size " + ToString(size) +
                          " but " + ToString(args->argsCount()) +
                          " values were assigned." ).c_str());
                (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever           
            } else {
                bool argsOK = true;
                for (int i = 0; i < args->argsCount(); ++i) {
                    if (args->arg(i)->exprType() != INT_EXPR) {
                        PARSE_ERR(
                            (yylsp[-1]),
                            (String("Array variable '") + name +
                            "' declared with invalid assignment values. Assigned element " +
                            ToString(i+1) + " is not an integer expression.").c_str()
                        );
                        argsOK = false;
                        break;
                    }
                    if (!args->arg(i)->isConstExpr()) {
                        PARSE_ERR(
                            (yylsp[-1]),
                            (String("const array variable '") + name +
                            "' must be defined with const values. Assigned element " +
                            ToString(i+1) + " is not a const expression though.").c_str()
                        );
                        argsOK = false;
                        break;
                    }
                }
                if (argsOK) {
                    context->vartable[name] = new IntArrayVariable(context, size, args, true);
                    (yyval.nStatement) = new NoOperation;
                } else
                    (yyval.nStatement) = new FunctionCall("nothing", new Args, NULL); // whatever
            }
        }
    }
#line 1899 "y.tab.c" /* yacc.c:1652  */
    break;

  case 24:
#line 369 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-2].sValue);
        if ((yyvsp[0].nExpression)->exprType() == STRING_EXPR) {
            if (name[0] == '$')
                PARSE_WRN((yylsp[0]), "Variable declared as integer, string expression assigned though.");
            String s;
            StringExprRef expr = (yyvsp[0].nExpression);
            if (expr->isConstExpr())
                s = expr->evalStr();
            else
                PARSE_ERR((yylsp[0]), (String("Assignment to const string variable '") + name + "' requires const expression.").c_str());
            ConstStringVariableRef var = new ConstStringVariable(context, s);
            context->vartable[name] = var;
            //$$ = new Assignment(var, new StringLiteral(s));
            (yyval.nStatement) = new NoOperation();
        } else {
            if (name[0] == '@')
                PARSE_WRN((yylsp[0]), "Variable declared as string, integer expression assigned though.");
            int i = 0;
            IntExprRef expr = (yyvsp[0].nExpression);
            if (expr->isConstExpr())
                i = expr->evalInt();
            else
                PARSE_ERR((yylsp[0]), (String("Assignment to const integer variable '") + name + "' requires const expression.").c_str());
            ConstIntVariableRef var = new ConstIntVariable(i);
            context->vartable[name] = var;
            //$$ = new Assignment(var, new IntLiteral(i));
            (yyval.nStatement) = new NoOperation();
        }
    }
#line 1934 "y.tab.c" /* yacc.c:1652  */
    break;

  case 25:
#line 399 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatement) = (yyvsp[0].nStatement);
    }
#line 1942 "y.tab.c" /* yacc.c:1652  */
    break;

  case 26:
#line 402 "parser.y" /* yacc.c:1652  */
    {
        if ((yyvsp[-4].nExpression)->exprType() == INT_EXPR) {
            (yyval.nStatement) = new While((yyvsp[-4].nExpression), (yyvsp[-2].nStatements));
        } else {
            PARSE_ERR((yylsp[-4]), "Condition for 'while' loops must be integer expression.");
            (yyval.nStatement) = new While(new IntLiteral(0), (yyvsp[-2].nStatements));
        }
    }
#line 1955 "y.tab.c" /* yacc.c:1652  */
    break;

  case 27:
#line 410 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatement) = new SyncBlock((yyvsp[-2].nStatements));
    }
#line 1963 "y.tab.c" /* yacc.c:1652  */
    break;

  case 28:
#line 413 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatement) = new If((yyvsp[-6].nExpression), (yyvsp[-4].nStatements), (yyvsp[-2].nStatements));
    }
#line 1971 "y.tab.c" /* yacc.c:1652  */
    break;

  case 29:
#line 416 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nStatement) = new If((yyvsp[-4].nExpression), (yyvsp[-2].nStatements));
    }
#line 1979 "y.tab.c" /* yacc.c:1652  */
    break;

  case 30:
#line 419 "parser.y" /* yacc.c:1652  */
    {
        if ((yyvsp[-3].nExpression)->exprType() == INT_EXPR) {
            (yyval.nStatement) = new SelectCase((yyvsp[-3].nExpression), (yyvsp[-2].nCaseBranches));
        } else {
            PARSE_ERR((yylsp[-3]), "Statement 'select' can only by applied to integer expressions.");
            (yyval.nStatement) = new SelectCase(new IntLiteral(0), (yyvsp[-2].nCaseBranches));
        }
    }
#line 1992 "y.tab.c" /* yacc.c:1652  */
    break;

  case 31:
#line 429 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nCaseBranches) = CaseBranches();
        (yyval.nCaseBranches).push_back((yyvsp[0].nCaseBranch));
    }
#line 2001 "y.tab.c" /* yacc.c:1652  */
    break;

  case 32:
#line 433 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nCaseBranches) = (yyvsp[-1].nCaseBranches);
        (yyval.nCaseBranches).push_back((yyvsp[0].nCaseBranch));
    }
#line 2010 "y.tab.c" /* yacc.c:1652  */
    break;

  case 33:
#line 439 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nCaseBranch) = CaseBranch();
        (yyval.nCaseBranch).from = new IntLiteral((yyvsp[-1].iValue));
        (yyval.nCaseBranch).statements = (yyvsp[0].nStatements);
    }
#line 2020 "y.tab.c" /* yacc.c:1652  */
    break;

  case 34:
#line 444 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nCaseBranch) = CaseBranch();
        (yyval.nCaseBranch).from = new IntLiteral((yyvsp[-3].iValue));
        (yyval.nCaseBranch).to   = new IntLiteral((yyvsp[-1].iValue));
        (yyval.nCaseBranch).statements = (yyvsp[0].nStatements);
    }
#line 2031 "y.tab.c" /* yacc.c:1652  */
    break;

  case 35:
#line 452 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[0].sValue);
        StatementsRef fn = context->userFunctionByName(name);
        if (context->functionProvider->functionByName(name)) {
            PARSE_ERR((yylsp[-1]), (String("Keyword 'call' must only be used for user defined functions, not for any built-in function like '") + name + "'.").c_str());
            (yyval.nStatements) = StatementsRef();
        } else if (!fn) {
            PARSE_ERR((yylsp[0]), (String("No user defined function with name '") + name + "'.").c_str());
            (yyval.nStatements) = StatementsRef();
        } else {
            (yyval.nStatements) = fn;
        }
    }
#line 2049 "y.tab.c" /* yacc.c:1652  */
    break;

  case 36:
#line 467 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-3].sValue);
        //printf("function call of '%s' with args\n", name);
        ArgsRef args = (yyvsp[-1].nArgs);
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR((yylsp[-3]), (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR((yylsp[-3]), (String("No built-in function with name '") + name + "'.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP((yyloc));
            (yyval.nFunctionCall) = new NoFunctionCall;
        } else if (args->argsCount() < fn->minRequiredArgs()) {
            PARSE_ERR((yylsp[-1]), (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (args->argsCount() > fn->maxAllowedArgs()) {
            PARSE_ERR((yylsp[-1]), (String("Built-in function '") + name + "' accepts max. " + ToString(fn->maxAllowedArgs()) + " arguments.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else {
            bool argsOK = true;
            for (int i = 0; i < args->argsCount(); ++i) {
                if (args->arg(i)->exprType() != fn->argType(i) && !fn->acceptsArgType(i, args->arg(i)->exprType())) {
                    PARSE_ERR((yylsp[-1]), (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects " + typeStr(fn->argType(i)) + " type, but type " + typeStr(args->arg(i)->exprType()) + " was given instead.").c_str());
                    argsOK = false;
                    break;
                } else if (fn->modifiesArg(i) && !args->arg(i)->isModifyable()) {
                    PARSE_ERR((yylsp[-1]), (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects an assignable variable.").c_str());
                    argsOK = false;
                    break;
                }
            }
            (yyval.nFunctionCall) = new FunctionCall(name, args, argsOK ? fn : NULL);
        }
    }
#line 2090 "y.tab.c" /* yacc.c:1652  */
    break;

  case 37:
#line 503 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-2].sValue);
        //printf("function call of '%s' (with empty args)\n", name);
        ArgsRef args = new Args;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR((yylsp[-2]), (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR((yylsp[-2]), (String("No built-in function with name '") + name + "'.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP((yyloc));
            (yyval.nFunctionCall) = new NoFunctionCall;
        } else if (fn->minRequiredArgs() > 0) {
            PARSE_ERR((yylsp[0]), (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else {
            (yyval.nFunctionCall) = new FunctionCall(name, args, fn);
        }
    }
#line 2116 "y.tab.c" /* yacc.c:1652  */
    break;

  case 38:
#line 524 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[0].sValue);
        //printf("function call of '%s' (without args)\n", name);
        ArgsRef args = new Args;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR((yylsp[0]), (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR((yylsp[0]), (String("No built-in function with name '") + name + "'.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP((yyloc));
            (yyval.nFunctionCall) = new NoFunctionCall;
        } else if (fn->minRequiredArgs() > 0) {
            PARSE_ERR((yylsp[0]), (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            (yyval.nFunctionCall) = new FunctionCall(name, args, NULL);
        } else {
            (yyval.nFunctionCall) = new FunctionCall(name, args, fn);
        }
    }
#line 2142 "y.tab.c" /* yacc.c:1652  */
    break;

  case 39:
#line 547 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nArgs) = new Args();
        (yyval.nArgs)->add((yyvsp[0].nExpression));
    }
#line 2151 "y.tab.c" /* yacc.c:1652  */
    break;

  case 40:
#line 551 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nArgs) = (yyvsp[-2].nArgs);
        (yyval.nArgs)->add((yyvsp[0].nExpression));
    }
#line 2160 "y.tab.c" /* yacc.c:1652  */
    break;

  case 42:
#line 560 "parser.y" /* yacc.c:1652  */
    {
        //printf("variable lookup with name '%s' as assignment expr\n", $1);
        const char* name = (yyvsp[-2].sValue);
        VariableRef var = context->variableByName(name);
        if (!var)
            PARSE_ERR((yylsp[-2]), (String("Variable assignment: No variable declared with name '") + name + "'.").c_str());
        else if (var->isConstExpr())
            PARSE_ERR((yylsp[-1]), (String("Variable assignment: Cannot modify const variable '") + name + "'.").c_str());
        else if (!var->isAssignable())
            PARSE_ERR((yylsp[-1]), (String("Variable assignment: Variable '") + name + "' is not assignable.").c_str());
        else if (var->exprType() != (yyvsp[0].nExpression)->exprType())
            PARSE_ERR((yylsp[0]), (String("Variable assignment: Variable '") + name + "' is of type " + typeStr(var->exprType()) + ", assignment is of type " + typeStr((yyvsp[0].nExpression)->exprType()) + " though.").c_str());
        (yyval.nStatement) = new Assignment(var, (yyvsp[0].nExpression));
    }
#line 2179 "y.tab.c" /* yacc.c:1652  */
    break;

  case 43:
#line 574 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-5].sValue);
        VariableRef var = context->variableByName(name);
        if (!var)
            PARSE_ERR((yylsp[-5]), (String("No variable declared with name '") + name + "'.").c_str());
        else if (var->exprType() != INT_ARR_EXPR)
            PARSE_ERR((yylsp[-4]), (String("Variable '") + name + "' is not an array variable.").c_str());
        else if (var->isConstExpr())
            PARSE_ERR((yylsp[-1]), (String("Variable assignment: Cannot modify const array variable '") + name + "'.").c_str());
        else if (!var->isAssignable())
            PARSE_ERR((yylsp[-1]), (String("Variable assignment: Array variable '") + name + "' is not assignable.").c_str());
        else if ((yyvsp[-3].nExpression)->exprType() != INT_EXPR)
            PARSE_ERR((yylsp[-3]), (String("Array variable '") + name + "' accessed with non integer expression.").c_str());
        else if ((yyvsp[0].nExpression)->exprType() != INT_EXPR)
            PARSE_ERR((yylsp[-1]), (String("Value assigned to array variable '") + name + "' must be an integer expression.").c_str());
        else if ((yyvsp[-3].nExpression)->isConstExpr() && (yyvsp[-3].nExpression)->asInt()->evalInt() >= ((IntArrayVariableRef)var)->arraySize())
            PARSE_WRN((yylsp[-3]), (String("Index ") + ToString((yyvsp[-3].nExpression)->asInt()->evalInt()) +
                          " exceeds size of array variable '" + name +
                          "' which was declared with size " +
                          ToString(((IntArrayVariableRef)var)->arraySize()) + ".").c_str());
        IntArrayElementRef element = new IntArrayElement(var, (yyvsp[-3].nExpression));
        (yyval.nStatement) = new Assignment(element, (yyvsp[0].nExpression));
    }
#line 2207 "y.tab.c" /* yacc.c:1652  */
    break;

  case 44:
#line 599 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = new IntLiteral((yyvsp[0].iValue));
    }
#line 2215 "y.tab.c" /* yacc.c:1652  */
    break;

  case 45:
#line 602 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = new StringLiteral((yyvsp[0].sValue));
    }
#line 2223 "y.tab.c" /* yacc.c:1652  */
    break;

  case 46:
#line 605 "parser.y" /* yacc.c:1652  */
    {
        //printf("variable lookup with name '%s' as unary expr\n", $1);
        VariableRef var = context->variableByName((yyvsp[0].sValue));
        if (var)
            (yyval.nExpression) = var;
        else {
            PARSE_ERR((yylsp[0]), (String("No variable declared with name '") + (yyvsp[0].sValue) + "'.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        }
    }
#line 2238 "y.tab.c" /* yacc.c:1652  */
    break;

  case 47:
#line 615 "parser.y" /* yacc.c:1652  */
    {
        const char* name = (yyvsp[-3].sValue);
        VariableRef var = context->variableByName(name);
        if (!var) {
            PARSE_ERR((yylsp[-3]), (String("No variable declared with name '") + name + "'.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (var->exprType() != INT_ARR_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Variable '") + name + "' is not an array variable.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if ((yyvsp[-1].nExpression)->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-1]), (String("Array variable '") + name + "' accessed with non integer expression.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            if ((yyvsp[-1].nExpression)->isConstExpr() && (yyvsp[-1].nExpression)->asInt()->evalInt() >= ((IntArrayVariableRef)var)->arraySize())
                PARSE_WRN((yylsp[-1]), (String("Index ") + ToString((yyvsp[-1].nExpression)->asInt()->evalInt()) +
                               " exceeds size of array variable '" + name +
                               "' which was declared with size " +
                               ToString(((IntArrayVariableRef)var)->arraySize()) + ".").c_str());
            (yyval.nExpression) = new IntArrayElement(var, (yyvsp[-1].nExpression));
        }
    }
#line 2264 "y.tab.c" /* yacc.c:1652  */
    break;

  case 48:
#line 636 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = (yyvsp[-1].nExpression);
    }
#line 2272 "y.tab.c" /* yacc.c:1652  */
    break;

  case 49:
#line 639 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = (yyvsp[0].nFunctionCall);
    }
#line 2280 "y.tab.c" /* yacc.c:1652  */
    break;

  case 50:
#line 642 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = new Neg((yyvsp[0].nExpression));
    }
#line 2288 "y.tab.c" /* yacc.c:1652  */
    break;

  case 51:
#line 645 "parser.y" /* yacc.c:1652  */
    {
        if ((yyvsp[0].nExpression)->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of bitwise operator '.not.' must be an integer expression, is ") + typeStr((yyvsp[0].nExpression)->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new BitwiseNot((yyvsp[0].nExpression));
        }
    }
#line 2301 "y.tab.c" /* yacc.c:1652  */
    break;

  case 52:
#line 653 "parser.y" /* yacc.c:1652  */
    {
        if ((yyvsp[0].nExpression)->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator 'not' must be an integer expression, is ") + typeStr((yyvsp[0].nExpression)->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Not((yyvsp[0].nExpression));
        }
    }
#line 2314 "y.tab.c" /* yacc.c:1652  */
    break;

  case 55:
#line 667 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->isConstExpr() && rhs->isConstExpr()) {
            (yyval.nExpression) = new StringLiteral(
                lhs->evalCastToStr() + rhs->evalCastToStr()
            );
        } else {
            (yyval.nExpression) = new ConcatString(lhs, rhs);
        }
    }
#line 2330 "y.tab.c" /* yacc.c:1652  */
    break;

  case 57:
#line 681 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator 'or' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator 'or' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Or(lhs, rhs);
        }
    }
#line 2348 "y.tab.c" /* yacc.c:1652  */
    break;

  case 58:
#line 696 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = (yyvsp[0].nExpression);
    }
#line 2356 "y.tab.c" /* yacc.c:1652  */
    break;

  case 59:
#line 699 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator 'and' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator 'and' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new And(lhs, rhs);
        }
    }
#line 2374 "y.tab.c" /* yacc.c:1652  */
    break;

  case 61:
#line 715 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of bitwise operator '.or.' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of bitwise operator '.or.' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new BitwiseOr(lhs, rhs);
        }
    }
#line 2392 "y.tab.c" /* yacc.c:1652  */
    break;

  case 62:
#line 730 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = (yyvsp[0].nExpression);
    }
#line 2400 "y.tab.c" /* yacc.c:1652  */
    break;

  case 63:
#line 733 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of bitwise operator '.and.' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of bitwise operator '.and.' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new BitwiseAnd(lhs, rhs);
        }
    }
#line 2418 "y.tab.c" /* yacc.c:1652  */
    break;

  case 65:
#line 749 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '<' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '<' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Relation(lhs, Relation::LESS_THAN, rhs);
        }
    }
#line 2436 "y.tab.c" /* yacc.c:1652  */
    break;

  case 66:
#line 762 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '>' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '>' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Relation(lhs, Relation::GREATER_THAN, rhs);
        }
    }
#line 2454 "y.tab.c" /* yacc.c:1652  */
    break;

  case 67:
#line 775 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '<=' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '<=' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Relation(lhs, Relation::LESS_OR_EQUAL, rhs);
        }
    }
#line 2472 "y.tab.c" /* yacc.c:1652  */
    break;

  case 68:
#line 788 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '>=' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '>=' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Relation(lhs, Relation::GREATER_OR_EQUAL, rhs);
        }
    }
#line 2490 "y.tab.c" /* yacc.c:1652  */
    break;

  case 69:
#line 801 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = new Relation((yyvsp[-2].nExpression), Relation::EQUAL, (yyvsp[0].nExpression));
    }
#line 2498 "y.tab.c" /* yacc.c:1652  */
    break;

  case 70:
#line 804 "parser.y" /* yacc.c:1652  */
    {
        (yyval.nExpression) = new Relation((yyvsp[-2].nExpression), Relation::NOT_EQUAL, (yyvsp[0].nExpression));
    }
#line 2506 "y.tab.c" /* yacc.c:1652  */
    break;

  case 72:
#line 810 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '+' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '+' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Add(lhs,rhs);
        }
    }
#line 2524 "y.tab.c" /* yacc.c:1652  */
    break;

  case 73:
#line 823 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '-' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '-' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Sub(lhs,rhs);
        }
    }
#line 2542 "y.tab.c" /* yacc.c:1652  */
    break;

  case 75:
#line 839 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '*' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '*' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Mul(lhs,rhs);
        }
    }
#line 2560 "y.tab.c" /* yacc.c:1652  */
    break;

  case 76:
#line 852 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of operator '/' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of operator '/' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Div(lhs,rhs);
        }
    }
#line 2578 "y.tab.c" /* yacc.c:1652  */
    break;

  case 77:
#line 865 "parser.y" /* yacc.c:1652  */
    {
        ExpressionRef lhs = (yyvsp[-2].nExpression);
        ExpressionRef rhs = (yyvsp[0].nExpression);
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[-2]), (String("Left operand of modulo operator must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR((yylsp[0]), (String("Right operand of modulo operator must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            (yyval.nExpression) = new IntLiteral(0);
        } else {
            (yyval.nExpression) = new Mod(lhs,rhs);
        }
    }
#line 2596 "y.tab.c" /* yacc.c:1652  */
    break;


#line 2600 "y.tab.c" /* yacc.c:1652  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, context, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, context, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, context);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, context);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, context, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, context);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, context);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 879 "parser.y" /* yacc.c:1918  */


void InstrScript_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err) {
    //fprintf(stderr, "%d: %s\n", locp->first_line, err);
    context->addErr(locp->first_line, locp->last_line, locp->first_column+1, locp->last_column+1, err);
}

void InstrScript_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt) {
    //fprintf(stderr, "WRN %d: %s\n", locp->first_line, txt);
    context->addWrn(locp->first_line, locp->last_line, locp->first_column+1, locp->last_column+1, txt);
}

/// Custom implementation of yytnamerr() to ensure quotation is always stripped from token names before printing them to error messages.
int InstrScript_tnamerr(char* yyres, const char* yystr) {
  if (*yystr == '"') {
      int yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
/*
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
*/
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
/*
    do_not_strip_quotes: ;
*/
    }

  if (! yyres)
    return (int) yystrlen (yystr);

  return int( yystpcpy (yyres, yystr) - yyres );
}
