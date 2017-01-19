/*
@(#)File:           $RCSfile: scc.c,v $
@(#)Version:        $Revision: 5.5 $
@(#)Last changed:   $Date: 2012/01/23 19:22:28 $
@(#)Purpose:        Strip C comments
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1991,1993,1997-98,2003,2005,2007-08,2011-12
*/

/*TABSTOP=4*/

/*
**  This processor removes any C comments and replaces them by a
**  single space.  It will be used as part of a formatting pipeline
**  for checking the equivalence of C code.
**
**  Note that backslashes at the end of a line can extend even a C++
**  style comment over several lines.  It matters not whether there is
**  one backslash or several -- the line splicing (logically) takes
**  place before any other tokenisation.
**
**  The -s option was added to simplify the analysis of C++ keywords.
**  After stripping comments, keywords appearing in (double quoted)
**  strings should be ignored too, so a replacement character such as X
**  works.  Without that, the command has to recognize C comments to
**  know whether the unmatched quotes in them matter (they shouldn't).
**  The -q option was added for symmetry with -s.  Maybe the options
**  letters should be -d for double quotes and -s for single quotes; or
**  maybe they should be -s for strings and -c for characters.
*/

#include "posixver.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "stderr.h"
#include "filter.h"

typedef enum { NonComment, CComment, CppComment } Comment;

static int cflag = 0;   /* Print comments and not code */
static int Cflag = 1;   /* Strip C++/C99 style (//) comments (unless -C flag) */
static int wflag = 0;   /* Warn about nested C-style comments */
static int sflag = 0;   /* Replacement character for strings */
static int qflag = 0;   /* Replacement character for quotes */
static int nflag = 0;   /* Keep newlines in comments */

static int nline = 0;   /* Line counter */

static const char optstr[] = "chns:q:wCV";
static const char usestr[] = "[-chnwCV][-s rep][-q rep] [file ...]";
static const char hlpstr[] =
    "  -c      Print comments and not the code\n"
    "  -h      Print this help and exit\n"
    "  -n      Keep newlines in comments\n"
    "  -s rep  Replace the body of string literals with rep (a single character)\n"
    "  -q rep  Replace the body of character literals with rep (a single character)\n"
    "  -w      Warn about nested C-style comments\n"
    "  -C      Do not recognize C++/C99 style (//) comments\n"
    "  -V      Print version information and exit\n"
    ;

#ifndef lint
static const char rcs[] = "@(#)$Id: scc.c,v 5.5 2012/01/23 19:22:28 jleffler Exp $";
#endif

static int getch(FILE *fp)
{
    int c = getc(fp);
    if (c == '\n')
        nline++;
    return(c);
}

static int ungetch(char c, FILE *fp)
{
    if (c == '\n')
        nline--;
    return(ungetc(c, fp));
}

static int peek(FILE *fp)
{
    int c;

    if ((c = getch(fp)) != EOF)
        ungetch(c, fp);
    return(c);
}

/* Put source code character */
static void s_putch(char c)
{
    if (!cflag)
        putchar(c);
}

/* Put comment (non-code) character */
static void c_putch(char c)
{
    if (cflag || (nflag && c == '\n'))
        putchar(c);
}

static void warning(const char *str, const char *file, int line)
{
    err_report(ERR_REM, ERR_STAT, "%s:%d: %s\n", file, line, str);
}

static void warning2(const char *s1, const char *s2, const char *file, int line)
{
    err_report(ERR_REM, ERR_STAT, "%s:%d: %s %s\n", file, line, s1, s2);
}

static void put_quote_char(char q, char c)
{
    if (q == '\'' && qflag != 0)
        s_putch(qflag);
    else if (q == '"' && sflag != 0)
        s_putch(sflag);
    else
        s_putch(c);
}

static void endquote(char q, FILE *fp, char *fn, const char *msg)
{
    int     c;

    while ((c = getch(fp)) != EOF && c != q)
    {
        put_quote_char(q, c);
        if (c == '\\')
        {
            if ((c = getch(fp)) == EOF)
                break;
            put_quote_char(q, c);
            if (c == '\\' && peek(fp) == '\n')
                s_putch(getch(fp));
        }
        else if (c == '\n')
            warning2("newline in", msg, fn, nline);
    }
    if (c == EOF)
    {
        warning2("EOF in", msg, fn, nline);
        return;
    }
    s_putch(q);
}

