/*
@(#)File:           $RCSfile: stderr.c,v $
@(#)Version:        $Revision: 8.29 $
@(#)Last changed:   $Date: 2008/06/02 13:00:00 $
@(#)Purpose:        Error reporting routines
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1988-91,1996-99,2001,2003,2005-08
@(#)Product:        :PRODUCT:
*/

/*TABSTOP=4*/

/*
** Configuration:
** USE_STDERR_SYSLOG   - include syslog functionality
** USE_STDERR_FILEDESC - include file descriptor functionality
*/

#include "stderr.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
extern int getpid(void);
#endif /* HAVE_UNISTD_H */

enum { MAX_MSGLEN = 2048 };

#if defined(HAVE_SYSLOG_H) && defined(HAVE_SYSLOG) && defined(USE_STDERR_SYSLOG)
#include <syslog.h>
const char jlss_id_stderr_c_with_syslog[] =
        "@(#)" __FILE__ " configured with USE_STDERR_SYSLOG";
#else
#undef USE_STDERR_SYSLOG
#undef HAVE_SYSLOG_H
#undef HAVE_SYSLOG
#endif /* syslog configuration */

#if defined(USE_STDERR_FILEDESC)
const char jlss_id_stderr_c_with_filedesc[] =
        "@(#)" __FILE__ " configured with USE_STDERR_FILEDESC";
#endif /* USE_STDERR_FILEDESC */

/* Global format strings */
const char err_format1[] = "%s\n";
const char err_format2[] = "%s %s\n";

static const char def_format[] = "%Y-%m-%d %H:%M:%S";
static const char *tm_format = def_format;
static char arg0[ERR_MAXLEN_ARGV0+1] = "**undefined**";

/* Where do messages go?
**  if   (defined USE_STDERR_SYSLOG && errlog != 0) ==> syslog
**  elif (err_fd >= 0)                       ==> file descriptor
**  else                                     ==> file pointer
*/
static int   errlog =  0;
static FILE *errout =  0;
static int   err_fd = -1;

/*
** err_???_print() functions are named systematically, and are all static.
**
** err_[ve][xr][fn]_print():
** --   v   takes va_list argument
** --   e   takes ellipsis argument
** --   x   exits (no return)
** --   r   returns (no exit)
** --   f   takes file pointer
** --   n   no file pointer (use errout)
**
** NB: no-return and printf-like can only be attached to declarations, not definitions.
*/

static void err_vxf_print(FILE *fp, int flags, int estat, const char *format, va_list args)
                NORETURN();
static void err_vxn_print(int flags, int estat, const char *format, va_list args)
                NORETURN();
static void err_exn_print(int flags, int estat, const char *format, ...)
                NORETURN() PRINTFLIKE(3,4);
static void err_terminate(int flags, int estat) NORETURN();

#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
const char jlss_id_stderr_c[] = "@(#)$Id: stderr.c,v 8.29 2008/06/02 13:00:00 jleffler Exp $";
#endif /* lint */

/* Change the definition of 'stderr', reporting on the old one too */
/* NB: using err_stderr((FILE *)0) simply reports the current 'stderr' */
FILE *(err_stderr)(FILE *newerr)
{
    FILE *old;

    if (errout == 0)
        errout = stderr;
    old = errout;
    if (newerr != 0)
        errout = newerr;
    return(old);
}

/* Change the definition of 'stderr', reporting on the old one too */
/* NB: using err_use_fd() with a negative value turns off 'errors to file descriptor' */
int (err_use_fd)(int new_fd)
{
    int old_fd = err_fd;

#if defined(USE_STDERR_FILEDESC)
    if (new_fd < 0)
        new_fd = -1;
    err_fd = new_fd;
#endif /* USE_STDERR_FILEDESC */
    return(old_fd);
}

/*
** Configure the use of syslog
** If not configured to use syslog(), this is a no-op.
** If configured to use syslog(), the facility argument should be one of
** the standard facilities (POSIX defines LOG_USER and LOG_LOCAL0 to
** LOG_LOCAL7) to turn on syslog(), or a negative value to turn it off.
** The logopts should be the bitwise combination of 0 and the options
** LOG_PID, LOG_CONS, LOG_NDELAY, LOG_ODELAY, LOG_NOWAIT.  However, the
** STDERR package sets LOG_PID regardless.
** The ident used in openlog() corresponds to the value in arg0.
** Note that when formatting the message for syslog(), the time, the PID
** and arg0 are not needed (and hence not provided).  The downside is
** you are stuck with the date formatted by syslog().
*/
int (err_use_syslog)(int logopts, int facility)
{
#if defined(USE_STDERR_SYSLOG)
    if (facility < 0)
    {
        /* Turn off syslog() */
        closelog();
        errlog = 0;
    }
    else
    {
        openlog(arg0, LOG_PID|logopts, facility);
        errlog = 1;
    }
#endif /* USE_STDERR_SYSLOG */
    return(errlog);
}

