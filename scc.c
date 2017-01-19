/*
@(#)File:           $RCSfile: scc.c,v $
@(#)Version:        $Revision: 6.16 $
@(#)Last changed:   $Date: 2015/08/16 00:36:26 $
@(#)Purpose:        Strip C comments
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1991,1993,1997-98,2003,2005,2007-08,2011-12,2014-15
*/

/*TABSTOP=4*/

/*
**  This processor removes any C comments and replaces them by a
**  single space.  It will be used as part of a formatting pipeline
**  for checking the equivalence of C code.
**
**  If the code won't compile, it is unwise to use this tool to modify
**  it.  It assumes that the code is syntactically correct.
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
**
**  Digraphs do not present a problem; the characters they represent do
**  not need special handling. Trigraphs do present a problem.
**
**  C++14 adds quotes inside numeric literals: 10'000'000 for 10000000,
**  10'000 for 10000, etc.  Unfortunately, that probably means we
**  have to recognize literal values fully, because these can appear in
**  hex too: 0xFFFF'ABCD.  C++14 also adds binary constants: 0b0001'1010
**  (b or B).
**
**  C++11 raw strings are another problem: R"x(...)x" is not bound to
**  have a close quote by end of line.  (The parentheses are mandatory;
**  the x's are optional and are not limited to a single character (but
**  must not be more than 16 characters long), but must match.  The
**  replacable portion (for -s) is the '...' in between parentheses.)
**  C++11 also has encoding prefixes u8, u, U, L which can preceded the
**  R of a R string.  Note that within a raw string, there are no
**  trigraphs.
**
**  Hence we add -S <std> for standards C++98, C++03, C++11, C++14,
**  C89, C99, C11.  For the purposes of this code, C++98 and C++03 are
**  identical, and C99 and C11 are identical.  The default would be C11.
**  This renders the -C flag irrelevant and unsupported.
**
**  C11 and C++11 add support for u"literal", U"literal", u8"literal",
**  and for u'char' and U'char' (but not u8'char').
*/

#include "posixver.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "stderr.h"
#include "filter.h"

typedef enum { NonComment, CComment, CppComment } Comment;

typedef enum
{
    C, C89, C90, C94, C99, C11,
    CXX, CXX98, CXX03, CXX11, CXX14
} Standard;

static const char std_name[][6] =
{
    [C]     = "c",      // Current C standard
    [CXX]   = "c++",    // Current C++ standard
    [C89]   = "c89",
    [C90]   = "c90",
    [C94]   = "c94",
    [C99]   = "c99",
    [C11]   = "c11",
    [CXX98] = "c++98",
    [CXX03] = "c++03",
    [CXX11] = "c++11",
    [CXX14] = "c++14",
};
enum { NUM_STDNAMES = sizeof(std_name) / sizeof(std_name[0]) };

static int cflag = 0;   /* Print comments and not code */
static int nflag = 0;   /* Keep newlines in comments */
static int qflag = 0;   /* Replacement character for quotes */
static int sflag = 0;   /* Replacement character for strings */
static int wflag = 0;   /* Warn about nested C-style comments */

/* Features recognized */
static bool f_DoubleSlash = false;  /* // comments */
static bool f_Trigraphs = false;    /* Trigraphs */
static bool f_RawString = false;    /* Raw strings */
static bool f_Unicode = false;      /* Unicode strings (u\"A\", U\"A\", u8\"A\") */
static bool f_Binary = false;       /* Binary constants 0b0101 */
static bool f_HexFloat = false;     /* Hexadecimal floats 0x2.34P-12 */
static bool f_NumPunct = false;     /* Numeric punctuation 0x1234'5678 */
static bool f_ExtendedIds = false;  /* Universal character names \uXXXX and \Uxxxxxxxx */

static int nline = 0;   /* Line counter */
static int l_nest = 0;  /* Last line with a nested comment warning */
static int l_cend = 0;  /* Last line with a comment end warning */

