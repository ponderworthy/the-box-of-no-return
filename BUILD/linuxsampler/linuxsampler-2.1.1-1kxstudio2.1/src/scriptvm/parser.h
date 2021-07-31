/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

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
