/*
@(#)File:           $RCSfile: scc.c,v $
@(#)Version:        $Revision: 6.31 $
@(#)Last changed:   $Date: 2016/06/13 04:34:04 $
@(#)Purpose:        Strip C comments
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1991,1993,1997-98,2003,2005,2007-08,2011-12,2014-16
*/

/*TABSTOP=4*/

/*
**  The SCC processor removes any C comments and replaces them by a
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
**  not need special handling.  Trigraphs do present a problem in theory
**  because ??/ is an encoding for backslash.  However, this program
**  ignores trigraphs altogether - calling upon GCC for precedent, and
**  noting that C++14 has deprecated trigraphs.  The JLSS programs
**  digraphs and trigraphs can manipulate (encode, decode) digraphs and
**  trigraphs.  This more of a theoretical problem than a practical one.
**
**  C++14 adds quotes inside numeric literals: 10'000'000 for 10000000,
**  10'000 for 10000, etc.  Unfortunately, that means SCC has to
**  recognize literal values fully, because these can appear in hex too:
**  0xFFFF'ABCD.  C++14 also adds binary constants: 0b0001'1010 (b or
**  B).  (See N3797 for draft C++14 standard.)
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
**  This renders the -C flag irrelevant and removed.
**
**  C11 and C++11 add support for u"literal", U"literal", u8"literal",
**  and for u'char' and U'char' (but not u8'char').  Previously, the
**  code needed no special handling for wide character strings L"x" or
**  constants L'x', but now they have to be handled appropriately.
*/

#include "posixver.h"
#include "filter.h"
#include "stderr.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef enum { NonComment, CComment, CppComment } Comment;

typedef enum
{
    C, C89, C90, C94, C99, C11,
    CXX, CXX98, CXX03, CXX11, CXX14, CXX17
} Standard;

enum { MAX_RAW_MARKER = 16 };
enum { LPAREN = '(', RPAREN = ')' }; 

static const char std_name[][6] =
{
    [C]     = "C",      // Current C standard
    [CXX]   = "C++",    // Current C++ standard
    [C89]   = "C89",
    [C90]   = "C90",
    [C94]   = "C94",
    [C99]   = "C99",
    [C11]   = "C11",
    [CXX98] = "C++98",
    [CXX03] = "C++03",
    [CXX11] = "C++11",
    [CXX14] = "C++14",
    [CXX17] = "C++17",
};
enum { NUM_STDNAMES = sizeof(std_name) / sizeof(std_name[0]) };

static const char * const dq_reg_prefix[] = { "L", "u", "U", "u8", };
static const char * const dq_raw_prefix[] = { "R", "LR", "uR", "UR", "u8R" };
enum { NUM_DQ_REG_PREFIX = sizeof(dq_reg_prefix) / sizeof(dq_reg_prefix[0]) };
enum { NUM_DQ_RAW_PREFIX = sizeof(dq_raw_prefix) / sizeof(dq_raw_prefix[0]) };

static int std_code = C11;  /* Selected standard */

static int cflag = 0;   /* Print comments and not code */
static int nflag = 0;   /* Keep newlines in comments */
static int qchar = 0;   /* Replacement character for quotes */
static int schar = 0;   /* Replacement character for strings */
static int wflag = 0;   /* Warn about nested C-style comments */

/* Features recognized */
static bool f_DoubleSlash = false;  /* // comments */
static bool f_RawString = false;    /* Raw strings */
static bool f_Unicode = false;      /* Unicode strings (u\"A\", U\"A\", u8\"A\") */
static bool f_Binary = false;       /* Binary constants 0b0101 */
static bool f_HexFloat = false;     /* Hexadecimal floats 0x2.34P-12 */
static bool f_NumPunct = false;     /* Numeric punctuation 0x1234'5678 */
static bool f_UniversalCharNames = false;  /* Universal character names \uXXXX and \Uxxxxxxxx */

