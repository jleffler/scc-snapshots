/*
@(#)File:           $RCSfile: scc-test.extended-identifiers.c,v $
@(#)Version:        $Revision: 1.1 $
@(#)Last changed:   $Date: 2014/06/16 10:50:22 $
@(#)Purpose:        Test SCC on Unicode extended identifiers
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1997,2003,2007,2013-14
@(#)Product:        SCC Version 6.16 (2016-01-19)
*/

/* Compile with -fextended-identifiers */

/*TABSTOP=4*/

// -- C++ comment

extern char s5[];
extern int d0;
//extern int id\u0165ntifier;
//extern int \U000AFFF0;
//extern int \u1234;
extern char *c9;
extern char *c8;
extern int c7;
extern char s4[];
extern int c6;
extern int c5;
extern int c4;
extern char s3[];
extern int i1[];
extern char c3[];
extern char c2[];
extern char c1;
extern int value2;
extern int value1;

/*
Multiline C-style comment
#ifndef lint
const char sccs[] = "@(#)$Id: scc-test.extended-identifiers.c,v 1.1 2014/06/16 10:50:22 jleffler Exp $";
#endif
*/

/*
Multi-line C-style comment
with embedded /* in this line which should generate a warning
if scc is run with the -w option
Two comment starts /* embedded /* in this line should generate one warning
*/

/* Comment */ int /* Comment Again */ value1 = 0; /*
Comment again on the next line */

// A C++ comment with a C-style comment marker /* in the middle
int value2 = 0;
// A C++ comment with a C-style comment end marker */ in the middle

char c1 = 'q';
char c2[] = { '\0', '\\', '\'', '\\
n', '\n', };

char c3[] = { '\n', '\
\n', };

int i1[] = { '/*', '*/', '//' };

char s3[] =
{
" */ /* SCC has been trained to know about strings /* */ */"
"\"Double quotes embedded in strings, \\\" too\'!"
"And \
newlines in them"
"/* This is not a comment */"
"// This is not a comment either"
};
int c4 = '/*' ; // No comment there
int c5 = '//' ; // No comment there
int c6 = '*/' ; // No end comment there

char s4[] = "And escaped double quotes at the end of a string\"";

int c7 = '\\
n' ; // OK
char *c8 = "\"";
char *c9 = "\
\n";

// C++/C99 comment with \
continuation character \
on three source lines (this should not be seen with the -C flag)

/\
/\
C++/C99 comment (this should not be seen with the -C flag)

/\
*\
Regular
comment
*\
/

/\
\
\
/ But this is a C++/C99 comment!

/\
*/ This is a regular C comment *\
but this is just a routine continuation *\
and that was not the end either - but this is *\
\
/

/\
\
\
\
* C comment */


/* Not allowed even with -fextended-identifiers */
//int \u1234 = 0;
//int \U0010FFF0 = 0;
//int id\u0165ntifier = 0;
int d0 = '\u0165';
char s5[] = "char\u0161cter\
 string";

