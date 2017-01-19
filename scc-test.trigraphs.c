/*
@(#)File:           $RCSfile: scc-test.trigraphs.c,v $
@(#)Version:        $Revision: 1.3 $
@(#)Last changed:   $Date: 2015/01/10 19:20:51 $
@(#)Purpose:        Test SCC on trigraphs
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 2014-15
@(#)Product:        SCC Version 6.16 (2016-01-19)
*/

??=include <stdio.h>

/??/
* -- Constant Definitions *??/
/

??=ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
extern const char jlss_id_scc_trigraphs_c[];
const char jlss_id_scc_trigraphs_c[] = "@(#)$Id: scc-test.trigraphs.c,v 1.3 2015/01/10 19:20:51 jleffler Exp $";
??=endif /* lint */

/*
**  Digraphs
**  <:  :>  <%  %>  %:  %:%:
**  [   ]   {   }   #   ##
**
**  Digraphs are boring in that they don't affect the interpretation of
**  comments, strings, characters, etc.  The only twisted part is that
**  digraphs are punctuators, so they're not relevant or interpreted
**  inside string literals (or multicharacter constants), unlike
**  trigraphs which are interpreted everywhere (and at a much earlier
**  phase of compilation).  That means a digraph inserter or replacer
**  must be aware of character and string literal rules (and comments).
*/

typedef unsigned char Uchar;

typedef struct Trigraphs
??<
    Uchar plain;
    Uchar reverse;
    char trigraph??(4??);
??> Trigraphs;

static Trigraphs trigraphs??(??) =
??<
    {   '#',    '=',    "\?\?=" },
    {   '[',    '(',    "\?\?(" },
    {   '\\',   '/',    "\?\?/" },
    {   ']',    ')',    "\?\?)" },
    {   '^',    '\'',   "\?\?'" },
    {   '{',    '<',    "\?\?<" },
    {   '|',    '!',    "\?\?!" },
    {   '}',    '>',    "\?\?>" },
    {   '~',    '-',    "\?\?-" },
??>;

static void twister(int i, int j)
{
    int x = i ??! j;
    int y = i ??' ??-j;
    printf("i = %2d; j = %2d; i ??! j = %2d; i ??' ??-j = %3d??/n", i, j, x, y);
}

int main(void)
{
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
            twister(i, j);
    }
    for (int i = 0; i < sizeof(trigraphs)/sizeof(trigraphs[0]); i++)
    {
        // Nasty code, but nasty must be tested too!
        printf("%c -> ??/
\?\?%c (%s)\n", trigraphs[i].plain,
               trigraphs[i].reverse, trigraphs[i].trigraph);
    }
    return 0;
}
