/*
@(#)File:           $RCSfile: scc-bogus.ucns.c,v $
@(#)Version:        $Revision: 1.1 $
@(#)Last changed:   $Date: 2016/06/10 04:10:55 $
@(#)Purpose:        Test SCC on broken Unicode characters
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2016
@(#)Product:        SCC Version 8.0.1 (2022-05-21)
*/

/* Compile with -fextended-identifiers */

/*TABSTOP=4*/

extern int id\u0xyz;        // Bogus
extern int id\u01xyz;       // Bogus
extern int id\u012xyz;      // Bogus
extern int id\u0123xyz;     // OK
extern int \U000AFFF0;      // OK
extern int \U000AFFFxyz;    // Bogus
extern int \U000AFFxyz;     // Bogus
extern int \U000AFxyz;      // Bogus
extern int \U000Axyz;       // Bogus
extern int \U000xyz;        // Bogus
extern int \U0Fxyz;         // Bogus
extern int \U0xyz;          // Bogus

/* Ranges of valid characters - see Annex D of ISO/IEC 9899:2011 */
/* Characters in a Private Use Area (PUA) are not allowed */
extern char bogus_ucns[];
char bogus_ucns[] =  // These UCNs are all invalid
    "\u0000"
    "\u0023"
    "\u0025"
    "\u003F"
    "\u0041"
    "\u005F"
    "\u0061"
    "\uD800"            // Smallest high-surrogate
    "\uDFFF"            // Largest low-surrogate
    "\uE000"            // PUA - BMP
    "\uF8FF"            // PUA - BMP
    "\U000F0000"        // PUA - Plane 15
    "\U00100000"        // PUA - Plane 16
    "\U0010FFFF"        // PUA - Maximum possible Unicode code
    "\U00110000"        // Too large for Unicode
    "\u007F"
    "\u0080"
    "\u009F"
    // GCC 6.1.0 does not complain about the following \U UCNs, even
    // though the standard explicitly says they're invalid in
    // identifiers, but these string components are not identifiers!
    "\U0001FFFE"        // Not a character
    "\U0001FFFF"        // Not a character
    "\U0002FFFE"        // Not a character
    "\U0002FFFF"        // Not a character
    "\U0003FFFE"        // Not a character
    "\U0003FFFF"        // Not a character
    "\U0004FFFE"        // Not a character
    "\U0004FFFF"        // Not a character
    "\U0005FFFE"        // Not a character
    "\U0005FFFF"        // Not a character
    "\U0006FFFE"        // Not a character
    "\U0006FFFF"        // Not a character
    "\U0007FFFE"        // Not a character
    "\U0007FFFF"        // Not a character
    "\U0008FFFE"        // Not a character
    "\U0008FFFF"        // Not a character
    "\U0009FFFE"        // Not a character
    "\U0009FFFF"        // Not a character
    "\U000AFFFE"        // Not a character
    "\U000AFFFF"        // Not a character
    "\U000BFFFE"        // Not a character
    "\U000BFFFF"        // Not a character
    "\U000CFFFE"        // Not a character
    "\U000CFFFF"        // Not a character
    "\U000DFFFE"        // Not a character
    "\U000DFFFF"        // Not a character
    "\U000EFFFE"        // Not a character
    "\U000EFFFF"        // Not a character
    ;

/* Valid characters */
extern char valid_ucns[];
char valid_ucns[] =
    // From section 6.4.3 Universal character names
    "\u0024 ($), \u0040 (@), or \u0060 (`)"
    // From Annex D Universal character names for identifiers
    "\u00A8, \u00AA, \u00AD, \u00AF, \u00B2−\u00B5, \u00B7−\u00BA, \u00BC−\u00BE, \u00C0−\u00D6,"
    "\u00D8−\u00F6, \u00F8−\u00FF"
    "\u0100−\u167F, \u1681−\u180D, \u180F−\u1FFF"
    "\u200B−\u200D, \u202A−\u202E, \u203F−\u2040, \u2054, \u2060−\u206F"
    "\u2070−\u218F, \u2460−\u24FF, \u2776−\u2793, \u2C00−\u2DFF, \u2E80−\u2FFF"
    "\u3004−\u3007, \u3021−\u302F, \u3031−\u303F"
    "\u3040−\uD7FF"
    "\uF900−\uFD3D, \uFD40−\uFDCF, \uFDF0−\uFE44, \uFE47−\uFFFD"

    "\U00010000−\U0001FFFD, \U00020000−\U0002FFFD, "
    "\U00030000−\U0003FFFD, \U00040000−\U0004FFFD, "
    "\U00050000−\U0005FFFD, \U00060000−\U0006FFFD, "
    "\U00070000−\U0007FFFD, \U00080000−\U0008FFFD, "
    "\U00090000−\U0009FFFD, \U000A0000−\U000AFFFD, "
    "\U000B0000−\U000BFFFD, \U000C0000−\U000CFFFD, "
    "\U000D0000−\U000DFFFD, \U000E0000−\U000EFFFD"
    ;
