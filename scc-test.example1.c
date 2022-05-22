/*
@(#)File:           $RCSfile: scc-test.example1.c,v $
@(#)Version:        $Revision: 1.10 $
@(#)Last changed:   $Date: 2015/08/17 04:14:48 $
@(#)Purpose:        Test SCC on core functionality
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1997,2003,2007,2013-15
@(#)Product:        SCC Version 8.0.1 (2022-05-21)
*/

/*TABSTOP=4*/

// -- C++ comment at start of line

/*
Multiline C-style comment
#ifndef lint
static const char sccs[] = "@(#)$Id: scc-test.example1.c,v 1.10 2015/08/17 04:14:48 jleffler Exp $";
#endif
*/

/*
Multi-line C-style comment
with embedded /* in line %C% which should generate a warning
if scc is run with the -w option
Two comment starts /* embedded /* in line %C% should generate one warning
*/

/* Comment */ Non-comment /* Comment Again */ Non-Comment Again /*
Comment again on the next line */

// A C++ comment with a C-style comment marker /* in the middle
This is plain text under C++ (C99) commenting - but comment body otherwise
// A C++ comment with a C-style comment end marker */ in the middle

The following C-style comment end marker should generate a warning
if scc is run with the -w option
*/
Two of these */ generate */ one warning

It is possible to have both warnings on a single line.
Eg:
*/ /* /* */ */

SCC has been trained to handle 'q' single quotes in most of
the aberrant forms that can be used.  '\0', '\\', '\'', '\\
n' (a valid variant on '\n'), because the backslash followed
by newline is elided by the token scanning code in CPP before
any other processing occurs.

This is a legitimate equivalent to '\n' too: '\
\n', again because the backslash/newline processing occurs early.

The non-portable 'ab', '/*', '*/', '//' forms are handled OK too.

Note that C++ comments can appear   // at the end of a line
And the issue then is how should    // the newline be handled
when the options -c or -n or both   // or neither are in use
Should the newline be treated as    // part of the comment
or should it be deleted so that     // text is run on altogether

The following quote should generate a warning from SCC; a
compiler would not accept it.  '
\n'

" */ /* SCC has been trained to know about strings /* */ */"!
"\"Double quotes embedded in strings, \\\" too\'!"
"And \
newlines in them"
"/* This is not a comment */"
"// This is not a comment either"
'/*' No comment there
'//' No comment there
'*/' No end comment there

"And escaped double quotes at the end of a string\""

aa '\\
n' OK
aa "\""
aa "\
\n"

This is followed by C++/C99 comment number 1.
// C++/C99 comment with \
continuation character \
on three source lines (this should not be seen with the -C flag)
The C++/C99 comment number 1 has finished.

This is followed by C++/C99 comment number 2.
/\
/\
C++/C99 comment (this should not be seen with the -C flag)
The C++/C99 comment number 2 has finished.

This is followed by regular C comment number 1.
/\
*\
Regular
comment
*\
/
The regular C comment number 1 has finished.

/\
\/ This is not a C++/C99 comment!

This is followed by C++/C99 comment number 3.
/\
\
\
/ But this is a C++/C99 comment!
The C++/C99 comment number 3 has finished.

/\
\* This is not a C or C++  comment!

This is followed by regular C comment number 2.
/\
*/ This is a regular C comment *\
but this is just a routine continuation *\
and that was not the end either - but this is *\
\
/
The regular C comment number 2 has finished.

This is followed by regular C comment number 3.
/\
\
\
\
* C comment */
The regular C comment number 3 has finished.

Note that \u1234 and \U0010FFF0 are legitimate Unicode characters
(officially universal character names) that could appear in an
id\u0065ntifier, a '\u0065' character constant, or in a "char\u0061cter\
 string".  Since these are mapped long after comments are eliminated,
they cannot affect the interpretation of /* comments */.  In particular,
none of \u0002A.  \U0000002A, \u002F and \U0000002F ever constitute part
of a comment delimiter ('*' or '/').

More double quoted string stuff:

    if (logtable_out)
    {
	sprintf(logtable_out,
		"insert into %s (bld_id, err_operation, err_expected, err_sql_stmt, err_sql_state)" 
		" values (\"%s\", \"%s\", \"%s\", \"", str_logtable, blade, operation, expected);
	/* watch out for embedded double quotes. */
    }


/* Non-terminated C-style comment at the end of the file
