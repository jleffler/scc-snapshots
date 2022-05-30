/*
@(#)File:           $RCSfile: scc-test.example2.c,v $
@(#)Version:        $Revision: 1.1 $
@(#)Last changed:   $Date: 2015/08/17 04:27:24 $
@(#)Purpose:        Test SCC on core functionality
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1997,2003,2007,2013-15
@(#)Product:        SCC Version 8.0.3 (2022-05-30)
*/

/* Mainly for testing */
/* Retained comments */
Including trailing // C++ style comments
What is the correct behaviour?  // The newline is part of the comment.
// The newline should be printed.
And with comment stripping?  // The newline should probably still be printed?
So that this text appears on a separate line from the question.
And what about newlines and /* C style */ comments when they are retained?
Perhaps a newline should be generated when the line had a comment on it somewhere?
Which function /* or functions */ controls this?

/*EOF*/
