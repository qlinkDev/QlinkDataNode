#ifndef COMMON_qlinkdatanode_H_INCLUDED
#define COMMON_qlinkdatanode_H_INCLUDED

#include <stdio.h>
#include <stdarg.h>

#include "device_management_service_v01.h"

#include "feature_macro_qlinkdatanode.h"

#define OFFICIAL_VER "V29.22"
#define MAX_LOG_BUF_LEN (1024)
#define DS_MAX_DIAG_LOG_MSG_SIZE (512) // For QXDM only

extern void print_to_null(const char * fmt,...);

#ifndef FEATURE_ENABLE_DEBUG_MODE
#define LOG print_to_null
#else

#ifdef FEATURE_DEBUG_SHELL_OUTPUT
extern void print_to_shell_no_pos(const char * fmt,...);

#define LOG(fmt,...) printf("%s(%d)-%s: "fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define LOG2(fmt,...) printf(fmt, ##__VA_ARGS__);
#define LOG6(fmt,...) printf("%s(%d): "fmt, __FILE__, __LINE__, ##__VA_ARGS__);
#endif

#ifdef FEATURE_DEBUG_LOG_FILE
#define LOG_FILE_PATH "/usr/bin/qlinkdatanode.log"

extern void print_to_file(const char *file_name, 
                            const unsigned int line_num, 
                            const char *func_name,
                            const char *log);
extern void print_to_file_no_lck(const char *file_name, 
                            const unsigned int line_num, 
                            const char *func_name,
                            const char *log);
extern void print_to_file_no_pos_n_lck_n_time(const char *fmt,...);
extern void print_to_file_no_pos_n_lck_but_space(const char * fmt,...);
extern void print_to_file_no_pos_n_lck(const char *fmt,...);