static int nline = 0;   /* Line counter */
static int l_nest = 0;  /* Last line with a nested comment warning */
static int l_cend = 0;  /* Last line with a comment end warning */

static const char optstr[] = "cfhnq:s:wS:V";
static const char usestr[] = "[-cfhnwV][-S std][-s rep][-q rep] [file ...]";
static const char hlpstr[] =
    "  -c      Print comments and not the code\n"
    "  -f      Print flags in effect (debugging mainly)\n"
    "  -h      Print this help and exit\n"
    "  -n      Keep newlines in comments\n"
    "  -s rep  Replace the body of string literals with rep (a single character)\n"
    "  -q rep  Replace the body of character literals with rep (a single character)\n"
    "  -w      Warn about nested C-style comments\n"
    "  -S std  Specify language standard (C, C++, C89, C90, C99, C11, C++98, C++03,\n"
    "          C++11, C++14, C++17; default C11)\n"
    "  -V      Print version information and exit\n"
    ;

/* Sequencing issue - preferably to be resolved without predeclaration */
static Comment non_comment(int c, FILE *fp, const char *fn);

#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
extern const char jlss_id_scc_c[];
const char jlss_id_scc_c[] = "@(#)$Id: scc.c,v 6.31 2016/06/13 04:34:04 jleffler Exp $";
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
    if (!cflag || (nflag && c == '\n'))
        putchar(c);
}

/* Put comment (non-code) character */
static void c_putch(char c)
{
    if (cflag || (nflag && c == '\n'))
        putchar(c);
}

/* Output string of statement characters */
static void s_putstr(const char *str)
{
    char c;
    while ((c = *str++) != '\0')
        s_putch(c);
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

/* The warn_XxxYyy() functions are crying out for refactoring */
static void warn_HexFloat(const char *fn)
{
    assert(fn != 0);
    warning2("Hexadecimal floating point constant used but not supported in", std_name[std_code], fn, nline);
}

static void warn_RawString(const char *fn)
{
    assert(fn != 0);
    warning2("Raw string used but not supported in", std_name[std_code], fn, nline);
}

static void warn_DoubleSlash(const char *fn)
{
    assert(fn != 0);
    warning2("Double slash comment used but not supported in", std_name[std_code], fn, nline);
}

static void warn_Unicode(const char *fn)
{
    assert(fn != 0);
    warning2("Unicode feature used but not supported in", std_name[std_code], fn, nline);
}

static void warn_Binary(const char *fn)
{
    assert(fn != 0);
    warning2("Binary literal feature used but not supported in", std_name[std_code], fn, nline);
}

static inline void warn_NumPunct(const char *fn)
{
    assert(fn != 0);
    warning2("Numeric punctuation feature used but not supported in", std_name[std_code], fn, nline);
}

static inline void warn_UniversalCharNames(const char *fn)
{
    assert(fn != 0);
    warning2("Universal character names feature used but not supported in", std_name[std_code], fn, nline);
}

static void put_quote_char(char q, char c)
{
    if (q == '\'' && qchar != 0)
        s_putch(qchar);
    else if (q == '"' && schar != 0)
        s_putch(schar);
    else
        s_putch(c);
}

static void put_quote_str(char q, char *str)
{
    char c;
    while ((c = *str++) != '\0')
        put_quote_char(q, c);
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
            if ((c == 'u' || c == 'U') && !f_UniversalCharNames)
                warn_UniversalCharNames(fn);
            if (c == '\\' && peek(fp) == '\n')
                s_putch(getch(fp));
        }
        else if (c == '\n')
            warning2("newline in", msg, fn, nline - 1);
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

/* Backslash was read but not printed! u or U was peeked but not read */
/*
** There's no compelling reason to handle UCNs - the code is supposed to
** be valid before using SCC on it, and invalid UCNs there should not
** appear.  OTOH, to report their use when not supported, you have to
** detect their existence.
*/
static void scan_ucn(int letter, int nbytes, FILE *fp, const char *fn)
{
    assert(letter == 'u' || letter == 'U');
    assert(nbytes == 4 || nbytes == 8);
    bool ok = true;
    int i;
    char str[8];
    if (!f_UniversalCharNames)
        warn_UniversalCharNames(fn);
    s_putch('\\');
    int c = getch(fp);
    assert(c == letter);
    s_putch(c);
    for (i = 0; i < nbytes; i++)
    {
        c = getch(fp);
        if (c == EOF)
        {
            ok = false;
            break;
        }
        if (!isxdigit(c))
        {
            ok = false;
            s_putch(c);
            break;
        }
        str[i] = c;
        s_putch(c);
    }
    if (!ok)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "Invalid UCN \\%c%.*s%c detected", letter, i, str, c);
        warning(msg, fn, nline);
    }
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
    if (!f_NumPunct)
        warn_NumPunct(fn);
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

