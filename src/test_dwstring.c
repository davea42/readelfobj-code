/*
Copyright (c) 2019-2019, David Anderson
All rights reserved.

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

#include <stdio.h>  /* for printf */
#include <stdlib.h>
#include <string.h>
#include "dwstring.h"
#ifndef TRUE
#define TRUE 1
#endif /* TRUE */
#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

static int errcount;

static void
check_string(const char *msg,char *exp,
    char *actual,int line)
{
    if(!strcmp(exp,actual)) {
        return;
    }
    printf("FAIL %s expected \"%s\" got \"%s\" test line %d\n",
        msg,exp,actual,line);
}
static void
check_value(const char *msg,unsigned long exp,
    unsigned long actual,int line)
{
    if(exp == actual) {
        return;
    }
    printf("FAIL %s expected %lu got %lu test line %d\n",
        msg,exp,actual,line);
    ++errcount;
}

static int
test1(int tnum)
{
    struct dwstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *bigstr = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                   "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
                   "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
                   "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
                   "ccccccbbbbbyyyybbbbbbbbbbbbccc";

    dwstring_constructor(&g);
    
    d = dwstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);

    res = dwstring_append(&g,"abc");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected abc ",(char *)"abc",d,__LINE__);

    res = dwstring_append(&g,"xy");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected abcxy ",(char *)"abcxy",d,__LINE__);

    dwstring_destructor(&g);

    dwstring_constructor(&g);
    res = dwstring_append(&g,bigstr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected bigstring ",bigstr,d,__LINE__);
    biglen = dwstring_strlen(&g);
    check_value("expected 120  ",strlen(bigstr),biglen,__LINE__);

    dwstring_append_length(&g,"xxyyzz",3);

    biglen = dwstring_strlen(&g);
    check_value("expected 123  ",strlen(bigstr)+3,biglen,__LINE__);
    dwstring_destructor(&g);
    
}

static int
test2(int tnum)
{
    struct dwstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *bigstr = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                   "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                   "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
                   "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
                   "ccccccbbbbbbbbbbbbbbbbbbbbbccc"
                   "ccccccbbbbbyyyybbbbbbbbbbbbccc";

    dwstring_constructor_fixed(&g,10);

    d = dwstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);

    res = dwstring_append(&g,"abc");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected abc ",(char *)"abc",d,__LINE__);

    res = dwstring_append(&g,"xy");
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected abcxy ",(char *)"abcxy",d,__LINE__);

    dwstring_destructor(&g);

    dwstring_constructor_fixed(&g,3);
    res = dwstring_append(&g,bigstr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected bigstring ",bigstr,d,__LINE__);
    biglen = dwstring_strlen(&g);
    check_value("expected 120  ",strlen(bigstr),biglen,__LINE__);
    dwstring_destructor(&g);

}

static int
test3(int tnum)
{
    struct dwstring_s g;
    char *d = 0;
    const char *expstr = "";
    int res = 0;
    unsigned long biglen = 0;
    char *bigstr = "a012345";
    char *targetbigstr = "a012345xy";

    dwstring_constructor_fixed(&g,10);

    d = dwstring_string(&g);
    check_string("expected empty string",(char *)expstr,d,__LINE__);

    res = dwstring_append(&g,bigstr);
    check_value("expected TRUE  ",TRUE,res,__LINE__);
    d = dwstring_string(&g);
    check_string("expected a012345 ",(char *)bigstr,d,__LINE__);

    res = dwstring_append_length(&g,"xyzzz",2);
    check_value("expected TRUE  ",TRUE,res,__LINE__);

    check_value("expected 9  ", 9,(unsigned)dwstring_strlen(&g),
        __LINE__);

    d = dwstring_string(&g);
    check_string("expected a012345xy ",
        (char *)targetbigstr,d,__LINE__);
}


int main()
{
    test1(1);
    test2(2);
    test3(3);
    if (errcount) {
        exit(1);
    }
    exit(0);
}
