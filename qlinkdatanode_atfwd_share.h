#ifndef __qlinkdatanode_ATFWD_SHARE__
#define __qlinkdatanode_ATFWD_SHARE__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define ATCMD5360_STRING_MAX10 10  
#define ATCMD5360_STRING_MAX20 20  
#define ATCMD5360_STRING_MAX50 50  
#define ATCMD5360_STRING_MAX70 70
#define ATCMD5360_STRING_MAX100 100  
#define ATCMD5360_STRING_MAX250 250  
#define ATCMD5360_FILE "/usr/bin/atcmd5360_file"

struct ATCMD5360_TYPE
{
  int isupdate;
  char cpin5360[ATCMD5360_STRING_MAX50];
  char simei5360[ATCMD5360_STRING_MAX100];
  char simcomati5360[ATCMD5360_STRING_MAX250];
  char csub5360[ATCMD5360_STRING_MAX100];
  char cnvr5360[ATCMD5360_STRING_MAX70];
  char ciccid5360[ATCMD5360_STRING_MAX50];
  char cgauth5360write[512];
  char cgdcont5360write[ATCMD5360_STRING_MAX250];
  char cgauth5360read[512];
  char cgdcont5360read[ATCMD5360_STRING_MAX250];
};

void write_atcmd5360_file(void);
int read_atcmd5360_file(struct ATCMD5360_TYPE *pstAtcmd5360 );

#endif
