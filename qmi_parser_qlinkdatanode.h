#ifndef QMI_PARSER_qlinkdatanode_H_INCLUDED
#define QMI_PARSER_qlinkdatanode_H_INCLUDED

#include "common_qlinkdatanode.h"

typedef struct _APDU_Setting_Tag{
  bool is2GCard;
  unsigned char proc_step;
  unsigned char head_byte;
  unsigned char data[5];
}APDU_Setting;

int startQMIParserThread(void);

#endif // QMI_PARSER_qlinkdatanode_H_INCLUDED