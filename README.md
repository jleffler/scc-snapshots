# scc-snapshots
SCC: Strip C Comments — Snapshots

This repository contains the code for the SCC (Strip C Comments) program described in
[Remove Comments from C/C++ Code](http://stackoverflow.com/questions/2394017/) on Stack Overflow.

There are currently 6 releases:
* 4.03 (2008-06-07)
* 4.04 (2008-11-27)
* 5.05 (2012-01-23)
* 6.16 (2016-01-19) — pre-release
* 6.50 (2016-06-12) — pre-release
* 6.60 (2016-06-12)

These are tagged release/x.yz.  The code is all on branch master.

Version 5.05 handles C and older C++ code.
It does not understand raw strings, or some of the other new features in C++
(binary constants, punctuation in numbers, etc.).

Version 6.60 handles C and newer C++ code.
It understands raw strings, binary constants and punctuation in numbers.
It has a different, slightly more complex interface compared with version 5.05.

Neither version handles trigraphs (and neither version needs to handle digraphs).
There are separate programs for adding or removing trigraphs and digraphs (called,
unimaginatively, `trigraphs` and `digraphs`).
