# scc-snapshots
SCC: Strip C Comments — Snapshots

This repository contains the code for the SCC (Strip C Comments) program
described in [Remove Comments from C/C++
Code](http://stackoverflow.com/questions/2394017/) on Stack Overflow.

There are currently 8 releases:
* 8.0.2 (2022-05-30) - stable release
* 8.0.1 (2022-05-21) - stable release
* 6.80  (2017-10-26) - stable release
* 6.70  (2017-10-17) - stable release
* 6.60  (2016-06-12) - stable release
* 6.50  (2016-06-12) — pre-release
* 6.16  (2016-01-19) — pre-release
* 5.05  (2012-01-23) - stable release
* 4.04  (2008-11-27) - stable release
* 4.03  (2008-06-07) - stable release

These are tagged release/x.yz.  The code is all on branch master.

### Version 8.0.2 - 2022-05-30

A bug fix release, dealing with GitHub issue 2, a bug in the handling of
regular strings (and also character constants) reported by Oleg
Skinderev.

### Version 8.0.1 - 2022-05-21

Primarily a bug fix release, dealing with GitHub issue 1, a bug in C++
'raw string' handling reported by Oleg Skinderev.

However, prior to that, 'internal' releases 7.00 (2018-11-11), 7.10
(2018-11-12), 7.20 (2018-11-21), 7.30 (2019-01-27), 7.40 (2019-05-01)
7.50 (2020-03-03), 7.80.0 (2022-01-06) and 8.0.0 (2022-04-07) had been
created.
These versions are not directly reflected in this Git repository, but
the changes are present in 8.0.1.
If you have an urgent need for intermediate versions (why?), contact
Jonathan Leffler by email.

By default, SCC now strips trailing blanks; use the `-t` option to keep
them.
It no longer reports "bogus number" for 08 because that is a valid
preprocessing number, even though it is isn't a valid octal constant.
The `-f` (features) option lists the features and exits (rather than
trying to process a file too).
The version numbering was changed to 'semantic versioning'
(https://semver.org/) and is now managed independently of the RCS file
version numbers (RCS is still used to manage the 'internal releases').

### Version 6.80 - 2017-10-26

Version 6.80 changes some of the testing technology because RHEL 5.3
(sigh!) has a version of Bash that does not recognize process
substitution (`<(cmd args)` notation).
Three test scripts failed.
The product source was unchanged.

### Version 6.70 - 2017-10-17

Version 6.70 provides some occasionally useful new functionality.
The `-e` option emits empty comments `/* */` or `//` (followed by a
newline) instead of a space.
This can simplify some code analyses.
The `-c` option has been upgraded to generate a newline after each line
of comments.
This makes the output (much) easier to read.
The flags (`-f`) option has more systematic names for the features.

### Version 6.60 - 2016-06-12

Version 6.60 handles C and newer C++ code.
It understands raw strings, binary constants and punctuation in numbers.
It has a different, slightly more complex interface compared with version 5.05.

### Version 5.05 - 2012-01-23

Version 5.05 handles C and older C++ code.
It does not understand raw strings, or some of the other new features in
C++ (binary constants, punctuation in numbers, etc.).
Compared to the earlier versions, it adds the `-s str` and `-q rep`
options to replace the bodies of strings or character literals; it adds
the `-c` option to print the comments instead of the code; it adds the
`-n` option to preserve newlines in comments, to make it easier to
back-track from stripped source to the original code.

### Version 4.04 - 2008-11-27
### Version 4.03 - 2008-06-07

Versions 4.03 and 4.04 are preliminary versions, possibly of historical
interest.
Functionally, they're almost identical, with minor code cleanups between
the two versions.
The meaning of the `-C` option was the opposite of the more modern
versions; it enabled the removal of C++ style comments.

## Digraphs and trigraphs

No version of SCC handles trigraphs (and no version needs to handle
digraphs).
There are separate programs for adding or removing trigraphs and
digraphs (called, unimaginatively, `trigraphs` and `digraphs`).

