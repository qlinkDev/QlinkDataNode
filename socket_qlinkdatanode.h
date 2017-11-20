#ifndef SOCKET_qlinkdatanode_H_INCLUDED
#define SOCKET_qlinkdatanode_H_INCLUDED

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h> // inet_pton etc
#include <sys/un.h>

#include "event_qlinkdatanode.h"
#include "protocol_qlinkdatanode.h"
#include "qmi_parser_qlinkdatanode.h" // APDU_setting

#define SOCK_READ_BYTES (1024)
#define MAX_URC_ARRAY_LEN (20)
#define MAX_URC_LEN (SOCK_READ_BYTES)
#define VALID_URC (0x01)
#define INVALID_URC (0x00)
#define AT_CEREG_EXT_RSP_COMMA_NUM (4)
#define AT_CGREG_EXT_RSP_COMMA_NUM (3)

#define FLU_COUNT_TIME_3MIN (2)
#define FLU_COUNT_VALUE ((FLU_COUNT_TIME_3MIN * 3))
#define STATUS_REPORT_COUNT_TIME_60MIN (20) // 35

#define FLU_COUNT_TIME_PINGTEST_5MIN (3)

#define SOCK_READ_BYTES (1024)

#define UDP_READ_BYTES (1024)

#define UDP_LINK_COUNT (10)




//Coefficient of RTT
#define ALPHA_qlinkdatanode (0.125)
#define BETA_qlinkdatanode (0.25)
#define K_qlinkdatanode (4)
//Second convert to microsecond
#define S_TO_US (1000000)

#define MAX_RTO_VAL ((2<<(sizeof(unsigned int)-1))-1)

#define MAX_SPEC_URC_ARR_LEN (10)

typedef enum {
  REG_URC_STATE_IDLE, /*0*/
  REG_URC_STATE_CARD_ERROR, /*1*/
  REG_URC_STATE_CARD_READY, /*2*/
  REG_URC_STATE_REGISTERED, /*3*/
  REG_URC_STATE_REGISTRATION_REJECTED, /*4*/ //Permanent rejection
  REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15, /*5*/
  REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111, /*6*/ //Add by rjf at 20150917
  REG_URC_STATE_REGISTRATION_FAIL, /*7*/
  REG_URC_STATE_NOT_REGISTERED, /*8*/ //Add by rjf at 20150921  //Handle "+CXREG: X"  X=2,3,4
  REG_URC_STATE_MAX /**/
}Reg_URC_State_E_Type;

typedef enum {
  REG_SYS_MODE_MIN, /*0*/
  REG_SYS_MODE_WCDMA, /*1*/
  REG_SYS_MODE_LTE, /*2*/
  REG_SYS_MODE_UNKNOWN, /*3*/
  REG_SYS_MODE_MAX, /*4*/
}Reg_Sys_Mode_State_E_Type;

typedef enum {
  REG_URC_UNKNOWN,          /*0*/
  REG_URC_CARD_ERROR,       /*1*/
  REG_URC_CARD_READY,       /*2*/ //+CPIN: READY
  REG_URC_REG,              /*3*/ //+CGREG: 1,...
  REG_URC_LTE_REG,          /*4*/ //+CEREG: 1,...
  REG_URC_REG_FAIL,         /*5*/ //+CXREG: 0
  REG_URC_REG_REJ_V01,      /*6*/ //+REGREJ: X,Y  Y=2,3,6,7,8
  REG_URC_REG_REJ_V02,      /*7*/ //+REGREJ: X,Y  Y=15
  REG_URC_REG_REJ_V03,      /*8*/ //+CXREG: 3
  REG_URC_REG_REJ_V04,      /*9*/ //+REGREJ: X,Y  Y=111 // Add by rjf at 20150917
  REG_URC_REG_SEARCHING,    /*10*/ //+CXREG: 2
  REG_URC_REG_UNKNOWN,      /*11*/ //+CXREG: 4
  REG_URC_REG_ROAMING,      /*12*/ //+CXREG: 5
  REG_URC_SYS_MODE_WCDMA,   /*13*/ //+CNSMOD: X  X=4,5,6,7(Usually 4 and 7)
  REG_URC_SYS_MODE_LTE,     /*14*/ //+CNSMOD: 8
  REG_URC_SYS_MODE_UNKNOWN, /*15*/ //+CNSMOD: X  X!=4,5,6,7,8
  REG_URC_RAB_REESTAB_REJ,
  REG_URC_RAB_REESTAB_FAIL,
  REG_URC_MAX               /*18*/
}Reg_URC_Id_E_Type;

typedef enum {
  SPEC_URC_ARR_IDX_VIRT_USIM_PWR_DWN,
  SPEC_URC_ARR_IDX_VIRT_USIM_SESSION_ILLEGAL,
  SPEC_URC_ARR_IDX_MAX
}Special_Urc_Array_Idx_E_Type;

typedef struct _Udp_Config_Tag{
		int sock_des;
		Evt sock_evt;
		struct sockaddr_in addr;

		Evt timer_evt;
		//Unit: microsecond; Max val: about 4000 seconds.
		unsigned int rto;
		unsigned int srtt; // srtt of last time
		unsigned int varrtt; // varrtt of last time
		unsigned int new_rtt;
		//step = 0: No RTT measurement has been made.
		//step = 1: Have got first RTT.
		//step = 2: Have got RTT more than twice.
		unsigned char step;
		//Retransmission timer is timed out, but no reply received.
		bool isTimeout;
}Udp_Config;


typedef enum {
  FILE_TYPE_FLUDATA = 0,
  FILE_TYPE_PRE_FLUDATA = 1,
  FILE_TYPE_SIMCARDFLUDATA = 2,
  FILE_TYPE_SWITCHSIMCARDFLUDATA = 3,
  FILE_TYPE_FLUDATA_MAX
}WriteFile_Type;

typedef struct _strflu_record_Tag{
  long long flu_value;
  unsigned int year_record;
  unsigned char month_record;
  unsigned char day_record;
  unsigned char hour_record;
  unsigned char min_record;
  unsigned char sec_record;
}strflu_record;

typedef struct strusedTimeByUser{
  
  		unsigned int year_record;
 		unsigned int month_record;
  		unsigned int day_record;
  		unsigned int hour_record;
  		unsigned int min_record;
  		unsigned int sec_record;
		
}strusedTimeByUser;

typedef struct _strflu_record2_Tag{
  unsigned long long flu_value;
  unsigned int year_record;
  unsigned char month_record;
  unsigned char day_record;
  unsigned char hour_record;
  unsigned char min_record;
  unsigned char sec_record;
}strflu_record2;

typedef enum {
  FILE_TYPE_NULL = 0,
  FILE_TYPE_YY_UPDATE = 1,
  FILE_TYPE_qlinkdatanode = 2,
  FILE_TYPE_YY_DAEMON = 3,
  FILE_TYPE_YY_UPDATE_qlinkdatanode = 4,
  FILE_TYPE_YY_UPDATE_YY_DAEMON = 5,
  FILE_TYPE_qlinkdatanode_YY_DAEMON = 6,
  FILE_TYPE_UPLOAD_MAX
}UploadFile_Type;

typedef	 struct data_msg_type_s
        {
        
	  unsigned char data_msghead;
	  unsigned char data_msglen[4];
	  unsigned char data_regtype;
	  unsigned char data_ueidtag;
	  unsigned char data_ueid[4];
	  unsigned char data_statustag;
	  unsigned char data_statusvalue;
	  unsigned char data_xor;
	

      }data_msg_type;


#endif // SOCKET_qlinkdatanode_H_INCLUDED
