#ifndef FEATURE_MACRO_qlinkdatanode_H_INCLUDED
#define FEATURE_MACRO_qlinkdatanode_H_INCLUDED
  
/******************************Running mode related******************************/	
//#define FEATURE_ENABLE_APP_DAEMON

/******************************Log related******************************/

#define FEATURE_ENABLE_DEBUG_MODE
#ifdef FEATURE_ENABLE_DEBUG_MODE
#define FEATURE_DEBUG_LOG_FILE

#ifdef FEATURE_DEBUG_LOG_FILE
#endif

//#define FEATURE_DEBUG_SYSLOG
//#define FEATURE_DEBUG_SHELL_OUTPUT
//#define FEATURE_DEBUG_QXDM
#ifdef FEATURE_DEBUG_QXDM
#define FEATURE_ENABLE_FMT_PRINT_DATA_LOG
#endif
#endif /* FEATURE_ENABLE_DEBUG_MODE */

#define FEATURE_ENABLE_TIMESTAMP_LOG

/*Logs of each source file*/
#define FEATURE_ENABLE_AT_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_AT_RSP_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_QMI_RECVER_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_QMI_SENDER_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_QMI_PARSER_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_COMMON_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_QUEUE_qlinkdatanode_C_LOG
#define FEATURE_ENABLE_MAIN_C_LOG


#ifdef FEATURE_ENABLE_DEBUG_MODE
//Function:
//Print recv buf of at rsp by ASCII.
//#define FEATURE_ENABLE_RECV_BUF_LOG
//#define FEATURE_ENABLE_READER_RECV_START_RECVED_DATA_LOG
//#define FEATURE_ENABLE_PROCESS_RECV_PRINT_RECVED_CHARS_LOG
#define FEATURE_ENABLE_PROCESS_RECV_PRINT_RECVED_APDU_LOG
#define FEATURE_ENABLE_PROCESS_RECV_RECVED_PARTIAL_URC_HEADER_CHARS_LOG
#define FEATURE_ENABLE_SOCK_RECV_HDLR_RECVED_DATA_LOG
//#define FEATURE_ENABLE_RECVCALLBACK_RECVED_DATA_LOG
#define FEATURE_ENABLE_SENDER_DISPATCHED_APDU_LOG
//#define FEATURE_ENABLE_SENDER_DISPATCHED_REMOTE_UIM_DATA_LOG
#define FEATURE_ENABLE_W_ACQDATAHDLR_DISPATCHED_DATA_LOG
#define FEATURE_ENABLE_SOCK_SEND_HDLR_DISPATCHED_DATA_LOG
//#define FEATURE_ENABLE_PARSE_QMI_IND_UNDECODED_CONTENT_LOG
#define FEATURE_ENABLE_PARSE_QMI_IND_DECODED_CONTENT_LOG
//#define FEATURE_ENABLE_PARSE_QMI_NAS_IND_UNDECODED_CONTENT_LOG
#define FEATURE_ENABLE_PARSE_QMI_NAS_IND_DECODED_CONTENT_LOG
#define FEATURE_ENABLE_PRINT_TIMER_LIST
//#define FEATURE_ENABLE_PRINT_TID

#define FEATURE_ENABLE_PRINT_ACQ_IMSI
#define FEATURE_ENABLE_PRINT_URC_ARRAY
#define FEATURE_ENABLE_SERVRSP_TIMEOUT_CB_APDU_DATA_LOG
#define FEATURE_ENABLE_APDURSP_TIMEOUT_EXT_CB_APDU_DATA_LOG
#define FEATURE_ENABLE_PRINT_MFG_INF_BUF
#define FEATURE_ENABLE_PRINT_PDP_CTX_INF_BUF
#define FEATURE_ENABLE_PRINT_CUSTOMED_AT_CMD_RSP

//Description:
//If app utilize QXDM as the output channel of its log info, we will miss the beginning part of log because
//QXDM starts rather slowly. If this macro is defined, app will generate a file to contain the startup log.
//#define FEATURE_ENABLE_PROCESS_STARTUP_LOG // Add by rjf at 20150731

#define FEATURE_ENABLE_CHANNEL_LOCK_LOG
#define FEATURE_ENABLE_ACQ_DATA_STATE_MTX_LOG
#define FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE_AT_ONCE

//Description:
//Print remote USIM data content when recved remote USIM data seems abnormal. (e.x. Recved data len is too small.)
#define FEATURE_ENABLE_ABNORMAL_REMOTE_USIM_DATA_PRINT_LOG

#endif // FEATURE_ENABLE_DEBUG_MODE

