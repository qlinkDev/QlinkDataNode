#ifndef AT_RSP_qlinkdatanode_H_INCLUDED
#define AT_RSP_qlinkdatanode_H_INCLUDED

#include "common_qlinkdatanode.h"

extern int process_CSQ(Dev_Info *di, char *ptr);
extern int process_CGREG(char *ptr);
extern int process_NETOPEN(char *str);
extern int process_CIPOPEN(char *str);
extern int process_CIPSEND(char *str);
extern int process_CGSN(char *str);
extern int process_COPS(Dev_Info *di, char *str);
extern int process_CGREG_lac(Dev_Info *di, char *str);
extern int process_CIPCLOSE(char *rsp);
extern int process_NETCLOSE(char *rsp);

int process_CGREG_lac_and_cellid(Dev_Info *di, char *str);
int process_Imodem_CEREG_lac_and_cellid(Dev_Info *di, char *str);
int process_Imodem_CGREG_lac_and_cellid(Dev_Info *di, char *str);
#endif //AT_RSP_qlinkdatanode_H_INCLUDED