#define LOG(fmt,...) \
  { \
    char log[MAX_LOG_BUF_LEN] = {0};\
    snprintf(log, MAX_LOG_BUF_LEN, fmt, ##__VA_ARGS__); \
    print_to_file(__FILE__, __LINE__, __FUNCTION__, log); \
  }
#define LOG2(fmt,...) \
  { \
    char log[MAX_LOG_BUF_LEN] = {0};\
    snprintf(log, MAX_LOG_BUF_LEN, fmt, ##__VA_ARGS__); \
    print_to_file_no_lck(__FILE__, __LINE__, __FUNCTION__, log); \
  }
#define LOG3 print_to_file_no_pos_n_lck_n_time
#define LOG4 print_to_file_no_pos_n_lck_but_space
#define LOG5 print_to_file_no_pos_n_lck

  //Not print function name. Lck is acqed inside.
#define LOG6(fmt,...) \
  { \
    char log[MAX_LOG_BUF_LEN] = {0};\
    snprintf(log, MAX_LOG_BUF_LEN, fmt, ##__VA_ARGS__); \
    print_to_file(__FILE__, __LINE__, NULL, log); \
  }
#endif

#ifdef FEATURE_DEBUG_QXDM
extern void print_to_QXDM_no_pos(const char *fmt,...);
extern void print_to_QXDM_internal(const char *log);
		
		#define LOG(fmt,...) \
		{ \
			char prt_buf[DS_MAX_DIAG_LOG_MSG_SIZE] = {0}; \
			snprintf(prt_buf, \
							DS_MAX_DIAG_LOG_MSG_SIZE, \
							"%s(%d)-%s: "fmt, \
							__FILE__, \
							__LINE__, \
							__FUNCTION__, \
							##__VA_ARGS__ \
						  ); \
			print_to_QXDM_internal(prt_buf); \
		}
		#define LOG2 print_to_QXDM_no_pos
		//Unitive macro name
		#define LOG6(fmt,...) \
		{ \
			char prt_buf[DS_MAX_DIAG_LOG_MSG_SIZE] = {0}; \
			snprintf(prt_buf, \
							DS_MAX_DIAG_LOG_MSG_SIZE, \
							"%s(%d): "fmt, \
							__FILE__, \
							__LINE__, \
							##__VA_ARGS__ \
						  ); \
			print_to_QXDM_internal(prt_buf); \
		}
#endif

#if !defined(FEATURE_DEBUG_SHELL_OUTPUT)&& \
		  !defined(FEATURE_DEBUG_LOG_FILE)&& \
		  !defined(FEATURE_DEBUG_SYSLOG)&& \
		  !defined(FEATURE_DEBUG_QXDM)
	#undef FEATURE_ENABLE_DEBUG_MODE
	#define LOG print_to_null
	#define LOG2 print_to_null
	#define LOG6 print_to_null
#endif

#endif //FEATURE_ENABLE_DEBUG_MODE


#ifdef FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE_AT_ONCE
#define REMOTE_USIM_DATA_LOG_FILE_PATH "/usr/bin/remote_usim_data_log"
#endif

#ifdef FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE
#define RECVED_DATA_LOG_FILE_PATH "/usr/bin/recved_data_log"
#endif

#define NET_RX_FILE "/sys/class/net/rmnet0/statistics/rx_bytes"
#define NET_TX_FILE "/sys/class/net/rmnet0/statistics/tx_bytes"

#ifdef FEATURE_VER_QRY
#define VER_FILE_PATH "/usr/bin/qlinkdatanode_version_file"
#endif

#ifdef FEATURE_MDMEmodem_BOOT_STAT_QRY
#define MDMEmodem_BOOT_STAT_FILE_PATH "/usr/bin/mdmEmodem_stat_file"
#endif

#define YY_CFG_PATH  "/etc/yy_cfg"

#define MAX_DEV_PATH_CHARS (15)
#define MAX_SERV_ADDR_CHARS (22)
  
#ifdef FEATURE_ENABLE_TIMER_SYSTEM
#define MAX_STARTUP_TIMER_SEC (10)
#define MAX_STARTUP_TIMER_USEC (0)

#ifdef FEATURE_ENABLE_APDU_RSP_TIMEOUT_SYS
//Phase 1: Recv APDU from modem.
#define DEFAULT_APDU_RSP_EXT_PHASE_1_TIMEOUT_SEC (20)
#define DEFAULT_APDU_RSP_EXT_PHASE_1_TIMEOUT_USEC (0)
//Phase 2: Send out APDU to server.
#define DEFAULT_APDU_RSP_EXT_PHASE_2_TIMEOUT_SEC (20)
#define DEFAULT_APDU_RSP_EXT_PHASE_2_TIMEOUT_USEC (0)
#endif

// add by jack li 20161123 start

#ifdef FEATURE_ENABLE_SERVER_RSP_TIMER

#ifdef FEATURE_ENABLE_IDCHECK_NEW_TIMEOUT_PROCESS
#define DEFAULT_ID_AUTH_RSP_TIMEOUT_SEC (60)
#else

#define DEFAULT_ID_AUTH_RSP_TIMEOUT_SEC (20)

#endif

// add by jack li 20161123 end

#define DEFAULT_ID_AUTH_RSP_TIMEOUT_USEC (0)
#define DEFAULT_REMOTE_USIM_DATA_RSP_TIMEOUT_SEC (360)
#define DEFAULT_REMOTE_USIM_DATA_RSP_TIMEOUT_USEC (0)
#endif

#ifdef FEATURE_ENABLE_ATE0_RSP_TIMER
#define DEFAULT_ATE0_RSP_TIMEOUT_SEC (2)
#define DEFAULT_ATE0_RSP_TIMEOUT_USEC (0)
#endif

#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
#define DEFAULT_MDMEmodem_IDLE_TIMER_SEC (8)
#define DEFAULT_MDMEmodem_IDLE_TIMER_USEC (0)
#endif

#ifdef FEATURE_ENABLE_PDP_ACT_CHECK_TIMER
#define DEFAULT_PDP_ACT_CHECK_INTERVAL_SEC (5)
#define DEFAULT_PDP_ACT_CHECK_INTERVAL_USEC (0)
#endif

#ifdef FEATURE_ENABLE_POWER_DOWN_NOTIFICATION_TIMER
#define DEFAULT_POWER_DOWN_NOTIFICATION_RSP_SEC (20)
#define DEFAULT_POWER_DOWN_NOTIFICATION_RSP_USEC (0)
#endif

#endif // FEATURE_ENABLE_TIMER_SYSTEM


#define DEFAULT_OPER_MODE_SETTING_DURATION (3)

#define IMEI_DATA_LEN (15)
#define IMSI_DATA_LEN (9)
#define PLMN_DATA_LEN (3)
#define LAC_DATA_LEN (2)
#define IMEI_BIT 0x01
#define IMSI_BIT 0x02
#define PLMN_BIT 0x04
#define LAC_BIT 0x08


#define APN_MESSAGE_LEN (200)

#define DEFAULT_BYTES_RECVED_FOR_ONCE (128)
#define MAX_TCP_URC_HEADER_LEN (47)
#define AT_BUF_LEN (256)

#define qlinkdatanode_MAX_PATH_NAME_LENGTH (512)

#define CSQ_VAL_THRESHOLD_OF_POOR_SIG (4)

#define LOG_TIME_MARK_LEN (19)
#define LOG_LINE_BREAK_LEN (1)

#define NOTE_TRAFFIC_OUTOF 0x00
#define NOTE_DATE_OUTOF 0x01
#define NOTE_SHOW_DATA_RATE 0x02
#define NOTE_SHOW_BOOK 0x03
#define NOTE_SHOW_MIN 0x04
#define NOTE_JUMP_WINDOWN 0x05
#define NOTE_CANCEL_APPLE 0x06

#ifdef FEATURE_ENABLE_CHANNEL_LOCK_LOG
#ifndef CHAN_LCK_NUM_DEFINED
extern int chan_lck_num;
#endif

	#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
		#define ACQ_CHANNEL_LOCK_WR \
		{ \
			chan_lck_num ++; \
			LOG6("##### ACQ_CHANNEL_LOCK_WR (%d) #####\n", chan_lck_num); \
			pthread_rwlock_wrlock(&ChannelLock); \
		}
		
		#define ACQ_CHANNEL_LOCK_RD \
		{ \
			chan_lck_num ++; \
			LOG6("##### ACQ_CHANNEL_LOCK_RD (%d) #####\n", chan_lck_num); \
			pthread_rwlock_rdlock(&ChannelLock); \
		}
		
		#define RLS_CHANNEL_LOCK \
		{ \
			LOG6("##### RLS_CHANNEL_LOCK (%d) #####\n", chan_lck_num); \
			pthread_rwlock_unlock(&ChannelLock); \
		}
	#else
		#define ACQ_CHANNEL_LOCK_WR \
		{ \
			chan_lck_num ++; \
			pthread_mutex_lock(&ChannelLock); \
		}
		
		#define ACQ_CHANNEL_LOCK_RD \
		{ \
			chan_lck_num ++; \
			pthread_mutex_lock(&ChannelLock); \
		}
		
		#define RLS_CHANNEL_LOCK \
		{ \
			pthread_mutex_unlock(&ChannelLock); \
		}
	#endif // FEATURE_ENABLE_RWLOCK_CHAN_LCK
#else
	#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
		#define ACQ_CHANNEL_LOCK_WR \
		{ \
			pthread_rwlock_wrlock(&ChannelLock); \
		}
		
		#define ACQ_CHANNEL_LOCK_RD \
		{ \
			pthread_rwlock_rdlock(&ChannelLock); \
		}
		
		#define RLS_CHANNEL_LOCK \
		{ \
			pthread_rwlock_unlock(&ChannelLock); \
		}
	#else
		#define ACQ_CHANNEL_LOCK_WR \
		{ \
			pthread_mutex_lock(&ChannelLock); \
		}
		
		#define ACQ_CHANNEL_LOCK_RD \
		{ \
			pthread_mutex_lock(&ChannelLock); \
		}
		
		#define RLS_CHANNEL_LOCK \
		{ \
			pthread_mutex_unlock(&ChannelLock); \
		}
	#endif /*FEATURE_ENABLE_RWLOCK_CHAN_LCK*/
#endif

#ifdef FEATURE_ENABLE_ACQ_DATA_STATE_MTX_LOG
	#ifndef ACQ_DATA_STATE_MTX_NUM_DEFINED
		extern int acq_data_state_mtx_num;
	#endif

#ifdef FEATURE_ENABLE_CANCEL_LOG_OUTPUT
	#define ACQ_ACQ_DATA_STATE_MTX \
	{	\
		acq_data_state_mtx_num ++; \
		LOG6("$$$$$ ACQ_ACQ_DATA_STATE_MTX (%d) $$$$$\n", acq_data_state_mtx_num); \
		pthread_mutex_lock(&acq_data_state_mtx); \
	}
#else
	#define ACQ_ACQ_DATA_STATE_MTX \
	{	\
		acq_data_state_mtx_num ++; \
		pthread_mutex_lock(&acq_data_state_mtx); \
	}


#endif

#ifdef FEATURE_ENABLE_CANCEL_LOG_OUTPUT
	//Define try_lck_result before.
	#define TRY_ACQ_ACQ_DATA_STATE_MTX \
	{	\
		try_lck_result = pthread_mutex_trylock(&acq_data_state_mtx); \
		if(0 == try_lck_result){ \
			acq_data_state_mtx_num ++; \
			LOG6("$$$$$ TRY_ACQ_ACQ_DATA_STATE_MTX (%d) $$$$$\n", acq_data_state_mtx_num); \
		} \
	}
	
	#define RLS_ACQ_DATA_STATE_MTX \
	{	\
		LOG6("$$$$$ RLS_ACQ_DATA_STATE_MTX (%d) $$$$$\n", acq_data_state_mtx_num); \
		pthread_mutex_unlock(&acq_data_state_mtx); \
	}

#else

	#define TRY_ACQ_ACQ_DATA_STATE_MTX \
	{	\
		try_lck_result = pthread_mutex_trylock(&acq_data_state_mtx); \
		if(0 == try_lck_result){ \
			acq_data_state_mtx_num ++; \
		} \
	}
	
	#define RLS_ACQ_DATA_STATE_MTX \
	{	\
		pthread_mutex_unlock(&acq_data_state_mtx); \
	}
#endif

#else
	#define ACQ_ACQ_DATA_STATE_MTX \
	{	\
		pthread_mutex_lock(&acq_data_state_mtx); \
	}
	
	//Define try_lck_result before.
	#define TRY_ACQ_ACQ_DATA_STATE_MTX \
	{	\
		try_lck_result = pthread_mutex_trylock(&acq_data_state_mtx); \
	}
	
	#define RLS_ACQ_DATA_STATE_MTX \
	{ \
		pthread_mutex_unlock(&acq_data_state_mtx); \
	}
#endif /*FEATURE_ENABLE_ACQ_DATA_STATE_MTX_LOG*/

typedef enum bool_t {false=0, true=!false}bool;

typedef struct _Buffer_Config_Tag{
  unsigned int size;
  void *data;
}rsmp_recv_buffer_s_type;

typedef struct _Dev_Info_Tag{
  char IMEI[IMEI_DATA_LEN];
  char IMSI[IMSI_DATA_LEN];
  char PLMN[PLMN_DATA_LEN];
  char LAC[LAC_DATA_LEN]; // When system mode is at LTE, the LAC is represented as TAC.
  char CELLID[4]; // 
  unsigned char power_lvl; // Battery power percentage. Standard Value: 0~99.(Unit: %)
  unsigned char sig_strength; //RSSI: Standard Value: 0~31. Invalid Value: 99.
  unsigned int flux[2]; // Little Endian Format. Not compatible with corresponding filed of private protocol.
}Dev_Info;

typedef struct _Net_Config_Tag{
  int link_num;
  char *protocol;
  char server_IP[16];
  int server_port;
  //The length of TCP URC header pre
  //i.e. "\r\nRECV FROM:[IP addr]:[Port num]\r\n+IPD"
  int header_pre_len;
}Server_Config;

typedef struct {
  //Description:
  //0x00: SIM error
  //0x01: SIM Ready, Reg searching
  //0x02: SIM Ready, Reg finish, Dial ongoing
  //0x03: SIM Ready, Reg fail permanently
  //0x04: SIM Ready, Reg finish, Dial finish
  //0x05: SIM Ready, Reg finish, Dial fail permanently 
  char basic_val;
  //Description:
  //0x01: Not req pin code
  //0x02: Pin code reqed.
  char cpin_val;
  char cnsmod_val;
  char cgreg_val;
  char cereg_val;
  char cregrej_val;
  char cnmp_val;
}Net_Info;

typedef struct _Req_Config_Tag{
  //data size, not struct size
  int size;
  void *data;
  unsigned int StatInfMsgSn;
}rsmp_transmit_buffer_s_type;

typedef struct _Req_Cclk_Tag{
   int year;
   int month;
   int mday;
   int hour;
   int min;
   int sec;
}rsp_cclk_time_type;

typedef enum {
  THD_IDX_MAIN,
  THD_IDX_EVTLOOP,
  THD_IDX_LISTENERLOOP,
  THD_IDX_SOCKLISTENERLOOP,
  THD_IDX_MSGLOOP,
  THD_IDX_PARSER,
  THD_IDX_RECVER,
  THD_IDX_SENDER,
  THD_IDX_UIM_SENDER,
  THD_IDX_DMS_SENDER,
  THD_IDX_NAS_SENDER,
  THD_IDX_MAX
}Thread_Idx_E_Type;

#ifdef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
typedef struct {
  char val[3];
}Time_Zone_S;
#endif

#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER
typedef struct {
  char flu_limit_data_level1[4];
  char flu_limit_speed_level1[4];
  char flu_limit_data_level2[4];
  char flu_limit_speed_level2[4];
}Flu_limit_control;
#endif

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
typedef struct {
  char trafficBuyTotal[8];
  char usedTrafficByUser[8];
  char usedTimeByUser[4];
}trafficFromServercontrol;
#endif

typedef enum _Chk_Net_State_Tag{
  CHK_NET_STATE_IDLE,
  CHK_NET_STATE_CSQ,
  CHK_NET_STATE_CGREG,
  CHK_NET_STATE_MAX
}Chk_Net_State;


typedef enum _Conf_State_Tag{
  CONF_STATE_IDLE,
  CONF_STATE_IFC,
  CONF_STATE_IPR,
  CONF_STATE_MAX
}Conf_State;

typedef enum _Acq_Data_State_Tag{
  ACQ_DATA_STATE_IDLE, /*0*/



 
  ACQ_DATA_STATE_NETOPEN, /*1*/
  
 #ifdef  FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
 
  ACQ_DATA_STATE_CIPOPEN_REQ, /*2*/
  ACQ_DATA_STATE_CIPCLOSE, /*3*/
  
#endif  

  ACQ_DATA_STATE_CIPOPEN, /*4*/
  ACQ_DATA_STATE_CIPSEND_1, /*5*/
  ACQ_DATA_STATE_CIPSEND_2, /*6*/
  ACQ_DATA_STATE_RECVING, /*7*/
  ACQ_DATA_STATE_WAITING, /*8*/

  ACQ_DATA_STATE_CGSN, /*9*/ //Make its value be 7 whenever FEATURE_USE_MDMEmodem_IMEI is on
  ACQ_DATA_STATE_COPS_1, /*10*/
  ACQ_DATA_STATE_COPS_2, /*11*/

  #ifdef  FEATURE_ENABLE_CPOL_CONFIG
  ACQ_DATA_STATE_COPS_3, /*12*/ //12
  ACQ_DATA_STATE_COPS_4, /*13*/ //12
  #endif

  #ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG
  ACQ_DATA_STATE_CCLK, /*14*/
 #endif

  
  ACQ_DATA_STATE_CREG_1, /*15*/ //10
  ACQ_DATA_STATE_CREG_2, /*16*/ //11
  
  ACQ_DATA_STATE_CREG_3, /*17*/
  
#ifdef FEATURE_ENABLE_CATR
  ACQ_DATA_STATE_CATR_1, /*18*/
  ACQ_DATA_STATE_CATR_2, /*19*/
#endif // FEATURE_ENABLE_CATR

  ACQ_DATA_STATE_CUSTOM_AT_CMD,
  
  ACQ_DATA_STATE_MAX
}Acq_Data_State;

typedef enum{
  SUSPEND_STATE_IDLE,
  SUSPEND_STATE_CIPCLOSE,
  SUSPEND_STATE_NETCLOSE,
  SUSPEND_STATE_MAX
}Suspend_Mdm_State;

typedef enum{
  LINKEmodem_STATE_IDLE,
  LINKEmodem_STATE_UDP,
  LINKEmodem_STATE_TCP,
  LINKEmodem_STATE_MAX
}LINKEmodem_Mdm_State;

#ifdef FEATURE_CONFIG_SIM5360
typedef enum {
  ACQ_MFG_INF_STATE_IDLE,
  ACQ_MFG_INF_STATE_SIMCOMATI,
  ACQ_MFG_INF_STATE_CSUB,
  ACQ_MFG_INF_STATE_SIMEI,
  ACQ_MFG_INF_STATE_CICCID,
  ACQ_MFG_INF_STATE_CNVR_1,
  ACQ_MFG_INF_STATE_CPIN,
  ACQ_MFG_INF_STATE_MAX
}Acq_Mfg_Inf_State_E_Type;

typedef enum {
  CHECK_PDP_CTX_STATE_MIN,
  CHECK_PDP_CTX_STATE_CGDCONT,
  CHECK_PDP_CTX_STATE_CGAUTH,
  CHECK_PDP_CTX_STATE_MAX
}Check_Pdp_Ctx_State_E_Type;

typedef enum {
  SET_PDP_CTX_STATE_MIN,
  SET_PDP_CTX_STATE_CGDCONT,
  SET_PDP_CTX_STATE_CGAUTH,
  SET_PDP_CTX_STATE_MAX
}Set_Pdp_Ctx_State_E_Type;
#endif


#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

typedef enum {
  SOFT_CARD_STATE_FLOW_DEFAULT = 0,
  SOFT_CARD_STATE_FLOW_GETKEY_REG = 1,
  SOFT_CARD_STATE_FLOW_GETKIOPC_REG = 2,
  SOFT_CARD_STATE_FLOW_MAX = 3
}Soft_Card_State_Flow_Type;

#endif

extern unsigned char get_byte_of_void_mem(void * ptr, unsigned int idx);
extern int check_IP_addr(const char *str);
extern int check_port(const int server_port);
extern int check_dev_addr(const char *str);
extern int parse_serv_addr(const char *str, char *IP, int *port);
extern void reset_rsmp_recv_buffer(void);
extern void reset_rsmp_transmit_buffer(rsmp_transmit_buffer_s_type *config);
#ifdef FEATURE_ENABLE_TIMESTAMP_LOG
extern void print_cur_gm_time(const char*);
#endif
extern void *find_sub_content(const void *src, void *tgt, unsigned int len_src, unsigned int len_tgt);
extern void init_dev_info(Dev_Info *di);
extern unsigned int conv_4_byte_be_mem_to_val(void *mem);

#endif //COMMON_qlinkdatanode_H_INCLUDED
