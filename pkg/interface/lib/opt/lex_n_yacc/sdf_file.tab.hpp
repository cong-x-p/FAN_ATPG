/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
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

#ifndef YY_SDF_FILE_LIB_OPT_LEX_N_YACC_SDF_FILE_TAB_HPP_INCLUDED
# define YY_SDF_FILE_LIB_OPT_LEX_N_YACC_SDF_FILE_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int sdf_filedebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    DELAYFILE = 258,
    SDFVERSION = 259,
    DESIGN = 260,
    DATE = 261,
    VENDOR = 262,
    PROGRAM = 263,
    VERSION = 264,
    DIVIDER = 265,
    VOLTAGE = 266,
    PROCESS = 267,
    TEMPERATURE = 268,
    TIMESCALE = 269,
    CELL = 270,
    CELLTYPE = 271,
    INSTANCE = 272,
    DELAY = 273,
    TIMINGCHECK = 274,
    TIMINGENV = 275,
    PATHPULSE = 276,
    PATHPULSEPERCENT = 277,
    ABSOLUTE = 278,
    INCREMENT = 279,
    IOPATH = 280,
    RETAIN = 281,
    COND = 282,
    SCOND = 283,
    CCOND = 284,
    CONDELSE = 285,
    PORT = 286,
    INTERCONNECT = 287,
    DEVICE = 288,
    SETUP = 289,
    HOLD = 290,
    SETUPHOLD = 291,
    RECOVERY = 292,
    REMOVAL = 293,
    RECREM = 294,
    SKEW = 295,
    WIDTH = 296,
    PERIOD = 297,
    NOCHANGE = 298,
    NAME = 299,
    EXCEPTION = 300,
    PATHCONSTRAINT = 301,
    PERIODCONSTRAINT = 302,
    SUM = 303,
    DIFF = 304,
    SKEWCONSTRAINT = 305,
    ARRIVAL = 306,
    DEPARTURE = 307,
    SLACK = 308,
    WAVEFORM = 309,
    POSEDGE = 310,
    NEGEDGE = 311,
    CASE_EQU = 312,
    CASE_INEQU = 313,
    EQU = 314,
    INEQU = 315,
    L_AND = 316,
    L_OR = 317,
    LESS_OR_EQU = 318,
    GREATER_OR_EQU = 319,
    RIGHT_SHIFT = 320,
    LEFT_SHIFT = 321,
    U_NAND = 322,
    U_NOR = 323,
    U_XNOR = 324,
    U_XNOR_ALT = 325,
    BIT_CONST = 326,
    ONE_CONST = 327,
    ZERO_CONST = 328,
    EDGE = 329,
    PATH = 330,
    IDENTIFIER = 331,
    QSTRING = 332,
    NUMBER = 333
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 30 "src/sdf_file.y"

    float                     yfloat;
    char                      ychar[IntfNs::NAME_LEN];
    IntfNs::SdfDelayType      ydelayType;
    IntfNs::SdfValue          yvalue;
    IntfNs::SdfDelayValue     ydelayValue;
    IntfNs::SdfDelayValueList ydelayValueList;
    IntfNs::SdfDelayDef       *ydelayDef;
    IntfNs::SdfIoPath         yioPath;
    IntfNs::SdfPortSpec       yportSpec;

#line 148 "lib/opt/lex_n_yacc/sdf_file.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE sdf_filelval;

int sdf_fileparse (void* param);

#endif /* !YY_SDF_FILE_LIB_OPT_LEX_N_YACC_SDF_FILE_TAB_HPP_INCLUDED  */