static inline void parse_exponent(FILE *fp, const char *fn)
{
    assert(fp != 0 && fn != 0);
    /* First character is known to be valid exponent (p, P, e, E) */
    int c = getch(fp);
    assert(c == 'e' || c == 'E' || c == 'p' || c == 'P');
    s_putch(c);
    int pc = peek(fp);
    int count = 0;
    if (pc == '+' || pc == '-')
        s_putch(getch(fp));
    while ((pc = peek(fp)) != EOF && isdigit(pc))
    {
        count++;
        s_putch(getch(fp));
    }
    if (count == 0)
    {
        char msg[80];
        snprintf(msg, sizeof(msg), "Exponent %c not followed by (optional sign and) one or more digits", c);
        warning(msg, fn, nline);
    }
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
    bool warned = false;
    while ((pc = peek(fp)) == '\'' || isxdigit(pc) || pc == '.')
    {
        if (pc == '\'')
            oc = check_punct(oc, fp, fn, isxdigit);
        else
        {
            if (pc == '.' && !f_HexFloat)
            {
                if (!warned)
                    warn_HexFloat(fn);
                warned = true;
            }
            oc = pc;
            s_putch(getch(fp));
        }
    }
    if (pc == 'p' || pc == 'P')
    {
        if (!f_HexFloat && !warned)
            warn_HexFloat(fn);
        parse_exponent(fp, fn);
    }
}

static void parse_binary(FILE *fp, const char *fn)
{
    /* Binary constant - integer */
    /* Should be followed by one or more binary digits */
    if (!f_Binary)
        warn_Binary(fn);
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
    /* Calling code checked for octal digit or s-quote */
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
    if (isdigit(pc))
        warningv("Non-octal digit %c in octal constant", fn, nline, pc);
}