/* Return stored basename of command */
const char *(err_getarg0)(void)
{
    return(arg0);
}

/* Store basename of command, excluding trailing slashes */
void (err_setarg0)(const char *argv0)
{
    /* Handle three pathological cases -- NULL, "/" and "" */
    if (argv0 != 0 && *argv0 != '\0' && (*argv0 != '/' || *(argv0 + 1) != '\0'))
    {
        const char *cp;
        size_t nbytes = sizeof(arg0) - 1;

        if ((cp = strrchr(argv0, '/')) != (char *)0 && *(cp + 1) == '\0')
        {
            /* Skip backwards over trailing slashes */
            const char *ep = cp;
            while (ep > argv0 && *ep == '/')
                ep--;
            /* Skip backwards over non-slashes */
            cp = ep;
            while (cp > argv0 && *cp != '/')
                cp--;
            cp++;
            nbytes = ep - cp + 1;
            if (nbytes > sizeof(arg0) - 1)
                nbytes = sizeof(arg0) - 1;
        }
        else if (cp != (char *)0)
        {
            /* Regular pathname containing slashes */
            cp++;
        }
        else
        {
            /* Basename of file only */
            cp = argv0;
        }
        strncpy(arg0, cp, nbytes);
        arg0[nbytes] = '\0';
    }
}

const char *(err_rcs_string)(const char *s2, char *buffer, size_t buflen)
{
    const char *src = s2;
    char *dst = buffer;
    char *end = buffer + buflen - 1;

    /*
    ** Bother RCS!  We've probably been given something like:
    ** "$Revision: 8.29 $ ($Date: 2008/06/02 13:00:00 $)"
    ** We only want to emit "7.5 (2001/08/11 06:25:48)".
    ** Skip the components between '$' and ': ', copy up to ' $',
    ** repeating as necessary.  And we have to test for overflow!
    ** Also work with the unexpanded forms of keywords ($Keyword$).
    ** Never needed this with SCCS!
    */
    while (*src != '\0' && dst < end)
    {
        while (*src != '\0' && *src != '$')
        {
            *dst++ = *src++;
            if (dst >= end)
                break;
        }
        if (*src == '$')
            src++;
        while (*src != '\0' && *src != ':' && *src != '$')
            src++;
        if (*src == '\0')
            break;
        if (*src == '$')
        {
            /* Unexpanded keyword '$Keyword$' notation */
            src++;
            continue;
        }
        if (*src == ':')
            src++;
        if (*src == ' ')
            src++;
        while (*src != '\0' && *src != '$')
        {
            *dst++ = *src++;
            if (dst >= end)
                break;
        }
        if (*src == '$')
        {
            if (*(dst-1) == ' ')
                dst--;
            src++;
        }
    }
    *dst = '\0';
    return(buffer);
}

/* Format a time string for now (using ISO8601 format) */
/* Allow for future settable time format with tm_format */
static char *err_time(char *buffer, size_t buflen)
{
    time_t  now;
    struct tm *tp;

    now = time((time_t *)0);
    tp = localtime(&now);
    strftime(buffer, buflen, tm_format, tp);
    return(buffer);
}

/* err_stdio - report error via stdio */
static void (err_stdio)(FILE *fp, int flags, int errnum, const char *format, va_list args)
{
    if ((flags & ERR_NOARG0) == 0)
        fprintf(fp, "%s: ", arg0);
    if (flags & ERR_STAMP)
    {
        char timbuf[32];
        fprintf(fp, "%s - ", err_time(timbuf, sizeof(timbuf)));
    }
    if (flags & ERR_PID)
        fprintf(fp, "pid=%d: ", (int)getpid());
    vfprintf(fp, format, args);
    if (flags & ERR_ERRNO)
        fprintf(fp, "error (%d) %s\n", errnum, strerror(errnum));
}