/******************************Timers related******************************/
#define FEATURE_ENABLE_TIMER_SYSTEM

  //Function:
  //The timer is set after acq_data_state changed to ACQ_DATA_STATE_WAITING,
  //and it'll call timeout hdlr when no AT cmd is applied and no rsp is recved.
#define FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER

#define FEATURE_ENABLE_APDU_RSP_TIMEOUT_SYS
#define FEATURE_ENABLE_SERVER_RSP_TIMER

#ifdef FEATURE_ENABLE_SERVER_RSP_TIMER
#define FEATURE_ENABLE_ID_AUTH_RSP_TIMER
#define FEATURE_ENABLE_REMOTE_USIM_DATA_RSP_TIMER
#define FEATURE_ENABLE_POWER_DOWN_NOTIFICATION_TIMER
#endif //FEATURE_ENABLE_SERVER_RSP_TIMER

#define FEATURE_ENABLE_ATE0_RSP_TIMER
#define FEATURE_ENABLE_PDP_ACT_CHECK_TIMER

#define FEATURE_ENABLE_WAKEUP_EVENT


/******************************Virtual USIM related******************************/	

//Description:
//Before receiving remote USIM data, use "AT+CATR" to ctrl URCs to specified destination.
#define FEATURE_ENABLE_CATR

//Description:
//Enable the function of deleting local USIM data.
#define FEATURE_ENABLE_DEL_LOCAL_USIM_DATA

/******************************Repeated AT cmd exec related******************************/	
#define FEATURE_KEEP_CHECKING_CGREG_UNTIL_REG_SUCCEEDED
#define FEATURE_KEEP_CHECKING_CSQ_UNTIL_RSSI_ISNT_0
#define FEATURE_KEEP_EXECUTING_CIPOPEN_UNTIL_SUCCESS

//xiaobin add
#define FEATURE_SET_3G_ONLY_ON_REJ_15
//#define FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE

/******************************Private protocol related******************************/
#define FEATURE_ENABLE_PRIVATE_PROTOCOL

#define FEATURE_USE_MDMEmodem_IMEI

//Enable the field of time zone in ID check rsp and send this info to other process by online and
//offline (of mdmEmodem) ind.
#define FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK

//If EvtLoop has 2 request of uploading status information, app will dump the older request and upload
//the new one.
#define FEATURE_ENHANCED_STATUS_INFO_UPLOAD

#define FEATURE_ENABLE_REQ_RETRAN_SYS_FOR_ID_AUTH_REJ

#define FEATURE_NEW_APDU_COMM //xiaobin add


/******************************Process msg related (To yy_daemon)******************************/
#define FEATURE_ENABLE_MSGQ

#define FEATURE_ENABLE_qlinkdatanode_RUN_STAT_IND
#ifdef FEATURE_ENABLE_qlinkdatanode_RUN_STAT_IND
#define FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
#endif


#define FEATURE_CONFIG_SIM5360

//Description:
//If this macro is defined, app is able to recv customed AT cmds sent from mdm9215, and send them to mdm9215.
#define FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD


/******************************MdmEmodem ctrl related******************************/
//Function:
//If the macro defined, app will launch mdmEmodem itself.
#define FEATURE_ENABLE_MDMEmodem_SOFT_STARTUP
//Function:
//If the macro defined, app will shut down mdmEmodem itself.
#define FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
//Description:
//When this macro is defined, no actual standby mode is supported.
#define FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
//Description: 
//This is for waking up mdmEmodem properly and completely.
#define FEATURE_MDMEmodem_MULTIPLE_SIGNAL_QUERY_AFTER_STANDBY_MODE_SWITCH

#ifdef FEATURE_ENABLE_MSGQ
#define FEATURE_MDMEmodem_RST_FOR_SIMCARD_NOT_AVAIL /*It depends on msgq*/
#endif //FEATURE_ENABLE_MSGQ

/******************************APDU related******************************/
//Function:
//Just use network link of mdmEmodem as transmission channel of APDUs.
#define FEATURE_SINGLE_CHANNEL_OF_APDU_TRANSMISSION

//Description:
//Once APDU data is sent out and APDU rsp ext timer is not expired, app will clear current APDU rsp ext timer,
//and set a new APDU rsp ext timer whose expired time is 10s.
#define FEATURE_ENABLE_RST_APDU_RSP_EXT_TIMER_WHEN_APDU_DATA_SENT_OUT

//Description:
//If this macro is defined, QMI usim power down is exec async.
#define FEATURE_QMI_ASYNC_REQ_UIM_PD

