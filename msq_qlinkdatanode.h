#ifndef MSQ_H_INCLUDED
#define MSQ_H_INCLUDED

#include "feature_macro_qlinkdatanode.h"

//Function:
//The keys of 2 msg queues:
//MSGQ_RCV is msg queue from which application process recvs msgs. (Powerdown msg)
//MSGQ_SND is msg queue to which application process sends msgs. (Battery power msg)
#define MSGQ_RCV (4282)
#define MSGQ_SND (4182)

#ifdef FEATURE_ENABLE_SEND_MSGTO_UPDATE
#define MSGQ_SND_UPDATE (4383)
#endif

//Unit: sec
//Not wait until being shut down forcibly for timeout(10s) by other process.
#define PD_MSG_TIMEOUT (5)

#define MAX_SIZE_OF_REQ_MSG_DATA (1024)

#define YY_PROCESS_MSG_BATTERY_RSP_V02 (2001)

//Description:
//*_IND : Msg tran from qlinkdatanode to yy_daemon
//*_RPT : Msg tran from yy_daemon to qlinkdatanode
typedef enum
{
  YY_PROCESS_MSG_POWER_DOWN_REQ = 5000,
  YY_PROCESS_MSG_POWER_DOWN_RSP = 5001,
  YY_PROCESS_MSG_BATTERY_REQ = 5002,
  YY_PROCESS_MSG_BATTERY_RSP = 5003,
  YY_PROCESS_MSG_ONLINE_IND = 5004, // Indicate that mdmEmodem is connected to server.
  YY_PROCESS_MSG_OFFLINE_IND = 5005,
  YY_PROCESS_MSG_DIALUP_IND = 5006,
  YY_PROCESS_MSG_MDMEmodem_WAKEUP_REQ = 5007,
  YY_PROCESS_MSG_qlinkdatanode_RUN_STAT_IND = 5008,
  YY_PROCESS_MSG_MDMEmodem_AT_CMD_REQ = 5009,
  YY_PROCESS_MSG_MDMEmodem_AT_CMD_RSP = 5010,
  YY_PROCESS_MSG_MDM9215_IMSI_IND = 5011, // Add by rjf at 20150911
  YY_PROCESS_MSG_MDM9215_USIM_RDY_IND = 5012, // Changed by rjf at 20150922
  YY_PROCESS_MSG_MDM9215_REG_IND = 5013, // Add by rjf at 20150922
  YY_PROCESS_MSG_MDM9215_NW_SYS_MODE_IND = 5014, // Add by rjf at 20151010
  YY_PROCESS_MSG_NTP_RPT = 5015, // Add by rjf gl & rjf at 20151021
  YY_PROCESS_MSG_SYS_RST = 5016, // Add by rjf at 20151031
  YY_PROCESS_MSG_DEV_RST = 5017, // Add by rjf at 20151104
  YY_PROCESS_MSG_SEND_PDP_NOTACTIVE = 5018, // Add by jack at 20160126
  YY_PROCESS_MSG_SEND_FLU_CURRENT = 5019, // Add by jack at 20160126
  YY_PROCESS_MSG_SEND_IMEI_TO_YY_DAEMON = 5020, // Add by  li at 20160707
  YY_PROCESS_MSG_SEND_VERSION_TO_YY_DAEMON = 5021, // Add by  li at 20161110
  YY_PROCESS_MSG_SEND_APN_MESSAGE_TO_YY_DAEMON = 5022, // Add by  li at 20161222
  YY_PROCESS_MSG_SEND_YYDAEMON_OUT_OF_TRAFFIC = 5023, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_OUT_OF_DATE = 5024, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_SHOW_RATE = 5025, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_BOOKORDER = 5026, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_USERMIN = 5027, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_WINDOW= 5028, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_APPLE= 5029, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_ISTEST_MODE= 5030, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_ISLIMIT_PULL_WINDOWN= 5031, // Add by  li at 20170206
  YY_PROCESS_MSG_SEND_YYDAEMON_SOFT_SIM_MODE = 5032, // Add by zhijie at 20170330
  YY_PROCESS_MSG_SEND_IMEI_TO_UPDATE = 6000, // Add by  li at 20160122
  YY_PROCESS_MSG_MAX
}Yy_Process_Cmd_E_Type;

typedef struct {
  int vol;
  int per;
  int mode;
}Raw_Power_Info_S_Type;

#ifdef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
typedef struct{
  long msg_id;
  char tz_v[3]; //timezone value
}Ext_Ind_Msg_S_Type;
#endif // FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK

typedef struct {
  long msg_id;
  char msg_data;
}Common_Proc_Msg_S_Type;

#define MAX_PROC_MSG_AT_CMD_STR_LEN (64)
#define MAX_PROC_MSG_AT_RSP_STR_LEN (256)
#define IMSI_DATA_LEN (9)
#define NTP_DATA_LEN (64)