#if defined(USE_STDERR_FILEDESC) || defined(USE_STDERR_SYSLOG)
static char *fmt_string(char *curr, const char *end, const char *format, va_list args)
{
    char *new_end = curr;
    if (curr < end - 1)
    {
        int more = vsnprintf(curr, end - curr, format, args);
        if (more >= 0)
            new_end += more;
    }
    return(new_end);
}

static char *fmt_strdots(char *curr, const char *end, const char *format, ...)
{
    va_list args;
    char *new_end;
    va_start(args, format);
    new_end = fmt_string(curr, end, format, args);
    va_end(args);
    return new_end;
}
#endif /* USE_STDERR_FILEDESC || USE_STDERR_SYSLOG */

#if defined(USE_STDERR_SYSLOG)
/* err_syslog() - report error via syslog
**
** syslog() automatically adds PID and program name (configured in openlog()) and time stamp.
*/
static void (err_syslog)(int flags, int errnum, const char *format, va_list args)
{
    char buffer[MAX_MSGLEN];
    char *curpos = buffer;
    char *bufend = buffer + sizeof(buffer);
    int priority;

    curpos = fmt_string(curpos, bufend, format, args);
    if (flags & ERR_ERRNO)
        curpos = fmt_strdots(curpos, bufend,
                            "error (%d) %s\n", errnum, strerror(errnum));

    if (flags & ERR_ABORT)
        priority = LOG_CRIT;
    else if (flags & ERR_EXIT)
        priority = LOG_ERR;
    else
        priority = LOG_WARNING;
    syslog(priority, "%s", buffer);
}
#endif /* USE_STDERR_SYSLOG */

#if defined(USE_STDERR_FILEDESC)
/* err_filedes() - report error via file descriptor */
static void (err_filedes)(int fd, int flags, int errnum, const char *format, va_list args)
{
    char buffer[MAX_MSGLEN];
    char *curpos = buffer;
    char *bufend = buffer + sizeof(buffer);

    buffer[0] = '\0';   /* Not strictly necessary */
    if ((flags & ERR_NOARG0) == 0)
        curpos = fmt_strdots(curpos, bufend, "%s: ", arg0);
    if (flags & ERR_STAMP)
    {
        char timbuf[32];
        curpos = fmt_strdots(curpos, bufend,
                             "%s - ", err_time(timbuf, sizeof(timbuf)));
    }
    if (flags & ERR_PID)
        curpos = fmt_strdots(curpos, bufend,
                             "pid=%d: ", (int)getpid());
    curpos = fmt_string(curpos, bufend, format, args);
    if (flags & ERR_ERRNO)
        curpos = fmt_strdots(curpos, bufend,
                             "error (%d) %s\n", errnum, strerror(errnum));
    /* There's no sensible way to handle short writes! */
    write(fd, buffer, curpos - buffer);
}
#endif /* USE_STDERR_FILEDESC */

/* Most fundamental (and flexible) error message printing routine - always returns */
static void (err_vrf_print)(FILE *fp, int flags, const char *format, va_list args)
{
    int errnum = errno;     /* Capture errno before it is damaged! */

    if ((flags & ERR_NOFLUSH) == 0)
        fflush(0);

#if defined(USE_STDERR_SYSLOG)
    if (errlog)
        err_syslog(flags, errnum, format, args);
    else
#endif /* USE_STDERR_SYSLOG */
#if defined(USE_STDERR_FILEDESC)
    if (err_fd >= 0)
        err_filedes(err_fd, flags, errnum, format, args);
    else
#endif /* USE_STDERR_FILEDESC */
        err_stdio(fp, flags, errnum, format, args);

    fflush(fp);
}

/* Terminate program - abort or exit */
static void err_terminate(int flags, int estat)
{
    assert(flags & (ERR_ABORT|ERR_EXIT));
    if (flags & ERR_ABORT)
        abort();
    exit(estat);
}

/* Most fundamental (and flexible) error message printing routine - may return */
static void (err_vf_print)(FILE *fp, int flags, int estat, const char *format, va_list args)
{
    err_vrf_print(fp, flags, format, args);
    if (flags & (ERR_ABORT|ERR_EXIT))
        err_terminate(flags, estat);
}

/* Analog of err_vf_print() which guarantees 'no return' */
static void (err_vxf_print)(FILE *fp, int flags, int estat, const char *format, va_list args)
{
    err_vrf_print(fp, flags, format, args);
    err_terminate(flags, estat);
}