/******************************MdmEmodem AT cmd rsp error correction related******************************/	
#define CORRECTION_NO_URC_FOR_NETCLOSE
#define CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR

/******************************PDP act related******************************/
//Function:
//Enable the notification system of dialup status variation by other process.
#define FEATURE_ENABLE_RECV_PDP_ACT_RPT

/******************************Atfwd related******************************/
//Description:
//Enable AT cmd for version qry.
#define FEATURE_VER_QRY

//Description:
//Enable AT cmd for bootup status of mdmEmodem.
#define FEATURE_MDMEmodem_BOOT_STAT_QRY

//Description:
//If this macro defined, the URC report "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" will be handled.
//Undef Reason:
// 1. The existence of idle timer of mdmEmodem avoid that the dialup is kicked off.
// 2. The handling process of fell reg is processed by handling inds of mdm9215.
#define FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE

//#define FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED

//Function:
//When msg to serv is recved and mdmEmodem serv conn is unavailable, app pushes req into queue and waiting  
//serv conn recovery.
#define FEATURE_ENABLE_QUEUE_SYSTEM

#define FEATURE_ENABLE_SYSTEM_RESTORATION

//Description:
//Not deactivate PDP when app trys to suspend mdmEmodem.
#define FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION

#define FEATURE_ENABLE_STATUS_REPORT_10_MIN
//#define FEATURE_ENABLE_TCP_SOCK_TEST
#define FEATURE_ENABLE_SEND_MSGTO_UPDATE
#define FEATURE_ENABLE_FLU_LIMIT
#define FEATURE_ENABLE_LIMITFLU_FROM_SERVER
#define FEATURE_ENABLE_FLU_LIMIT_SPEED_256K	"2mbit"
#define FEATURE_ENABLE_FLU_LIMIT_SPEED_128K	"1mbit"

#define FEATURE_ENABLE_APNCONFIG_FROM_SERVER

#define FEATURE_ENABLE_CPOL_CONFIG
#define FEATURE_ENABLE_CPOL_CONFIG_DEBUG
#define FEATURE_ENABLE_CPOL_CONFIG_FOR_HK

#define FEATURE_ENABLE_REPORT_FOR_CPINREDAY
//#define FEATURE_ENABLE_EXCUTE_SERVERCOMMAND_DEBUG

#define FEATURE_ENABLE_UPLOAD_LOG_DEBUG

#define FEATURE_ENABLE_REBOOT_DEBUG

#define FEATURE_ENABLE_3GONLY

#define FEATURE_ENABLE_Emodem_LCDSHOW

#define FEATURE_ENABLE_UPDATE_STATUS

#define FEATURE_ENABLE_Emodem_BUSY_ISSUE

#define FEATURE_ENABLE_4G3GAUTO_FOR_US

#define FEATURE_ENABLE_5360IMEI_TO_YYDAEMON
#define FEATURE_ENABLE_APN_MESSAGE_TO_YYDAEMON

//#define FEATURE_ENABLE_CANCEL_LOG_OUTPUT
#define FEATURE_ENABLE_F9_CIPCLOSE

#define FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD

#define REGIST_NET_OK (0x01)
#define REGIST_NET_ERROR (0x02)

#define CPIN_READY_OK (0x03)
#define CPIN_READY_OK_AFTER (0x04)


#define FEATURE_ENABLE_EmodemUDP_LINK

//#define FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

#define FEATURE_ENABLE_Emodem_UDP_PORT  9111

#define FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

#define FEATURE_ENABLE_IDCHECK_7

#define FEATURE_ENABLE_IDCHECK_C7_C8

#define FEATURE_ENABLE_IDCHECK_NEW_TIMEOUT_PROCESS

#define FEATURE_ENABLE_AT_UART_RECEIVE_DATA

#define FEATURE_ENABLE_4G_OK_F0

#define FEATURE_ENABLE_SFOTCARD_DEBUG

#define FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

#define FEATURE_ENABLE_MESSAGE_TO_USER_0113DEBUG

#define FEATURE_ENABLE_CONTRL_EmodemREBOOT_0117DEBUG

#define FEATURE_ENABLE_UPDATE_IP3_DEBUG

#define FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

//#define FEATURE_ENABLE_SYSTEM_FOR_PORT_0223

#define FEATURE_ENABLE_MESSAGE_TO_YY_DAEMON_CANCEL_APPLE0302
#define FEATURE_ENABLE_STSRT_TEST_MODE

#endif // FEATURE_MACRO_qlinkdatanode_H_INCLUDED