/*
** read_bsnl() - Count the number of backslash newline pairs that
** immediately follow in the input stream.  On entry, peek() might
** return the backslash of a backslash newline pair, or some other
** character.  On exit, getch() will return the first character
** after the sequence of n (n >= 0) backslash newline pairs.  It
** relies on two characters of pushback (but only of data already
** read) - not mandated by POSIX or C standards.  Double pushback
** may cause problems on HP-UX, Solaris or AIX (but does not cause
** trouble on Linux or BSD).
*/
static int read_bsnl(FILE *fp)
{
    int n = 0;
    int c;

    while ((c = peek(fp)) != EOF)
    {
        if (c != '\\')
            return(n);
        c = getch(fp);              /* Read backslash */
        if ((c = peek(fp)) != '\n') /* Single pushback */
        {
            ungetch('\\', fp);      /* Double pushback */
            return(n);
        }
        n++;
        c = getch(fp);              /* Read newline */
    }
    return(n);
}

static void write_bsnl(int bsnl, void (*put)(char))
{
    while (bsnl-- > 0)
    {
        (*put)('\\');
        (*put)('\n');
    }
}

static void scc(FILE *fp, char *fn)
{
    int bsnl;
    int oc;
    int pc;
    int c;
    int l_nest = 0; /* Last line with a nested comment warning */
    int l_cend = 0; /* Last line with a comment end warning */
    Comment status = NonComment;

    nline = 1;
    for (oc = '\0'; (c = getch(fp)) != EOF; oc = c)
    {
        switch (status)
        {
        case CComment:
            if (c == '*')
            {
                bsnl = read_bsnl(fp);
                if (peek(fp) == '/')
                {
                    status = NonComment;
                    c = getch(fp);
                    c_putch('*');
                    write_bsnl(bsnl, c_putch);
                    c_putch('/');
                    s_putch(' ');
                }
                else
                {
                    c_putch(c);
                    write_bsnl(bsnl, c_putch);
                }
            }
            else if (wflag == 1 && c == '/' && peek(fp) == '*')
            {
                if (l_nest != nline)
                    warning("nested C-style comment", fn, nline);
                l_nest = nline;
                c_putch(c);
            }
            else
                c_putch(c);
            break;
        case CppComment:
            if (c == '\n' && oc != '\\')
            {
                status = NonComment;
                s_putch(c);
            }
            else
                c_putch(c);
            break;
        case NonComment:
            if (c == '*' && wflag == 1 && peek(fp) == '/')
            {
                /* NB: does not detect star backslash newline slash as stray end of comment */
                if (l_cend != nline)
                    warning("C-style comment end marker not in a comment",
                            fn, nline);
                l_cend = nline;
            }
            if (c == '\'')
            {
                s_putch(c);
                /*
                ** Single quotes can contain multiple characters, such as
                ** '\\', '\'', '\377', '\x4FF', 'ab', '/ *' (with no space, and
                ** the reverse character pair) , etc.  Scan for an unescaped
                ** closing single quote.  Newlines are not acceptable either,
                ** unless preceded by a backslash -- so both '\<nl>\n' and
                ** '\\<nl>n' are OK, and are equivalent to a newline character
                ** (when <nl> is a physical newline in the source code).
                */
                endquote('\'', fp, fn, "character constant");
            }
            else if (c == '"')
            {
                s_putch(c);
                /* Double quotes are relatively simple, except that */
                /* they can legitimately extend over several lines */
                /* when each line is terminated by a backslash */
                endquote('\"', fp, fn, "string literal");
            }
            else if (c != '/')
            {
                s_putch(c);
            }
            /* c is a slash from here on - potential start of comment */
            else
            {
                bsnl = read_bsnl(fp);
                if ((pc = peek(fp)) == '*')
                {
                    status = CComment;
                    c = getch(fp);
                    c_putch('/');
                    write_bsnl(bsnl, c_putch);
                    c_putch('*');
                }
                else if (Cflag == 1 && pc == '/')
                {
                    status = CppComment;
                    c = getch(fp);
                    c_putch('/');
                    write_bsnl(bsnl, c_putch);
                    c_putch('/');
                }
                else
                {
                    s_putch(c);
                    write_bsnl(bsnl, s_putch);
                }
            }
            break;
        }
    }
    if (status != NonComment)
        warning("unterminated C-style comment", fn, nline);
}

int main(int argc, char **argv)
{
    int opt;

    err_setarg0(argv[0]);

    while ((opt = getopt(argc, argv, optstr)) != EOF)
    {
        switch (opt)
        {
        case 'c':
            cflag = 1;
            break;
        case 'h':
            err_help(usestr, hlpstr);
            break;
        case 'n':
            nflag = 1;
            break;
        case 's':
            sflag = *optarg;
            break;
        case 'q':
            qflag = *optarg;
            break;
        case 'w':
            wflag = 1;
            break;
        case 'C':
            Cflag = 0;
            break;
        case 'V':
            err_version("SCC", "$Revision: 5.5 $ ($Date: 2012/01/23 19:22:28 $)");
            break;
        default:
            err_usage(usestr);
            break;
        }
    }

    filter(argc, argv, optind, scc);
    return(0);
}
