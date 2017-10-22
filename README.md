# scc-snapshots
SCC: Strip C Comments — Snapshots

This repository contains the code for the SCC (Strip C Comments) program described in
[Remove Comments from C/C++ Code](http://stackoverflow.com/questions/2394017/) on Stack Overflow.

There are currently 7 releases:
* 6.70 (2017-10-17) - stable release
* 6.60 (2016-06-12) - stable release
* 6.50 (2016-06-12) — pre-release
* 6.16 (2016-01-19) — pre-release
* 5.05 (2012-01-23) - stable release
* 4.04 (2008-11-27) - stable release
* 4.03 (2008-06-07) - stable release

These are tagged release/x.yz.  The code is all on branch master.

Versions 4.03 and 4.04 are preliminary versions, possibly of historical
interest.
Functionally, they're almost identical, with minor code cleanups between
the two versions.
The meaning of the `-C` option was the opposite of the more modern
versions; it enabled the removal of C++ style comments.

Version 5.05 handles C and older C++ code.
It does not understand raw strings, or some of the other new features in
C++ (binary constants, punctuation in numbers, etc.).
Compared to the earlier versions, it adds the `-s str` and `-q rep`
options to replace the bodies of strings or character literals; it adds
the `-c` option to print the comments instead of the code; it adds the
`-n` option to preserve newlines in comments, to make it easier to
back-track from stripped source to the original code.

Version 6.60 handles C and newer C++ code.
It understands raw strings, binary constants and punctuation in numbers.
It has a different, slightly more complex interface compared with version 5.05.

Version 6.70 provides some occasionally useful new functionality.
The `-e` option emits empty comments `/* */` or `//` (followed by a
newline) instead of a space.
This can simplify some code analyses.
The `-c` option has been upgraded to generate a newline after each line
of comments.
This makes the output (much) easier to read.
The flags (`-f`) option has more systematic names for the features.

No version of SCC handles trigraphs (and no version needs to handle
digraphs).
There are separate programs for adding or removing trigraphs and
digraphs (called, unimaginatively, `trigraphs` and `digraphs`).
