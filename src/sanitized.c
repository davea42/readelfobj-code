/*
Copyright 2016-2018 David Anderson. All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "sanitized.h"

#define TRUE 1
#define FALSE 0

/*  This does a uri-style conversion of control characters.
    So  SOH prints as %01 for example.
    Which stops corrupted or crafted strings from
    doing things to the terminal the string is routed to.

    We do not translate an input % to %% (as in real uri)
    as that would be a bit confusing for most readers.

    The conversion makes it possble to print UTF-8 strings
    reproducibly, sort of (not showing the
    real glyph!).

    Only call this in a printf or sprintf, and
    only call it once in any single printf/sprintf.
    Othewise you will get bogus results and confusion. */

/* ASCII control codes:
We leave newline as is, NUL is end of string,
the others are translated.
NUL Null             0  00              Ctrl-@ ^@
SOH Start of heading 1  01      Alt-1   Ctrl-A ^A
STX Start of text    2  02      Alt-2   Ctrl-B ^B
ETX End of text      3  03      Alt-3   Ctrl-C ^C
EOT End of transmission 4 04    Alt-4   Ctrl-D ^D
ENQ Enquiry          5    05    Alt-5   Ctrl-E ^E
ACK Acknowledge      6    06    Alt-6   Ctrl-F ^F
BEL Bell             7    07    Alt-7   Ctrl-G ^G
BS  Backspace        8    08    Alt-8   Ctrl-H ^H
HT  Horizontal tab   9    09    Alt-9   Ctrl-I ^I
LF  Line feed       10    0A    Alt-10  Ctrl-J ^J
VT  Vertical tab    11    0B    Alt-11  Ctrl-K ^K
FF  Form feed       12    0C    Alt-12  Ctrl-L ^L
CR  Carriage return 13    0D    Alt-13  Ctrl-M ^M
SO  Shift out       14    0E    Alt-14  Ctrl-N ^N
SI  Shift in        15    0F    Alt-15  Ctrl-O ^O
DLE Data line escape 16   10    Alt-16  Ctrl-P ^P
DC1 Device control 1 17   11    Alt-17  Ctrl-Q ^Q
DC2 Device control 2 18   12    Alt-18  Ctrl-R ^R
DC3 Device control 3 19   13    Alt-19  Ctrl-S ^S
DC4 Device control 4 20   14    Alt-20  Ctrl-T ^T
NAK Negative acknowledge 21 15  Alt-21  Ctrl-U ^U
SYN Synchronous idle 22   16    Alt-22  Ctrl-V ^V
ETB End transmission block 23 17 Alt-23 Ctrl-W ^W
CAN Cancel              24 18   Alt-24  Ctrl-X ^X
EM  End of medium       25 19   Alt-25  Ctrl-Y ^Y
SU  Substitute          26 1A   Alt-26  Ctrl-Z ^Z
ES  Escape              27 1B   Alt-27  Ctrl-[ ^[
FS  File separator      28 1C   Alt-28  Ctrl-\ ^\
GS  Group separator     29 1D   Alt-29  Ctrl-] ^]
RS  Record separator    30 1E   Alt-30  Ctrl-^ ^^
US  Unit separator      31 1F   Alt-31  Ctrl-_ ^_

In addition,  characters decimal 141, 157, 127,128, 129
143,144,157
appear to be questionable too.
Not in iso-8859-1 nor in html character entities list.

We translate all strings with a % to do sanitizing and
we change a literal ASCII '%' char to %27 so readers
know any % is a sanitized char. We could double up
a % into %% on output, but switching to %27 is simpler
and for readers and prevents ambiguity.

Since we do not handle utf-8 properly nor detect it
we turn all non-ASCII to %xx below.
*/

/*  do_sanity_insert() and no_questionable_chars()
    absolutely must have the same idea of
    questionable characters.  Be Careful.

    The user-supplied buffer must be at least 30 bytes
    longer than expected strings. A
    passed-in buffer under 100
    bytes is basically not going to work.
*/
static void
do_sanity_insert( const char *s,char *buffer,unsigned len)
{
    const char *cp = s;
    unsigned charcount = 0;
    char smallbuf[8];

    for ( ; *cp; cp++) {
        unsigned c = *cp & 0xff ;

        if (charcount  > (len - 30)) {
            static char *m = "...Truncated...";
            /*  Oops. Out of room, or so we fear.
                Being conservative. */
            if ((len - charcount) < strlen(m)) {
                /* Really too short! */
                buffer[charcount] = 0;
                return;
            }
            strcat(buffer+charcount,"...Truncated...");
            buffer[charcount] = 0;
            return;
        }
        if (c == '%') {
            /* %xx for this. Simple and unambiguous */
            sprintf(smallbuf,"%%%02x",c & 0xff);
            strcpy(buffer+charcount,smallbuf);
            charcount += 3;
            continue;
        }
        if (c >= 0x20 && c <=0x7e) {
            /* Usual case, ASCII printable characters. */
            buffer[charcount] = c;
            ++charcount;
            continue;
        }
#ifdef _WIN32
        if (c == 0x0D) {
            buffer[charcount] = c;
            ++charcount;
            continue;
        }
#endif /* _WIN32 */
        if (c < 0x20) {
            sprintf(smallbuf,"%%%02x",c & 0xff);
            strcpy(buffer+charcount,smallbuf);
            charcount += 3;
            continue;
        }
        /* ASSERT: c >= 0x7f */
        /* ISO-8859 or UTF-8. Not handled well yet. */
        sprintf(smallbuf,"%%%02x",c & 0xff);
        strcpy(buffer+charcount,smallbuf);
        charcount += 3;
        continue;
    }
    buffer[charcount] = 0;
    return;
}

/*  This routine improves overall dwarfdump
    run times a lot by separating strings
    that might print badly from strings that
    will print fine.
    In one large test case it reduces run time
    from 140 seconds to 13 seconds. */
static int
no_questionable_chars(const char *s) {
    const char *cp = s;

    for ( ; *cp; cp++) {
        unsigned c = *cp & 0xff ;
        if (c == '%') {
            /* Always sanitize a % ASCII char. */
            return FALSE;
        }
        if (c >= 0x20 && c <=0x7e) {
            /* Usual case, ASCII printable characters */
            continue;
        }
#ifdef _WIN32
        if (c == 0x0D) {
            continue;
        }
#endif /* _WIN32 */
        if (c == 0x0A || c == 0x09 ) {
            continue;
        }
        if (c < 0x20) {
            return FALSE;
        }
        if (c >= 0x7f) {
            /*  This notices iso-8859 and UTF-8
                data as we don't deal with them
                properly in dwarfdump. */
            return FALSE;
        }
    }
    return TRUE;
}

const char *
sanitized(const char *s,char *outbuf, unsigned outbuf_len)
{
    if (!s) {
        do_sanity_insert("<no-name!>",outbuf,outbuf_len);
        return outbuf;
    }
    if (no_questionable_chars(s)) {
        /*  The original string is safe as is. */
        return s;
    }
    do_sanity_insert(s,outbuf,outbuf_len);
    return outbuf;
}