typedef enum{
  AT_CMD_ID_SIMEI = 0,
  AT_CMD_ID_MAX
}AT_Cmd_Id_E_Type;

typedef struct{
  char data[MAX_PROC_MSG_AT_RSP_STR_LEN];
}AT_Cmd_Cfg_S_Type;

typedef struct{
  char data[IMSI_DATA_LEN]; // 9
}Imsi_Ind_Data_S_Type;

typedef struct{
  char ntp_time_yy[NTP_DATA_LEN]; // 64
}ntp_rev_yy_daemon;

typedef struct{
  long id;
  union{
    int flag; /*For indicating whether it is "Factory Reset" or "Power Down"*/
#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
    AT_Cmd_Cfg_S_Type at_cmd_config;
    int ctrl_word;
#endif //FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
    int mdm9215_startup_inf;
    Raw_Power_Info_S_Type pwr_inf;
    Imsi_Ind_Data_S_Type imsi_data;
    int usim_status;
    int reg_status;
    int nw_sys_mode;
    int ntp_stat;
    int sys_rst_data;
    int dev_rst_data;
    ntp_rev_yy_daemon ntp_time;
  }data;

}Proc_Req_Msg_S_Type;

typedef struct {
  int at_cmd_id;
  char cmd_str[MAX_PROC_MSG_AT_CMD_STR_LEN];
  char rsp_str[MAX_PROC_MSG_AT_RSP_STR_LEN];
}Proc_Msg_At_Cmd_Cfg_S_Type;

typedef enum {
  qlinkdatanode_RUN_STAT_IMEI_REJ = 0,
  qlinkdatanode_RUN_STAT_RM_USIM = 1,
  qlinkdatanode_RUN_STAT_HLD_USIM = 2,
  qlinkdatanode_RUN_STAT_RMT_USIM_UNAVL = 3,
  qlinkdatanode_RUN_STAT_CVR_USIM = 4,
  qlinkdatanode_RUN_STAT_USIM_ERR = 5,
  qlinkdatanode_RUN_STAT_USIM_PD = 6,
  qlinkdatanode_RUN_STAT_USIM_RDY = 7,
  
  qlinkdatanode_RUN_STAT_SIG_WEAK = 8,
  qlinkdatanode_RUN_STAT_SIG_FINE = 9,
  qlinkdatanode_RUN_STAT_NOT_REG = 10,
  qlinkdatanode_RUN_STAT_REG = 11,
  qlinkdatanode_RUN_STAT_PDP_NOT_ACT = 12,
  qlinkdatanode_RUN_STAT_PDP_ACT = 13,
  qlinkdatanode_RUN_STAT_SERV_DISCONN = 14,
  qlinkdatanode_RUN_STAT_SERV_CONN = 15,
  
  qlinkdatanode_RUN_STAT_MDMEmodem_RESTORING = 16,
  qlinkdatanode_RUN_STAT_MDMEmodem_SUSPENDED = 17, 
  qlinkdatanode_RUN_STAT_IMEI_REJ_SERVER = 18, //  add  by jack  li 2016/02/18
  qlinkdatanode_RUN_STAT_DEVICE_NOTFIND = 19, //  add  by jack  li 2016/02/18
  qlinkdatanode_RUN_STAT_DATAIS_ERROR = 20, //  add  by jack  li 2016/02/29
  qlinkdatanode_RUN_STAT_MDMEmodem_POWEROFF = 21, // Start Emodem fail add  by jack  li 2016/03/03
  qlinkdatanode_RUN_STAT_MDMEmodem_POWERON = 22, // Start Emodem fail add  by jack  li 2016/03/03
  qlinkdatanode_RUN_STAT_Emodem_CPIN_ERROR = 23, //  add  by jack  li 2016/03/03
  qlinkdatanode_RUN_STAT_Emodem_CPIN_REDAY = 24, //  add  by jack  li 2016/03/03
  qlinkdatanode_RUN_STAT_9862 = 25, //  add  by jack  li 2016/10/11
  qlinkdatanode_RUN_STAT_AUTR_AGAIN = 26, //  add  by jack  li 2016/10/11
  qlinkdatanode_RUN_STAT_DONTGETDATA = 27, //  add  by jack  li 2016/10/11
  qlinkdatanode_RUN_STAT_GETDATA = 28, //  add  by jack  li 2016/10/11
  qlinkdatanode_RUN_STAT_7100CGREG2 = 29, //  add  by jack  li 2016/10/11
  qlinkdatanode_RUN_STAT_7100CGREG1 = 30, //  add  by jack  li 2016/10/11
  qlinkdatanode_RUN_STAT_MAX
}qlinkdatanode_Run_Stat_E_Type;





#endif // MSQ_H_INCLUDED