/* External interface to err_vf_print() - may return */
void (err_logmsg)(FILE *fp, int flags, int estat, const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_vf_print(fp, flags, estat, format, args);
    va_end(args);
}

/* Print error message to current error output - no return */
static void (err_vxn_print)(int flags, int estat, const char *format, va_list args)
{
    if (errout == 0)
        errout = stderr;
    err_vxf_print(errout, flags, estat, format, args);
}

/* Print error message to current error output - no return */
static void err_exn_print(int flags, int estat, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    err_vxn_print(flags, estat, format, args);
    va_end(args);
}

/* Print error message to nominated output - always returns */
static void err_erf_print(FILE *fp, int flags, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    err_vrf_print(fp, flags, format, args);
    va_end(args);
}

/* Print message using current error file */
void (err_print)(int flags, int estat, const char *format, va_list args)
{
    if (errout == 0)
        errout = stderr;
    err_vf_print(errout, flags, estat, format, args);
}

static void err_vrn_print(int flags, const char *format, va_list args)
{
    if (errout == 0)
        errout = stderr;
    err_vrf_print(errout, flags, format, args);
}

/* Report warning including message from errno */
void (err_sysrem)(const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_vrn_print(ERR_SYSREM, format, args);
    va_end(args);
}

/* Report error including message from errno */
void (err_syserr)(const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_vxn_print(ERR_SYSERR, ERR_STAT, format, args);
    va_end(args);
}

/* Report warning */
void (err_remark)(const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_vrn_print(ERR_REM, format, args);
    va_end(args);
}

/* Report error */
void (err_error)(const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_vxn_print(ERR_ERR, ERR_STAT, format, args);
    va_end(args);
}

/* Report message - sometimes exiting too */
void (err_report)(int flags, int estat, const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_print(flags, estat, format, args);
    va_end(args);
}

/* Print usage message and exit with failure status */
void (err_usage)(const char *s1)
{
    err_exn_print(ERR_NOARG0|ERR_EXIT, EXIT_FAILURE, "Usage: %s %s\n", err_getarg0(), s1);
}

/* Report failure and generate core dump */
void (err_abort)(const char *format, ...)
{
    va_list         args;

    va_start(args, format);
    err_vxn_print(ERR_ABORT, EXIT_FAILURE, format, args);
    va_end(args);
}

/* Report version information (no exit), removing embedded RCS keyword strings (but not values) */
void (err_printversion)(const char *program, const char *verinfo)
{
    char buffer[64];

    if (strchr(verinfo, '$'))
        verinfo = err_rcs_string(verinfo, buffer, sizeof(buffer));
    err_erf_print(stdout, ERR_DEFAULT, "%s Version %s\n", program, verinfo);
}

/* Report version information and exit, removing embedded RCS keyword strings (but not values) */
void (err_version)(const char *program, const char *verinfo)
{
    err_printversion(program, verinfo);
    exit(EXIT_SUCCESS);
}

/* Report an internal error and exit */
/* Abort if JLSS_INTERNAL_ERROR_ABORT set in environment */
void (err_internal)(const char *function, const char *msg)
{
    int flags = ERR_EXIT;
    const char *ev = getenv("JLSS_INTERNAL_ERROR_ABORT");

    if (ev != 0 && *ev != '\0')
        flags = ERR_ABORT;  /* Generate core dump */
    err_exn_print(flags, EXIT_FAILURE, "internal error in %s(): %s\n", function, msg);
}

#ifdef TEST

#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>

static const char *list[] =
{
    "/usr/fred/bloggs",
    "/usr/fred/bloggs/",
    "/usr/fred/bloggs////",
    "bloggs",
    "/.",
    ".",
    "/",
    "//",
    "///",
    "////",
    "",
    (char *)0
};

/* Fork a child process and have it run the given function. */
/* Parent waits and reports the pid and raw exit status */
static void test_fork(void (*function)(void))
{
    pid_t pid;
    int   status;

    if ((pid = fork()) == 0)
    {
        (*function)();
        /* NOTREACHED */
    }
    else if (pid == -1)
        err_sysrem("failed to fork - ");
    else
    {
        pid = wait(&status);
        err_logmsg(stdout, ERR_PID|ERR_REM, EXIT_SUCCESS,
                   "Child %d exited with status 0x%04x\n", (int)pid, status);
    }
}

static void test_version(void)
{
    err_logmsg(stdout, ERR_PID|ERR_REM, EXIT_SUCCESS,
               err_format1, "reporting on version!");
    err_version("STDERR", "$Revision: 8.29 $ ($Date: 2008/06/02 13:00:00 $)");
}

