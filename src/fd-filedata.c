/* Copyright (c) 2013-2018, David Anderson
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

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>
#include "reading.h"
#include "readelfobj.h"
#include "sanitized.h"

static char buffer1[BUFFERSIZE];

int
get_filedata(const char *name, int fd,struct filedata_s *fida)
{
    int res = 0;
    LONGESTSTYPE ssize = 0;
    LONGESTUTYPE usize = 0;

    res = fseek(fd,0L,SEEK_END);
    if(res) {
        int myerr = errno;
        printf("FAIL fseek end %s with errno %d %s\n",
            sanitized(name,buffer1,BUFFERSIZE),
            myerr,strerror(myerr));
        printf("Giving up");
        exit(1);
    }
    ssize = ftell(fd);
    if (ssize < 0) {
        int myerr = errno;
        printf("FAIL ftell %s with errno %d %s\n",
            sanitized(name,buffer1,BUFFERSIZE),
            myerr,strerror(myerr));
        printf("Giving up");
        exit(1);
    }
    usize = ssize;
    res = fseek(fd,0L,SEEK_SET);
    if(res) {
        int myerr = errno;
        printf("FAIL fseek 0  %s with errno %d %s\n",
            sanitized(name,buffer1,BUFFERSIZE),
            myerr,strerror(myerr));
        printf("Giving up");
        exit(1);
    }
    fida->f_filesize = usize;
    return RO_OK;
}