static const char optstr[] = "cfhnq:s:twS:V";
static const char usestr[] = "[-cfhntwV][-S std][-s rep][-q rep] [file ...]";
static const char hlpstr[] =
    "  -c      Print comments and not the code\n"
    "  -f      Print flags in effect (debugging mainly)\n"
    "  -h      Print this help and exit\n"
    "  -n      Keep newlines in comments\n"
    "  -s rep  Replace the body of string literals with rep (a single character)\n"
    "  -q rep  Replace the body of character literals with rep (a single character)\n"
    "  -t      Recognize trigraphs\n"
    "  -w      Warn about nested C-style comments\n"
    "  -S std  Specify language standard (C, C++, C89, C90, C99, C11, C++98, C++03,\n"
    "          C++11, C++14; default C11)\n"
    "  -V      Print version information and exit\n"
    ;

#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
extern const char jlss_id_scc_c[];
const char jlss_id_scc_c[] = "@(#)$Id: scc.c,v 6.16 2015/08/16 00:36:26 jleffler Exp $";
#endif /* lint */

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

static void warningv(const char *fmt, const char *file, int line, ...)
{
    char buffer[BUFSIZ];
    va_list args;
    va_start(args, line);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    warning(buffer, file, line);
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

static void endquote(char q, FILE *fp, const char *fn, const char *msg)
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

static Comment c_comment(int c, FILE *fp, const char *fn)
{
    Comment status = CComment;
    if (c == '*')
    {
        int bsnl = read_bsnl(fp);
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
    return status;
}

static Comment cpp_comment(int c, int oc)
{
    Comment status = CppComment;
    if (c == '\n' && oc != '\\')
    {
        status = NonComment;
        s_putch(c);
    }
    else
        c_putch(c);
    return status;
}

static inline int is_idchar(int c)
{
    assert(c == EOF || (c >= 0 && c <= UCHAR_MAX));
    return(isalnum(c) || c == '_');
}

static inline int is_binary(int c)
{
    assert(c == EOF || (c >= 0 && c <= UCHAR_MAX));
    return(c == '0' || c == '1');
}

static inline int is_octal(int c)
{
    assert(c == EOF || (c >= 0 && c <= UCHAR_MAX));
    return(c >= '0' && c <= '7');
}

static int check_punct(int oc, FILE *fp, const char *fn, int (*digit_check)(int c))
{
    int sq = getch(fp);
    assert(sq == '\'');
    s_putch(sq);
    if (!(*digit_check)(oc))
    {
        warning("Single quote in numeric context not preceded by a valid digit", fn, nline);
        return sq;
    }
    int pc = peek(fp);
    if (pc == EOF)
    {
        warning("Single quote in numeric context followed by EOF", fn, nline);
        return sq;
    }
    if (!(*digit_check)(pc))
        warning("Single quote in numeric context not followed by a valid digit", fn, nline);
    return pc;
}

static inline void parse_exponent(int c, FILE *fp, const char *fn)
{
    assert(c != 0 && fp != 0 && fn != 0);
}

static void parse_hex(FILE *fp, const char *fn)
{
    /* Hex constant - integer or float */
    /* Should be followed by one or more hex digits */
    s_putch('0');
    int c = getch(fp);
    assert(c == 'x' || c == 'X');
    s_putch(c);
    int oc = c;
    int pc;
    while ((pc = peek(fp)) == '\'' || isxdigit(pc))
    {
        if (pc == '\'')
            oc = check_punct(oc, fp, fn, isxdigit);
        else
        {
            oc = pc;
            s_putch(getch(fp));
        }
    }
    /* TODO: handle hexadecimal floating point exponent */
    if (isdigit(pc))
        warningv("Non-hexadecimal digit %c in hexadecimal constant", fn, nline, pc);
}

static void parse_binary(FILE *fp, const char *fn)
{
    /* Binary constant - integer */
    /* Should be followed by one or more binary digits */
    s_putch('0');     /* 0 */
    int c = getch(fp);
    assert(c == 'b' || c == 'B');
    s_putch(c);     /* b or B */
    int oc = c;
    int pc;
    while ((pc = peek(fp)) == '\'' || is_binary(pc))
    {
        if (pc == '\'')
            oc = check_punct(oc, fp, fn, is_binary);
        else
        {
            oc = pc;
            s_putch(getch(fp));
        }
    }
    if (isdigit(pc))
        warningv("Non-binary digit %c in binary constant", fn, nline, pc);
}

static void parse_octal(FILE *fp, const char *fn)
{
    /* Octal constant - integer */
    /* Should be followed by one or more octal digits */
    s_putch('0');     /* 0 */
    int c = getch(fp);
    assert(is_octal(c) || c == '\'');
    s_putch(c);
    int oc = c;
    int pc;
    while ((pc = peek(fp)) == '\'' || is_octal(pc))
    {
        if (pc == '\'')
            oc = check_punct(oc, fp, fn, is_octal);
        else
        {
            oc = pc;
            s_putch(getch(fp));
        }
    }
    if (is_octal(pc))
        warningv("Non-octal digit %c in octal constant", fn, nline, pc);
}

static void parse_decimal(int c, FILE *fp, const char *fn)
{
    /* Decimal integer, or decimal floating point */
    /* TODO: finish this code */
    s_putch(c);
    int pc = peek(fp);
    if (isdigit(pc) || pc == '\'')
    {
        s_putch(c);
        c = getch(fp);
        s_putch(pc);
        int oc = c;
        while ((pc = peek(fp)) == '\'' || isdigit(pc))
        {
            if (pc == '\'')
                oc = check_punct(oc, fp, fn, isdigit);
            else
            {
                oc = pc;
                s_putch(getch(fp));
            }
        }
        /* TODO: handle floating point exponent */
        if (isdigit(pc))
            warningv("Non-decimal digit %c in decimal constant", fn, nline, pc);
    }
}

/*
** Parse numbers - inherently unsigned.
** Need to recognize:-
** 12345            // Decimal
** 01234567         // Octal - validation not required?
** 0xABCDEF12       // Hexadecimal
** 0b01101100       // C++14 binary
** 9e-82            // Float
** 9.23             // Float
** .987             // Float
** .987E+30         // Float
** 0xA.BCP12        // C99 Hex floating point
** 0B0110'1100      // C++14 punctuated binary number
** 0XDEFA'CED0      // C++14 punctuated hex number
** 234'567.123'987  // C++14 punctuated decimal number
** 0'234'127'310    // C++14 punctuated octal number
** 9'234.192'214e-8 // C++14 punctuated decimal Float
** 0xA'B'C.B'Cp-12  // Presumed punctuated Hex floating point
**
** Note that backslash-newline can occur part way through a number.
*/

static void parse_number(int c, FILE *fp, const char *fn)
{
    assert(isdigit(c) || c == '.');
    int pc = peek(fp);
    if (c != '0')
        parse_decimal(c, fp, fn);
    else if (pc == 'x' || pc == 'X')
        parse_hex(fp, fn);
    else if ((pc == 'b' || pc == 'B') && f_Binary)
        parse_binary(fp, fn);
    else if (is_octal(pc) || pc == '\'')
        parse_octal(fp, fn);
    else if (pc == 'e' || pc == 'E')
    {
        /* Zero floating point decimal constant */
        parse_decimal(c, fp, fn);
    }
    else if (isdigit(pc) || pc == '.')
    {
        /* Malformed number of some sort (09, for example) */
        /* Or does 0.1234 get here too? */
        /* TODO: finish this code */
        err_remark("0%c read - bogus number?\n", pc);
        s_putch(c);
    }
    else
    {
        /* Just a zero? */
        /* E.g. array[0] */
        err_remark("Just zero\n");
        s_putch(c);
    }
}

/*
** Parse identifiers.  Also parse strings and characters preceded by
** alphanumerics (raw strings, etc).  L"xxx" in all standard variants of
** C and C++. u"xxx", U"xxx", u8"xxx in C11 and C++11.  LR"xxx",
** uR"y(xxx)y", UR"y(xxx)y" and u8R"y(xxx)y" in C++11.
*/
static void parse_identifier(int c, FILE *fp, const char *fn)
{
    assert(isalpha(c) || c == '_');
    s_putch(c);
    /* TODO: finish this code */
    while ((c = peek(fp)) != EOF)
    {
        if (!is_idchar(c))
            break;
        c = getch(fp);
        s_putch(c);
    }
}

static Comment non_comment(int c, FILE *fp, const char *fn)
{
    Comment status = NonComment;
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
    else if (c == '/')
    {
        /* Potential start of comment */
        int bsnl = read_bsnl(fp);
        int pc;
        if ((pc = peek(fp)) == '*')
        {
            status = CComment;
            c = getch(fp);
            c_putch('/');
            write_bsnl(bsnl, c_putch);
            c_putch('*');
        }
        else if (f_DoubleSlash == 1 && pc == '/')
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
    else if (isdigit(c) || (c == '.' && isdigit(peek(fp))))
        parse_number(c, fp, fn);
    else if (isalnum(c) || c == '_')
        parse_identifier(c, fp, fn);
    else
    {
        /* space, punctuation, ... */
        s_putch(c);
    }
    return status;
}

static void scc(FILE *fp, char *fn)
{
    int oc;
    int c;
    Comment status = NonComment;

    l_nest = 0; /* Last line with a nested comment warning */
    l_cend = 0; /* Last line with a comment end warning */
    nline = 1;

    for (oc = '\0'; (c = getch(fp)) != EOF; oc = c)
    {
        switch (status)
        {
        case CComment:
            status = c_comment(c, fp, fn);
            break;
        case CppComment:
            status = cpp_comment(c, oc);
            break;
        case NonComment:
            status = non_comment(c, fp, fn);
            break;
        }
    }
    if (status != NonComment)
        warning("unterminated C-style comment", fn, nline);
}

static int parse_std_arg(const char *std)
{
    size_t len = strlen(std);
    char name[len + 1];
    for (size_t i = 0; i < len; i++)
        name[i] = tolower((unsigned char)std[i]);
    name[len] = '\0';
    for (size_t i = 0; i < NUM_STDNAMES; i++)
    {
        if (strcmp(name, std_name[i]) == 0)
            return i;
    }
    err_error("Unrecognized standard name %s\n", std);
    /*NOTREACHED*/
    return -1;
}

static void set_features(int code)
{
    switch (code)
    {
    case C89:
    case C94:
        break;
    case C:
    case C11:
        f_Unicode = true;
        /*FALLTHROUGH*/
    case C99:
        f_HexFloat = true;
        f_ExtendedIds = true;
        f_DoubleSlash = true;
        break;
    case CXX14:
        f_Binary = true;
        f_NumPunct = true;
        /*FALLTHROUGH*/
    case CXX:
    case CXX11:
        f_RawString = true;
        f_Unicode = true;
        /*FALLTHROUGH*/
    case CXX98:
    case CXX03:
        f_ExtendedIds = true;
        f_DoubleSlash = true;
        break;
    default:
        err_internal(__func__, "Invalid standard code %d\n", code);
        break;
    }
}

static void print_features(int code)
{
    printf("Standard: %s\n", std_name[code]);
    if (f_DoubleSlash)
        printf("Feature:  // comments\n");
    if (f_Trigraphs)
        printf("Feature:  Trigraphs\n");
    if (f_RawString)
        printf("Feature:  Raw strings\n");
    if (f_Unicode)
        printf("Feature:  Unicode strings (u\"A\", U\"A\", u8\"A\")\n");
    if (f_Binary)
        printf("Feature:  Binary constants 0b0101\n");
    if (f_HexFloat)
        printf("Feature:  Hexadecimal floats 0x2.34P-12\n");
    if (f_NumPunct)
        printf("Feature:  Numeric punctuation 0x1234'5678\n");
    if (f_ExtendedIds)
        printf("Feature:  Universal character names \\uXXXX and \\Uxxxxxxxx\n");
}

int main(int argc, char **argv)
{
    int opt;
    int std_code = C11;
    bool fflag = false;

    err_setarg0(argv[0]);

    while ((opt = getopt(argc, argv, optstr)) != EOF)
    {
        switch (opt)
        {
        case 'c':
            cflag = 1;
            break;
        case 'f':
            fflag = true;
            break;
        case 'h':
            err_help(usestr, hlpstr);
            break;
        case 'n':
            nflag = 1;
            break;
        case 'q':
            qflag = *optarg;
            break;
        case 's':
            sflag = *optarg;
            break;
        case 't':
            f_Trigraphs = true;
            break;
        case 'w':
            wflag = 1;
            break;
        case 'S':
            std_code = parse_std_arg(optarg);
            break;
        case 'V':
            err_version("SCC", "$Revision: 6.16 $ ($Date: 2015/08/16 00:36:26 $)");
            break;
        default:
            err_usage(usestr);
            break;
        }
    }

    set_features(std_code);
    if (fflag)
        print_features(std_code);

    filter(argc, argv, optind, scc);
    return(0);
}