static void test_abort(void)
{
    err_logmsg(stdout, ERR_PID|ERR_REM, EXIT_SUCCESS,
               err_format1, "doing an abort!");
    err_abort("core dump intentional\n");
}

static void test_usage(void)
{
    err_logmsg(stdout, ERR_PID|ERR_REM, EXIT_SUCCESS,
               err_format1, "reporting on usage!");
    err_usage("[arg ...]");
}

static void test_by_forking(void)
{
    /* Now we have test_fork(), should test error exits, etc. */
    /* Including, especially, non-standard exit statuses! */
    test_fork(test_usage);
    test_fork(test_version);

    /* Test the abort code and clean up the core file it should generate! */
    test_fork(test_abort);
    errno = 0;
    if (unlink("core") != 0)
        err_sysrem("removed core file - ");
    else
        err_remark("removed core file\n");
}

int main(int argc, char **argv)
{
    const char **name;
    char *data;
    FILE *oldfp;

    err_setarg0(argv[0]);

    err_printversion("STDERR", &"@(#)$Revision: 8.29 $ ($Date: 2008/06/02 13:00:00 $)"[4]);
    err_logmsg(stdout, ERR_LOG, EXIT_SUCCESS, "testing ERR_LOG\n");
    err_logmsg(stdout, ERR_STAMP|ERR_REM, EXIT_SUCCESS,
                "testing ERR_STAMP\n");
    err_logmsg(stdout, ERR_PID|ERR_REM, EXIT_SUCCESS,
                "testing ERR_PID\n");
    errno = EXDEV;
    err_logmsg(stdout, ERR_ERRNO|ERR_REM, EXIT_SUCCESS,
                "testing ERR_ERRNO\n");

    oldfp = err_stderr(stdout);
    assert(oldfp == stderr);
    err_remark("Here's a remark with argument %d (%s)\n", 3, "Hello");
    assert(errno == EXDEV);
    err_sysrem("Here's a remark with a system error %d\n", errno);
    oldfp = err_stderr(stderr);
    assert(oldfp == stdout);

    err_remark(err_format1, "testing values for argv[0]");

    for (name = list; *name != (char *)0; name++)
    {
        data = (char *)malloc(strlen(*name) + 1);   /*=C++=*/
        strcpy(data, *name);
        printf("name = <<%s>>; ", *name);
        err_setarg0(*name);
        printf(" (<<%s>>) arg0 = <<%s>>\n", *name, err_getarg0());
        free(data);
    }

    err_setarg0(argv[0]);
    if (argc > 1)
    {
        err_remark(err_format1, "reporting arguments to program");
        while (*++argv != (char *)0)
            err_remark(err_format2, "next argument", *argv);
    }
    else
        err_remark(err_format1, "no arguments passed to program");

    test_by_forking();

#if defined(USE_STDERR_SYSLOG)
    err_use_syslog(LOG_CONS|LOG_NOWAIT, LOG_USER);
    test_by_forking();
    err_use_syslog(0, -1);
#endif /* USE_STDERR_SYSLOG */

#if defined(USE_STDERR_FILEDESC)
    {
        const char stderr_log[] = "stderr.log";
        int fd;

        (void)unlink(stderr_log);
        fd = open(stderr_log, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0644);
        if (fd < 0)
            err_sysrem("failed to open file %s - ", stderr_log);
        else
        {
            err_use_fd(fd);
            test_by_forking();
            close(fd);
            err_use_fd(-1);
            if ((fd = open(stderr_log, O_RDONLY, 0)) < 0)
                err_syserr("failed to reopen %s: ", stderr_log);
            else
            {
                char buffer[2048];
                ssize_t n;
                fprintf(stderr, "*** Contents of %s:\n", stderr_log);
                while ((n = read(fd, buffer, sizeof(buffer))) > 0)
                {
                    if (write(STDOUT_FILENO, buffer, n) != n)
                        err_syserr("failed to write to standard output!\n");
                }
                fprintf(stderr, "*** End of %s:\n", stderr_log);
            }
            if (unlink(stderr_log) != 0)
                err_syserr("failed to unlink %s: ", stderr_log);
        }
    }
#endif /* USE_STDERR_FILEDESC */

    err_remark("Completed testing - normal exit\n");
    return(0);
}

#endif /* TEST */