static void parse_decimal(int c, FILE *fp, const char *fn)
{
    /* Decimal integer, or decimal floating point */
    s_putch(c);
    int pc = peek(fp);
    if (isdigit(pc) || pc == '\'')
    {
        c = getch(fp);
        assert(c == pc);
        s_putch(pc);
        int oc = c;
        while ((pc = peek(fp)) == '\'' || isdigit(pc))
        {
            /* Assuming isdigit alone generates a function pointer */
            if (pc == '\'')
                oc = check_punct(oc, fp, fn, isdigit);
            else
            {
                oc = pc;
                s_putch(getch(fp));
            }
        }
        if (pc == 'e' || pc == 'E')
            parse_exponent(fp, fn);
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
** 0xA'B'C.B'Cp-12  // C++17 Presumed punctuated Hex floating point
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
    else if ((pc == 'b' || pc == 'B'))
        parse_binary(fp, fn);
    else if (is_octal(pc) || pc == '\'')
        parse_octal(fp, fn);
    else if (pc == 'e' || pc == 'E' || pc == '.')
    {
        /* Simple fractional (0.1234) or zero floating point decimal constant 0E0 */
        parse_decimal(c, fp, fn);
    }
    else if (isdigit(pc))
    {
        /* Malformed number of some sort (09, for example) */
        err_remark("0%c read - bogus number!\n", pc);
        s_putch(c);
    }
    else
    {
        /* Just a zero? -- e.g. array[0] */
        s_putch(c);
    }
}

static void read_remainder_of_identifier(FILE *fp, const char *fn)
{
    int c;
    while ((c = peek(fp)) != EOF && is_idchar(c))
    {
        c = getch(fp);
        s_putch(c);
    }
}

static inline bool could_be_string_literal(char c)
{
    return(c == 'U' || c == 'u' || c == 'L' || c == 'R' || c == '8');
}

static bool valid_dq_raw_prefix(const char *prefix)
{
    for (int i = 0; i < NUM_DQ_RAW_PREFIX; i++)
    {
        if (strcmp(prefix, dq_raw_prefix[i]) == 0)
            return true;
    }
    return false;
}

static bool valid_dq_reg_prefix(const char *prefix)
{
    for (int i = 0; i < NUM_DQ_REG_PREFIX; i++)
    {
        if (strcmp(prefix, dq_reg_prefix[i]) == 0)
            return true;
    }
    return false;
}

static bool valid_dq_prefix(const char *prefix)
{
    return valid_dq_reg_prefix(prefix) || valid_dq_raw_prefix(prefix);
}

static bool raw_scan_marker(char *markstr, int *marklen, const char *pfx, FILE *fp, const char *fn)
{
    int len = 0;
    int c;
    char message[64];
    while ((c = getch(fp)) != EOF)
    {
        if (c == LPAREN)
        {
            /* End of marker */
            assert(len <= MAX_RAW_MARKER);
            markstr[len] = '\0';
            *marklen = len;
            return true;
        }
        else if (strchr("\") \\\t\v\f\n", c) != 0 || len >= MAX_RAW_MARKER)
        {
            /* Invalid mark character or marker is too long */
            if (len >= MAX_RAW_MARKER)
            {
                markstr[len++] = c;
                markstr[len] = '\0';
                snprintf(message, sizeof(message),
                         "Too long a raw string d-char-sequence: %s\"%.*s",
                         pfx, len, markstr);
            }
            else
            {
                char qc[10] = "";
                if (isgraph(c))
                    snprintf(qc, sizeof(qc), " '%s%c'",
                             ((c == '\'' || c == '\\') ? "\\" : ""), c);
                snprintf(message, sizeof(message),
                         "Invalid mark character (code %d%s) in d-char-sequence: %s\"%.*s",
                         c, qc, pfx, len, markstr);
            }
            warning(message, fn, nline);
            markstr[len++] = c;
            markstr[len] = '\0';
            *marklen = len;
            return false;
        }
        else
            markstr[len++] = c;
    }
    snprintf(message, sizeof(message),
             "Unexpected EOF in raw string d-char-sequence: %s\"%.*s",
             pfx, len, markstr);
    warning(message, fn, nline);
    markstr[len] = '\0';
    *marklen = len;
    return false;
}

/* Look for ) followed by markstr and double quote */
static void raw_scan_string(const char *markstr, int marklen, FILE *fp, const char *fn, int line1)
{
    int c;

    while ((c = getch(fp)) != EOF)
    {
        if (c != RPAREN)
            put_quote_char('"', c);
        else
        {
            char endstr[MAX_RAW_MARKER + 2];
            int len = 0;
            while ((c = getch(fp)) != EOF)
            {
                if (c == '"' && len == marklen)
                {
                    /* Got the end! */
                    s_putch(RPAREN);
                    s_putstr(markstr);
                    s_putch(c);
                    return;
                }
                else if (c == markstr[len])
                    endstr[len++] = c;
                else if (c == RPAREN)
                {
                    /* Restart scan for mark string */
                    endstr[len] = '\0';
                    put_quote_char('"', RPAREN);
                    put_quote_str('"', endstr);
                    len = 0;
                }
                else
                {
                    endstr[len] = '\0';
                    put_quote_char('"', RPAREN);
                    put_quote_str('"', endstr);
                    break;
                }
            }
        }
    }
    warning("Unexpected EOF in raw string starting at this line", fn, line1);
}

static void parse_raw_string(const char *prefix, FILE *fp, const char *fn)
{
    /*
    ** Have read up to and including the double quote at the start of a
    ** raw string literal (u8R" for example) and prefix but not double
    ** quote has been printed.  Now find lead mark and open parenthesis.
    ** NB: lead mark is not allowed to be longer than 16 characters.
    ** u8R"lead(data)lead" is valid, as is u8R"(data)".
    **
    ** In the standard, the lead mark characters are called 'd-char's: any
    ** member of the basic source character set except: space, the left
    ** parenthesis (, the right parenthesis ), the backslash \, and the
    ** control characters representing horizontal tab, vertical tab,
    ** form feed, and newline.
    **
    ** A string literal that has an R in the prefix is a raw string
    ** literal.  The d-char-sequence serves as a delimiter.  The
    ** terminating d-char-sequence of a raw-string is the same sequence
    ** of characters as the initial d-charsequence.  A d-char-sequence
    ** shall consist of at most 16 characters.
    **
    ** NB: The fact that backslash is not allowed in the marker means
    **     that double quote is also prohibited since it would have to
    **     be preceded by a backslash.
    **
    ** Processing:
    ** 1. Find valid lead mark - up to first (.
    ** 2. If invalid, report as such and process as ordinary dq-string.
    ** 3. Else find ) and lead mark followed by close dq.
    **    - NB: R"aa( )aa )aa" is valid; the first ")aa" is content
    **      because it is not followed by a double quote.
    ** 4. If EOF encountered first, report the problem.
    ** Save nline for start of literal.
    **
    ** NB: If replacing string characters, the raw string delimiters are
    **     printed unmapped, but the body of the raw string is printed
    **     as the replacement character.
    */
    assert(prefix != 0 && fp != 0 && fn != 0);
    char markstr[MAX_RAW_MARKER + 2];
    int  marklen;
    if (raw_scan_marker(markstr, &marklen, prefix, fp, fn))
    {
        s_putch('"');
        s_putstr(markstr);
        s_putch(LPAREN);
        raw_scan_string(markstr, marklen, fp, fn, nline);
    }
    else
    {
        s_putch('"');
        put_quote_str('"', markstr);
        endquote('"', fp, fn, "string literal");
    }
}

static void parse_dq_string(const char *prefix, FILE *fp, const char *fn)
{
    assert(valid_dq_prefix(prefix));
    if (valid_dq_raw_prefix(prefix))
    {
        if (!f_RawString)
            warn_RawString(fn);
        s_putstr(prefix);
        parse_raw_string(prefix, fp, fn);
    }
    else
    {
        if (strcmp(prefix, "L") != 0 && !f_Unicode)
            warn_Unicode(fn);
        s_putstr(prefix);
        (void)non_comment('"', fp, fn);
    }
}

static void process_poss_string_literal(char c, FILE *fp, const char *fn)
{
    char prefix[6] = "";
    int idx = 0;
    prefix[idx++] = c;
    while ((c = peek(fp)) != EOF)
    {
        if (c == '\'')
        {
            /* process sinqle quote */
            /* Curiously, it really doesn't matter if the prefix is valid or not */
            /* SCC will process it the same way, printing prefix and then processing single quote */
            s_putstr(prefix);
            c = getch(fp);
            (void)non_comment(c, fp, fn);
            break;
        }
        else if (c == '"')
        {
            /* process double quote - possibly raw */
            if (valid_dq_prefix(prefix))
            {
                c = getch(fp);
                parse_dq_string(prefix, fp, fn);
            }
            else
            {
                /* Invalid syntax - identifier followed by double quote */
                s_putstr(prefix);
                c = getch(fp);
                (void)non_comment(c, fp, fn);
            }
            break;
        }
        else if (could_be_string_literal(c))
        {
            c = getch(fp);
            prefix[idx++] = c;
            if (idx > 3)
            {
                s_putstr(prefix);
                read_remainder_of_identifier(fp, fn);
                break;
            }
            /* Only loop continuation */
        }
        else
        {
            s_putstr(prefix);
            read_remainder_of_identifier(fp, fn);
            break;
        }
    }
}

/*
** Parse identifiers.
** Also parse strings and characters preceded by alphanumerics (raw
** strings, Unicode strings, and some character literals).
** L"xxx" in all standard variants of C and C++.
** u"xxx", U"xxx", u8"xxx from C11 and C++11 onwards.
** R"y(xxx)y", LR"y(xxx)y", uR"y(xxx)y", UR"y(xxx)y" and u8R"y(xxx)y" in C++11 onwards.
** L'x' in all standard variants of C and C++.
** U'x' and u'x' from C11 and C++11 onwards.
** No space is allowed between the prefix and the quote.
**
** NB: UCNs in an identifier are parsed independently of 'identifier'.
*/
static void parse_identifier(int c, FILE *fp, const char *fn)
{
    assert(isalpha(c) || c == '_');
    if (could_be_string_literal(c))
        process_poss_string_literal(c, fp, fn);
    else
    {
        s_putch(c);
        read_remainder_of_identifier(fp, fn);
    }
}

static Comment non_comment(int c, FILE *fp, const char *fn)
{
    int pc;
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
        endquote(c, fp, fn, "character constant");
    }
    else if (c == '"')
    {
        s_putch(c);
        /* Double quotes are relatively simple, except that */
        /* they can legitimately extend over several lines */
        /* when each line is terminated by a backslash */
        endquote(c, fp, fn, "string literal");
    }
    else if (c == '/')
    {
        /* Potential start of comment */
        int bsnl = read_bsnl(fp);
        if ((pc = peek(fp)) == '*')
        {
            status = CComment;
            c = getch(fp);
            c_putch('/');
            write_bsnl(bsnl, c_putch);
            c_putch('*');
        }
        else if (!f_DoubleSlash && pc == '/')
        {
            warn_DoubleSlash(fn);
            c = getch(fp);
            s_putch(c);
            write_bsnl(bsnl, s_putch);
            s_putch(c);
        }
        else if (f_DoubleSlash && pc == '/')
        {
            status = CppComment;
            c = getch(fp);
            c_putch(c);
            write_bsnl(bsnl, c_putch);
            c_putch(c);
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
    else if (c == '\\' && ((pc = peek(fp)) == 'u' || pc == 'U'))
        scan_ucn(pc, (pc == 'u' ? 4 : 8), fp, fn);
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
        name[i] = toupper((unsigned char)std[i]);
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
    case C90:
    case C94:
        break;
    case C:
    case C11:
        f_Unicode = true;
        /*FALLTHROUGH*/
    case C99:
        f_HexFloat = true;
        f_UniversalCharNames = true;
        f_DoubleSlash = true;
        break;
    case CXX17:
        f_HexFloat = true;
        /*FALLTHROUGH*/
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
        f_UniversalCharNames = true;
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
    if (f_UniversalCharNames)
        printf("Feature:  Universal character names \\uXXXX and \\Uxxxxxxxx\n");
}

int main(int argc, char **argv)
{
    int opt;
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
            qchar = *optarg;
            break;
        case 's':
            schar = *optarg;
            break;
        case 'w':
            wflag = 1;
            break;
        case 'S':
            std_code = parse_std_arg(optarg);
            break;
        case 'V':
            err_version("SCC", &"@(#)6.60 (2016-06-12)"[4]);
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
