#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

#include "socket_qlinkdatanode.h"
#include "event_qlinkdatanode.h"
#include "protocol_qlinkdatanode.h"
#include "common_qlinkdatanode.h"
#include "qmi_parser_qlinkdatanode.h" // notify_Sender()
#include "qmi_sender_qlinkdatanode.h" // notify_Sender()
#include "at_rsp_qlinkdatanode.h" // process_COPS, process_CGREG_lac
#include "queue_qlinkdatanode.h" // add_node
#include "at_qlinkdatanode.h" // add_node
/******************************External Functions*******************************/
extern void notify_EvtLoop(const int msg);
extern void notify_UimSender(char opt);
extern void notify_NasSender(char opt);
extern void notify_DmsSender(char opt);
#ifdef FEATURE_ENABLE_SYSTEM_RESTORATION
extern void notify_RstMonitor(char msg);
#endif
extern void push_ChannelNum(const int chan_num);
extern void push_flow_state(const rsmp_state_s_type fs);
extern void push_thread_idx(const Thread_Idx_E_Type tid);
extern int msq_qry_pwr_inf(void);
extern void msq_send_pd_rsp(void);
extern void msq_send_dialup_ind(void);
extern void msq_send_qlinkdatanode_run_stat_ind(int evt);
extern void msq_send_mdm9215_usim_rdy_ind(int opt);
extern void msq_send_mdm9215_reg_status_ind(int opt);
extern void msq_send_mdm9215_nw_sys_mode_ind(int val);
extern int one_digit_hex_to_dec(const char digit);
extern void set_ChannelNum(const int val);
extern int check_ChannelNum(const int opt);
extern int get_ChannelNum(void);
extern int check_acq_data_state(Acq_Data_State state);
extern Acq_Data_State get_acq_data_state(void);
extern void print_fmt_data(char * str);
extern rsmp_protocol_type conv_flow_state_to_req_type(rsmp_state_s_type state);
extern int mdmEmodem_check_busy(void);
extern int mdmEmodem_check_if_running(void);
extern void mdmEmodem_shutdown(void);
extern int proto_update_flow_state(rsmp_state_s_type state);
extern void upload_status_info_and_power_up_virt_USIM_later(void);
extern void clr_pdp_check_act_routine_timer(void);
extern int check_rsp_timers(void);
extern void add_timer_mdmEmodem_idle_timeout(char opt);
extern Thread_Idx_E_Type get_thread_idx(void);

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

extern int msqSendToTrafficAndDateYydaemon(char versionf,long long fluRate);


#endif


/******************************External Variables*******************************/
extern char IP_addr[16];
extern int server_port;
extern bool g_qlinkdatanode_run_status_03;

extern rsmp_transmit_buffer_s_type g_rsmp_transmit_buffer[THD_IDX_MAX+1];

extern int ChannelNum;
#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
extern pthread_rwlock_t ChannelLock;
#else
extern pthread_mutex_t ChannelLock;
#endif

extern At_Config at_config;
extern bool b_enable_softwarecard_ok;
//Description:
//The vars below are all for synchronizing threads in situation that EvtLoop request sock thread
//to do some actions.
pthread_mutex_t el_req_mtx;
pthread_cond_t el_req_cnd;
bool el_reqed = false;

extern rsmp_control_block_type flow_info;
extern Dev_Info sim5360_dev_info;
extern Dev_Info rsmp_dev_info;

extern Suspend_Mdm_State suspend_state;

extern bool isQueued;
extern bool isNeedToSendIDMsg;
extern bool isWaitToSusp;
extern bool isWaitToRcvr;
extern bool isTermSuspNow;
extern bool isTermRcvrNow;
extern bool isUimPwrDwn;
extern bool RcvrMdmEmodemConnWhenSuspTerm;
extern bool PowerUpUsimAfterRecvingUrcRpt;
extern bool isPwrDwnRspAcq;
extern bool isSockListenerRecvPdpActMsg;
extern bool isAutoReprotPowerDown;

extern bool isEnableManualApdu;

extern bool isUimPwrDwnMssage;


//  Explanation:
//		isEnableManualApdu was set true when ApduRsp ext timer expired, and set false when virt USIM
//	was powered up later. So, during this period of setting isEnableAssemblyApdu from true to false, it was 
//	in process of dealing with APDU rsp timeout.

//  Applicable Situation:
//		isApduRspTimeout is used for determining if status info upload triggered by reg inds is valid or not. If
//	APDU rsp timeout, status info upload triggered by reg inds is invalid then.(Abandoned!!!)
//		isApduRspTimeout is used for determining if reg inds are valid for processing.
#define isApduRspTimeout (isEnableManualApdu)

extern pthread_mutex_t evtloop_mtx;

extern bool isUimRstOperCplt;
extern pthread_mutex_t rst_uim_mtx;
extern pthread_cond_t rst_uim_cond;

extern bool isRstingUsim;
extern bool isRstRf;
extern bool isRfRsted;
extern pthread_mutex_t rst_rf_mtx;
extern pthread_cond_t rst_rf_cond;

extern int nas_inf_acqed;
extern pthread_mutex_t acq_nas_inf_mtx;
extern pthread_cond_t acq_nas_inf_cnd;

extern int pwr_inf_acqed;
extern pthread_mutex_t pwr_inf_qry_mtx;
extern pthread_cond_t pwr_inf_qry_cnd;

extern int dms_inf_acqed;
extern pthread_mutex_t acq_dms_inf_mtx;
extern pthread_cond_t acq_dms_inf_cnd;

extern int StatInfReqNum;

extern pthread_mutex_t log_file_mtx;
extern pthread_mutex_t pd_mtx;

extern unsigned long thread_id_array[THD_IDX_MAX];
extern int qlinkdatanode_run_stat;

#ifdef FEATURE_ENABLE_4G3GAUTO_FOR_US

extern bool isneedtoset4G3Gauto;

#endif

extern unsigned char g_qlinkdatanode_run_status_01;
extern unsigned char g_qlinkdatanode_run_status_02;

#ifdef FEATURE_ENABLE_EmodemUDP_LINK

extern bool g_EmodemUdp_link_server_state;

extern bool g_enable_EmodemTCP_link;
extern unsigned char g_enable_TCP_IP01_linkcount;
extern unsigned char g_enable_TCP_IP02_linkcount;
extern bool g_isip1_use;
extern bool g_isip2_use;
extern int server_port;
extern unsigned int tcp_server_port;
extern unsigned int tcp_server_port_2;
extern char tcp_IP_addr[16];
extern char tcp_IP_addr_2[16];
extern char IP_addr[16];



#endif

extern unsigned char  g_is7100modemstartfail;

#ifdef FEATURE_ENABLE_CONTRL_EmodemREBOOT_0117DEBUG

extern unsigned char g_is7100modem_register_state;

#endif

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

	extern trafficFromServercontrol trafficDataServer;
	extern bool isBookMode;
	extern bool isCancelRecoveryNet;

#endif


#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

extern bool isTestMode;

#endif


/******************************Local Variables*******************************/
static int at_fd = -1;
static int listen_fd = -1;
//   1: Success
//   0: Operation failed. e.g. AT command response is ERROR.
// -1: Recv excessive amount of data.
// -2: Internal function failed. e.g. read() failed.
// -3: Unexpected msg recved.
// -4: Wrong param of function found.
// -5: Need to retry. e.g. OK of rsp not found.
// -6: In parse_2nd_param_of_URC_ext(), recv unexpected URC report about registration.
// -7: In parse_2nd_param_of_URC_ext(), recv unexpected URC report about registration rejection.
static int err_num_s = 0xff;

static pthread_t listenerloop_tid;
static pthread_mutex_t listener_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t listener_cond = PTHREAD_COND_INITIALIZER;
static int listenerStarted;

//Function:
//Make sure no conflict of different AT rsp in their hdlr.
static pthread_mutex_t at_mutex = PTHREAD_MUTEX_INITIALIZER;

static char unexp_urc_buf[128] = {0};

//Description:
//	When app send AT cmd to mdm9215 but recv unexpected URC report, app need wait to
//	recv AT cmd rsp and ignore URC report for time being. At this moment, it was set true, and 
//	it would be set false when AT cmd rsp is recved.
static bool isWaitForRsp = false; // Add by rjf at 20150713

//Description:
//	Once reg was denied for rej cause #15, it would be set true, and app would need to check rej cause once
//	recved ind "CXREG: 0,2".
//static bool isRejCause15Recved = false;
//Description:
//	Once URC rpt "+REGREJ:..." recved (or rej cause checked by AT cmd rsp), it would be set true. And
//	no need to check AT cmd rsp when URC rpt "+CXREG: 3" (or handle the URC rpt "+REGREJ:...").
//	It would be set false when virt usim was set LPM for recving unrecoverable reg rej cause.
static bool isUnrcvRejCauseRecved = false;

//Description:
//		Once reg ind recved by socket thread, app will save its reg stat val into new_reg_stat.
//	And new_reg_stat will replace last_reg_stat with itself before updating new_reg_stat.
//	The app will check if new_reg_stat is different to last_reg_stat. If yes, app may
//	see it as reg off.
//Reason for adding this:
//	Handling the special situation in which signal is poor but "+CXREG: 0" not reported. In this situation,
//	you are just acquire reg status with "+CXREG: 2".
// Add by rjf at 201507271601
static int new_reg_stat = -1;
static int last_reg_stat = -1;

static int URC_array_len = 0;
static char URC_array[MAX_URC_ARRAY_LEN+1][MAX_URC_LEN];
//Description:
//For the limitation of SOCK_READ_BYTES, app may read less than what buffer has. Then this URC fragment
//will be stored into URC_frag.
static char urc_fragment[MAX_URC_LEN] = {0};
//Description:
//For the limitation of size of URC_array, app may miss some URCs in read buffer. So this pointer will point to
//the unread URC in the read buffer.
static char *URC_buf_break_point = NULL;
static int valid_URC_prefix_array_len = 0;
static char valid_URC_prefix_array[MAX_URC_ARRAY_LEN+1][MAX_URC_LEN];
//Description:
//In check_valid_URC_array_elem(), the qualified elements of URC array will make the elements of URC_valid_mark_array
//with the corresponding indexes be set 1.
static char URC_valid_mark_array[MAX_URC_ARRAY_LEN] = {0};

//Description:
//It's the array which contains the special URC string you want to monitor.
//Elements:
//		Idx 1 : "+SIMCARD: NOT AVAILABLE"
static char special_URC_array[MAX_SPEC_URC_ARR_LEN][MAX_URC_LEN];
static int monitored_special_URC_num = 0;
//Description:
//	It is the array of marks which indicates that URCs of corresponding index in special_URC_array is recved.
//	The elem in this arr will be reset or initialized depended on different situation.
//	Elements:
// Idx 1 : 1 ----- When "+SIMCARD: NOT AVAILABLE" is recved.
// 0 ----- When virtual card is powered up again.
char URC_special_mark_array[MAX_SPEC_URC_ARR_LEN] = {0};

static Reg_URC_State_E_Type reg_urc_state = REG_URC_STATE_IDLE;
static Reg_URC_State_E_Type last_reg_urc_state = REG_URC_STATE_IDLE;

//Description:
//Once reg is rejected for cause #15 and URC "+CXREG: 2" is recved, this will be set true. After checking the rejection cause
//after recving "+CXREG: 2", this bool var will be set false.
//static bool need_to_check_rej_cause = false;
static Reg_Sys_Mode_State_E_Type reg_sys_mode_state = REG_SYS_MODE_MIN;
static Reg_Sys_Mode_State_E_Type last_reg_sys_mode_state = REG_SYS_MODE_MIN;

static bool need_to_upload_stat_inf = false;

//Description:
//When USIM is set powerdown manually, app need to wait for recving "+SIMCARD: NOT AVAILABLE". This var is set for remind app
//of exiting from listener_hdlr() when "+SIMCARD: NOT AVAILABLE" has been recved already.
static bool isSimCardNotAvailRecved = false;
static bool isSessIllegalRecved = false;


/******************************Global Variables*******************************/
strflu_record strGobalstandTime;

bool isRegOn = false;
bool isMifiConnected = false;

bool is1stRunOnMdmEmodem = true;

pthread_mutex_t i1r_mtx;
bool isMdmEmodemRunning = false;

//xiaobin add
bool isUMTSOnlySet = false;

static unsigned int rab_restab_rej_or_fail_counter = 0;

Net_Info rsmp_net_info = {
  .basic_val = 0x00,
  .cpin_val = 0x02,
  .cnsmod_val = 0x00,
  .cgreg_val = 0x00,
  .cereg_val = 0x00,
  .cregrej_val = 0x00,
  .cnmp_val = 0x00
};

int SockListener_2_Listener_pipe[2];
int EvtLoop_2_Listener_pipe[2];

//Description:
//	This bool var is for indicating if mdm9215 is registered.
//	If mdm9215 was deregistered of poor signal, it would be set true. And app would set uim driver LPM and powerdown then.
//	When mdmEmodem is reconnecting to the server, (it basically indicate that reg network is available) uim driver will be set Online
//	Mode and powerup, then this var will be set false.
//	The 3 scenes as below:
//		1. Timeout when send APDU to server.
//		2. Recv "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" of mdmEmodem URC rpt.
//		3. Recv "+CXREG: 0" of mdm9215 URC rpt.
//	As if these 3 scenes show up one by one, it will trigger the processing handler of latest scene no matter if last executed 
 //	processing handler is still running. It means if that isRegOffForPoorSig = true will not block the execution of new processing
//	handler of these 3 scenes.
bool isRegOffForPoorSig = false;

bool isMdm9215CpinReady = false;

//Description:
//	StatInfMsgSn:
//		This var is marked as the serial num of latest status info msg.
//	stat_inf_mtx:
//			This mutex is for avoiding that status_info_upload_acq_info() is called by 2 threads at same time.
//		StatInfMsgSn may not be accurate.
unsigned int StatInfMsgSn = 0;
pthread_mutex_t stat_inf_mtx = PTHREAD_MUTEX_INITIALIZER;


/******************************Reference Declaration*******************************/
void closeSock(void);
static void FdListener(const int fd, evt_cb cb_func, void *userdata);
void createSock(void);
static int trigger_rsmp_status_report(void);
void get_dev_and_net_info_for_status_report_session(void);
int mdm9215_check_CNSMOD(void);
int mdm9215_check_CSQ(void);
//int sock_check_reg_rej_cause(const int fd);
#ifdef  FEATURE_ENABLE_STATUS_REPORT_10_MIN
rsmp_transmit_buffer_s_type req_config_get = {-1, NULL, true};

static pthread_t status_parser_tid;
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t status_cond = PTHREAD_COND_INITIALIZER;

static int statusStarted;
int Status_rw_pipe[2]; 
Evt statusReport_timeout_evt;


#endif


/******************************Macro And Types*******************************/
#define LISTENER_CHANNEL "/dev/smd11"
#define AT_CHANNEL "/dev/smd8"

#define PRIV_PROT_APDU_MSG_HEAD_IDX_S (0)
#define PRIV_PROT_APDU_MSG_LEN_IDX_S (1)
#define PRIV_PROT_APDU_REQ_TYPE_IDX_S (5)
#define PRIV_PROT_APDU_DATA_TYPE_1_IDX_S (6)
#define PRIV_PROT_APDU_UEID_IDX_S (7)
#define PRIV_PROT_APDU_DATA_TYPE_2_IDX_S (11)
#define PRIV_PROT_APDU_DATA_LEN_IDX_S (12)
#define PRIV_PROT_APDU_DATA_IDX_S (14)

typedef void (*evt_cb)(void *userdata);
typedef struct{
	int fd;
	char *pre;
	int ret;
}Stat_Check_Hdlr_Param_S_Type;

typedef struct {
	int fd;
	int rej_cause;
}Reg_Rej_Hdlr_Param_S_Type;

//parameters:
//	i: The integer value of port number.
//	p: The pointer which pointed to storage space of result.
#define ITOA(i, p) \
{ \
	int n = i; \
	int r; \
	int cnt; \
	int div = 10000; \
	for( cnt=0; cnt<5; cnt++){ \
		p[cnt] = '0'; \
	} \
	p[cnt] = '\0'; \
	for( cnt=0; cnt<5; cnt++){ \
		if((r = (n)/div) != 0){ \
			p[cnt] = r+'0'; \
		} \
    	n -= r*div; \
		div /= 10; \
	} \
	cnt = 0; \
	while(p[cnt] == '0'){ \
    	memmove(p, p+1, 5-(cnt+1)); \
    	p[5-(cnt+1)] = '\0'; \
	} \
}

//function:
//	No param examination in this macro.
//Parameters:
//	us: The type is unsigned int. It means us microseconds.
//	tv: The type is struct timeval. us microseconds will be converted to struct timeval.
#define US_TO_TIMEVAL(us,tv) \
{ \
  (tv).tv_usec = (us)%S_TO_US; \
  (tv).tv_sec = (us)/S_TO_US; \
}

#define SOCK_READ_OK(fd) \
{ \
	char rsp_buf[SOCK_READ_BYTES] = {0}; \
	int ret; \
	FdListener((fd), NULL, NULL); \
	if(err_num_s != 1) \
		return -3; \
	do{ \
		ret = read((fd), rsp_buf, SOCK_READ_BYTES); \
	}while((ret < 0 && errno == EINTR) || ret == 0); \
	if(ret < 0){ \
		LOG("read() failed in reception of rsp \"OK\".\n"); \
		return -4; \
	} \
}

//Return Values:
//	  1: Success
//	-2: Writen bytes is inadequate or write() failed.
//	-3: FdListener() failed.
//	-4: Fail to recv rsp of "OK".
static int send_at_and_check_response(const int fd, const char * at)
{
  int ret;

  ret = write(fd, at, strlen(at));
  if(ret != strlen(at)){
    if(ret < 0){
      LOG("write() error. errno=%s(%d).\n",
      strerror(errno), errno);
    }else{
      LOG("write() error. ret=%d.\n", ret);
    }
    return -2;
  }else{
    SOCK_READ_OK(fd);
  }

  return 1;
}

/*void yy_popen_call(const char   *command,char * data)
{
	int result = 0;
	FILE *stream = NULL;

	LOG("system call: %s \n", command);

	stream = popen( command, "r" );
	if (stream != NULL) 
	{ 
		if (fread(data, 1, 511, stream) != NULL) 
		{
			LOG("command succeed\r\n");
			LOG("!!!!!!!!!!%s!!!!!!!!!!\n",data);
		} 
		if( 0 > pclose( stream ) )
		{
			LOG("pclose command failed \n");
		}
	}
	else
	{
		LOG("system command failed error:%s \n",strerror(errno));
	}
}*/


int yy_popen_call( char   *command)
{
	int result = 0;
	FILE *stream = NULL;
	char data[512];

	LOG("system call: %s \n", command);
	memset(data, 0 , sizeof(data));
	stream = popen( command, "r" );
	if (stream != NULL) 
	{ 
		if (fread(data, 1, 511, stream) != NULL) 
		{
			LOG("command succeed\r\n");
			LOG("!!!!!!!!!!%s!!!!!!!!!!\n",data);
		} 
		if( 0 > pclose( stream ) )
		{
			LOG("pclose command failed \n");
			result = -1;
		}
	}
	else
	{
		if( 0 > pclose( stream ) )
		{
			LOG("pclose command failed \n");
		}
		LOG("system command failed error:%s \n",strerror(errno));
		result = -1;
	}

	return result;
}


//Parameters:
//	option=1: Check for err_num_s after calling FdListener().
//Return Values:
//	  2: Reg rej report recved.
//	  1: Success
//	  0: Retry again. Call sock_send_at_rd_cmd() again.
//	-2: Writen bytes is inadequate or write() failed.
//	-3: FdListener() failed.
int sock_send_at_rd_cmd(const int fd, const char *at, evt_cb hdlr, void *param, int option)
{
	int ret;
	
	LOG("          --> %s\n", at);
	ret = write(fd, at, strlen(at));
	if(ret != strlen(at)){
		if(ret < 0){
			LOG("ERROR: write() error. errno=%s(%d).\n",
					strerror(errno), errno);
		}else{
			LOG("ERROR: write() error. ret=%d.\n", ret);
		}
		return -2;
	}else{
		__SOCK_SEND_AT_RD_CMD_CALL_FDLISTENER__:
		FdListener(fd, hdlr, (param));
		if((option) == 1){
			if(err_num_s != 1 && err_num_s != -5 && err_num_s != -6 && err_num_s != -7){
				return -3;
			}else if(err_num_s == -5){
				LOG("err_num_s=%d. Call FdListener() again...\n", err_num_s);
				goto __SOCK_SEND_AT_RD_CMD_CALL_FDLISTENER__;
			}else if(err_num_s == -6){
				return 0;
			}else if(err_num_s == -7){
				return 2;
			}
		} // if((option) == 1)
	}
	
	return 1;
}

/*************************Basic Functions**************************/
void sock_update_reg_urc_state(Reg_URC_State_E_Type state)
{
  last_reg_urc_state = reg_urc_state;
  reg_urc_state = state;
  LOG("last_reg_urc_state=%d, reg_urc_state=%d.\n",
  last_reg_urc_state, reg_urc_state);
  return;
}

//Param:
//	tag = 0: +CPIN rsp changed.
//				 opt1 = 0x01 -> +CPIN: READY
//				 opt2 = 0x02 -> otherwise
//	tag = 1: Registration state changed.
//				 opt1: cnsmod
//				 opt2: cgreg
//				 opt3: cereg
//	tag = 2: Dial-up state changed.
//				 opt1: dial-up state. 0x00 means failed, and 0x01 success.
//Modication Records:
//	2015-9-10: Remove the update of mdmImodem_ni.cond when tag is 1.
void update_net_info(int tag, int opt1, int opt2, int opt3)
{

 LOG("Debug:  param input. tag=%d, opt1=%d,opt2=%d, opt3=%d .\n",tag, opt1,opt2,opt3);

  switch(tag)
  {
  case 0:
  {
    if(opt1 != 1 && opt1 != 2){
      LOG("ERROR: Wrong param input. tag=%d, opt1=%d.\n",tag, opt1);
      return;
    }
    rsmp_net_info.cpin_val = opt1;
    if(opt1 == 2){
      rsmp_net_info.basic_val = 0x00;
    }
    break;
  } // case 0

  case 1:
  {
    rsmp_net_info.cnsmod_val = opt1;
    if(-1 != opt2)
      rsmp_net_info.cgreg_val = opt2;
    if(-1 != opt3)
      rsmp_net_info.cereg_val = opt3;

    //If no status info is about to be uploaded, rsmp_net_info.basic_val may be wrong. (Be 0x01 but should be 0x02.)
    if((rsmp_net_info.cnsmod_val == 8 && (opt3 == 1 ||opt3 == 5)) 
      || (rsmp_net_info.cnsmod_val != 8 && (opt2 == 1 ||opt2 == 5)) )
    {
      if(rsmp_net_info.basic_val == 0x01)
        rsmp_net_info.basic_val = 0x02;
    }

    break;
  }// case 1

  case 2:
  {
    //Note:
    //Failed dial-up doesn't need update net_info structure.
    if(opt1 != 0x01){
      LOG("ERROR: Wrong param input. tag=%d, opt1=%d.\n", tag, opt1);
    return;
    }

    rsmp_net_info.basic_val = 0x04;
    break;
  } // case 2

  default:
  {
    LOG("ERROR: Wrong param input. tag=%d.\n", tag);
    break;
  } // default
  }
}

//Note:
//	If macro FEATURE_ENABLE_STATUS_INFO_UPLOAD_BY_MDMEmodem_ONLY is not defined, please
//	acquire ChannelLock when you call this function!
static void sock_net_info_processor(char pre_cond, Net_Info cur_ni)
{
  char cur_cond = cur_ni.basic_val;

  if(pre_cond < 0x00 || pre_cond > 0x05){
    LOG("ERROR: Wrong param input. pre_cond=%d.\n", pre_cond);
    return;
  }

  if(pre_cond != cur_cond)
  {
#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
    StatInfReqNum++;
#endif
    get_dev_and_net_info_for_status_report_session();

    trigger_rsmp_status_report();
  }
}

void net_info_processor(char pre_cond, Net_Info cur_ni)
{
  sock_net_info_processor(pre_cond, cur_ni);
}

//Param:
//	option=0: read rx bytes.
//	option=1: read tx bytes.
//Return Values:
//	>0: The num of rx/tx bytes.
//	-1: Wrong param input.
//	-2: open() failed.
//	-3: read() failed.
static long long int read_net_traffic_info(int option) 
{
	char buf[80] = {0}, *file_path = NULL;
	int len, fd;

	switch(option){
		case 0: file_path = NET_RX_FILE; break;
		case 1: file_path = NET_TX_FILE; break;
		default:{
			LOG("ERROR: Wrong param input. option=%d.\n", option);
			return -1;
		}
	}
	
	fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		LOG("ERROR: Fail to open %s. errno=%s(%d).\n", file_path, strerror(errno), errno);
		return -2;
	}
	
    len = read(fd, buf, sizeof(buf)-1);
	if (len < 0) {
		LOG("ERROR: read() failed. errno=%s(%d).\n", strerror(errno), errno);
        close(fd);
        return -3;
    }

  close(fd);

  buf[len] = '\0';

  return atoll(buf);
}

//Function:
//Monitor a fd if it is ready to read.(extensable)
static void FdListener(const int fd, evt_cb cb_func, void *userdata)
{
  int ret = -1;
  fd_set rfd;
  
  if(fd < 0){
    LOG("ERROR: Wong param input. fd=%d.\n", fd);
    err_num_s = -4;
    return;
  }

  FD_ZERO(&rfd);
  FD_SET(fd, &rfd);
__SELECT_AT_FD_AGAIN__:
  ret = select(fd+1, &rfd, NULL, NULL, NULL);
  if(ret > 0){
    if(cb_func != NULL)
      cb_func(userdata);
  }else{
    LOG("ERROR: select() error. ret=%d.\n", ret);
    goto __SELECT_AT_FD_AGAIN__;
  }

  if(cb_func == NULL)
    err_num_s = 1;
  return;
}

//Function:
//	open() a path and acquire a fd. Then send "ATE0" to this fd.
//Return Value:
//	1: Success
//	-1: open() failed.
//	-2: Sending AT cmd failed.
//	-3: FdListener() failed.
//	-4: read() failed.
//	-5: Wrong param input. path=NULL.
static int initATChannel(int *p_fd, const char *path)
{
  if(path == NULL){
    LOG("Wrong param input. path=null.\n");
    return -5;
  }

  *p_fd = open(path, O_RDWR | O_NONBLOCK | O_NOCTTY);
  if(*p_fd < 0){
    LOG("ERROR: open() failed. errno=%d.\n", errno);
    return -1;
  }

  pthread_mutex_lock(&at_mutex);
  send_at_and_check_response(*p_fd, "ATE0\r\n");
  pthread_mutex_unlock(&at_mutex);

  return 1;
}

static void init_URC_array(void)
{
  memset(URC_array, 0, sizeof(URC_array));
  URC_array_len = 0;
}

static void init_valid_URC_prefix_array(void)
{
  memset(valid_URC_prefix_array, 0, sizeof(valid_URC_prefix_array));
  valid_URC_prefix_array_len = 0;
}

//Description:
//Parse out raw data in read buffer, and put them into URC_array by one element for 
//one URC report.
//Return Values:
// 1 : Process succeeded.
// 0 : Process terminated for running out of URC_array space.
// -1 : Wrong parameter input.
// -2 : Wrong URC report recved. E.x. URC report is too big!
static int parse_raw_URC_data(char *buf, int read_bytes)
{
  char *cur = NULL, buf_idx = 0, orig_URC_array_len = URC_array_len;

  if(buf == NULL){
    LOG("ERROR: Wrong param input. buf=null.\n");
    return -1;
  }
  
  if(read_bytes == 0){
    return 1;
  }

  for(cur=buf;;)
  {
    char *URC_head = NULL, *URC_tail = NULL;

    if(*urc_fragment != 0x00)
    {
			LOG("There is something unread in urc_fragment.\n");
			
			if(MAX_URC_ARRAY_LEN < URC_array_len+1){
				LOG("Unable to save more URCs into URC array now! %d URCs has been saved already.\n",
						URC_array_len-orig_URC_array_len);
				URC_buf_break_point = URC_head;
				return 0;
			}
			
			if(*cur != '\r' && *cur != '\n' && *cur != '\0'){
				int already_saved_len = 0;
				
				URC_head = cur;
				for(; (*cur!='\r' && *cur!='\n' && *cur!='\0') && buf_idx<read_bytes; cur++, buf_idx++);
				if(read_bytes == buf_idx){
					LOG("ERROR: Wrong URC report recved. URC report is too big!\n");
					return -2;
				}
				URC_tail = cur;
				*URC_tail = '\0';
				URC_array_len ++;
				memset(URC_array[URC_array_len-1], 0, sizeof(URC_array[0]));
				memcpy(URC_array[URC_array_len-1], urc_fragment, sizeof(urc_fragment));
				already_saved_len = strlen(URC_array[URC_array_len-1]);
				memcpy(URC_array[URC_array_len-1]+already_saved_len, 
							  URC_head, 
							  (URC_tail-URC_head+already_saved_len>MAX_URC_LEN)?(MAX_URC_LEN-already_saved_len-1):(URC_tail-URC_head) 
							  );
				memset(urc_fragment, 0, sizeof(urc_fragment)); // Initialize urc_fragment
			}else{
				//Situation:
				//Last URC fragment is breaked between last valid char and line break chars.
				
				URC_array_len ++;
				memset(URC_array[URC_array_len-1], 0, sizeof(URC_array[0]));
				memcpy(URC_array[URC_array_len-1], urc_fragment, sizeof(urc_fragment));
				
				memset(urc_fragment, 0, sizeof(urc_fragment)); // Initialize urc_fragment
			}
		}

    for(; (*cur=='\r' || *cur=='\n' || *cur=='\0') && buf_idx<read_bytes; cur++, buf_idx++);
    if(read_bytes == buf_idx)
    {
      memset(urc_fragment, 0, sizeof(urc_fragment));
      break;
    }

    URC_head = cur;
    for(; (*cur!='\r' && *cur!='\n' && *cur!='\0') && buf_idx<read_bytes; cur++, buf_idx++);
    if(read_bytes == buf_idx)
    {
      memset(urc_fragment, 0, sizeof(urc_fragment));
      memcpy(urc_fragment, URC_head, cur-URC_head);
      LOG("NOTICE: Recv URC frag. Content: \"%s\".\n", urc_fragment);
      break;
    }else
    {
      URC_tail = cur;
      *URC_tail = '\0';
      if(MAX_URC_ARRAY_LEN < URC_array_len+1)
      {
        LOG("Unable to save more URCs into URC array now! %d URCs has been saved already.\n",
                                      URC_array_len-orig_URC_array_len);
        URC_buf_break_point = URC_head;
        return 0;
      }
      URC_array_len ++;
      memset(URC_array[URC_array_len-1], 0, sizeof(URC_array[0]));
      memcpy(URC_array[URC_array_len-1], URC_head, URC_tail-URC_head);
      continue;
    }
  }

  return 1;
}

//Description:
//	Set the prefix of valid URC into valid_URC_prefix_array.
//Parameter:
//	pre:
//		It must be C-style string.(Ended with a null char)
//Return Values:
//	  1 : Succeed.
//	-1 : Wrong param input.
//	-2 : Valid URC prefix array is totally occupied.
//	-3 : valid_URC_prefix_array_len found.
static int set_valid_prefix_array(const char *pre)
{
	if(pre == NULL){
		LOG("ERROR: Wrong param input. pre=null.\n");
		return -1;
	}
	if(*pre == '\0'){
		LOG("ERROR: Wrong param input. *pre=0x00.\n");
		return -1;
	}
	if(valid_URC_prefix_array_len == MAX_URC_ARRAY_LEN){
		LOG("ERROR: No more space for storing new valid URC prefix.\n");
		return -2;
	}else if(valid_URC_prefix_array_len > MAX_URC_ARRAY_LEN || valid_URC_prefix_array_len < 0){
		LOG("ERROR: Wrong valid_URC_prefix_array_len(%d) found!\n",
				valid_URC_prefix_array_len);
		return -3;
	}
	
	valid_URC_prefix_array_len += 1;
	memset(valid_URC_prefix_array[valid_URC_prefix_array_len-1], 0, sizeof(valid_URC_prefix_array[valid_URC_prefix_array_len]));
	memcpy(valid_URC_prefix_array[valid_URC_prefix_array_len-1], pre, strlen(pre));
	return 1;
}

//Description:
//		Check URC_array refer to valid_URC_prefix_array if the element of URC_array is valid. If yes, the corresponding element of
//	URC_valid_mark_array will be set 0x01. Otherwise, the corresponding element of URC_valid_mark_array will be maintained 0x00.
//		Check URC_array to look for the monitored URC specified in special_URC_array.
//Return Value:
//	>=0 : The number of valid URCs in URC array.
//	  -1  : Wrong param input.
static int check_valid_URC_array_elem(char (*valid_URC_pre_arr)[MAX_URC_LEN], const int valid_URC_pre_arr_len)
{
  int idx = 0, valid_URC_num = 0;

  if(valid_URC_pre_arr == NULL){
    LOG("ERROR: Wrong param input. valid_URC_prefix_array=null.\n");
    return -1;
  }
  
  if(valid_URC_pre_arr_len == 0){
    LOG("ERROR: Wrong param input. valid_URC_num=0.\n");
    return -1;
  }
  
  for(; valid_URC_pre_arr[idx]!=NULL && valid_URC_pre_arr[idx][0]!='\0' && idx<valid_URC_pre_arr_len; idx++);
  if(idx != valid_URC_pre_arr_len){
      LOG("ERROR: Wrong param input. Empty element (idx:%d) of valid_URC_prefix_array found! valid_URC_num=%d.\n", 
                  idx, valid_URC_pre_arr_len);
    return -1;
  }

  memset(URC_valid_mark_array, INVALID_URC, sizeof(URC_valid_mark_array));

  for( idx=0; idx<URC_array_len; idx++)
  {
    int valid_arr_idx = 0, spec_arr_idx = 0;
		
		for(; valid_arr_idx<valid_URC_pre_arr_len; valid_arr_idx++){
			int cmp_result = -1;
			
			cmp_result = strncmp(valid_URC_pre_arr[valid_arr_idx], URC_array[idx], strlen(valid_URC_pre_arr[valid_arr_idx]));
			if(0 == cmp_result){
				URC_valid_mark_array[idx] = VALID_URC;
				valid_URC_num ++;
				break;
			}
		}
		
		for(;spec_arr_idx<monitored_special_URC_num; spec_arr_idx++){
			if( 0 == strncmp(special_URC_array[spec_arr_idx], URC_array[idx], strlen(special_URC_array[spec_arr_idx])) )
				URC_special_mark_array[spec_arr_idx]++; //Use "++" rather than "+= 1". Maybe, the rpt times of some URCs is monitored.
		}

    //Tips:
    //Not recommended code below. Not compatible with the method of checking valid URC. We should input special_URC_array addr
    //as arg rather than use global var directly.

    //Handle URC rpt "+SIMCARD: NOT AVAILABLE"
    if(0 != URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_PWR_DWN])
    {
      LOG("\"+SIMCARD: NOT AVAILABLE\" has been recved!\n");
	  
#ifdef FEATURE_ENABLE_REBOOT_DEBUG
      if(isAutoReprotPowerDown == false)
      {
      		LOG("ERROR: auto report +SIMCARD: NOT AVAILABLE \n");
		//yy_popen_call("reboot");
		yy_popen_call("reboot -f");
		
      }
#endif

#ifdef FEATURE_ENABLE_SYSTEM_RESTORATION
     // notify_RstMonitor(0x00);
      isUimPwrDwnMssage = true;
      LOG("~~~~ start monitor 02 isUimPwrDwnMssage :=%d\n",isUimPwrDwnMssage);
#endif

      pthread_mutex_lock(&pd_mtx);

      isUimPwrDwn = true;
      isSimCardNotAvailRecved = true;
      isMdm9215CpinReady = false;

      if(true == PowerUpUsimAfterRecvingUrcRpt)
      {
        LOG("PowerUpUsimAfterRecvingUrcRpt = 1.\n");
        PowerUpUsimAfterRecvingUrcRpt = false;

        upload_status_info_and_power_up_virt_USIM_later();

        //After handling this URC rpt, the corresponding elem in URC_special_mark_array should be cleared.
        URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_PWR_DWN] = 0x00;
      }

      pthread_mutex_unlock(&pd_mtx);
    }

    //Handle URC rpt "+SIMCARD: SESSION ILLEGAL"
    //Note:
    //PowerUpUsimAfterRecvingUrcRpt must be false when "+SIMCARD: NOT AVAILABLE" is recved later, so no redundant
    //powerup will be exec.
    if(0 != URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_SESSION_ILLEGAL])
    {
      LOG("\"+SIMCARD: SESSION ILLEGAL\" has been recved!\n");

      if(!isRstingUsim)
      {
        isRstingUsim = true;

        isSessIllegalRecved = true;

        //Set LPM, and set uim powerdown
        sock_update_reg_urc_state(REG_URC_STATE_IDLE);
        pthread_mutex_lock(&rst_uim_mtx);
        
#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        msq_send_qlinkdatanode_run_stat_ind(6);
#endif
        proto_update_flow_state(FLOW_STATE_RESET_UIM);
        notify_DmsSender(0x04);
        notify_UimSender(0x00);
        while(isUimRstOperCplt == false){
          pthread_cond_wait(&rst_uim_cond, &rst_uim_mtx);
        }
        isUimRstOperCplt = false;
        pthread_mutex_unlock(&rst_uim_mtx);

        //Stat inf upload and ID auth
        isNeedToSendIDMsg = true;
        LOG("Now, go to recover mdmEmodem.\n");
        pthread_mutex_lock(&evtloop_mtx);
        push_ChannelNum(5);
        notify_EvtLoop(0x02);
        pthread_mutex_unlock(&evtloop_mtx);

      //URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_SESSION_ILLEGAL] will be resetted after powering up USIM
      }else{
        LOG("NOTICE: USIM rst is ongoing! So ignore this URC rpt.\n");
      }

      //If USIM rst is ongoing, the reception of this URC rpt is dumped. 
      //If not, after handling this URC rpt, the corresponding elem in URC_special_mark_array should be cleared.
      URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_SESSION_ILLEGAL] = 0x00;
    } // "+SIMCARD: SESSION ILLEGAL" recved
  }

  return valid_URC_num;
}

//Param:
//str:
//A C-style string.
static int count_comma_num(const char *str)
{
	int num = 0, idx = 0;
	
	//No examination of validity of arg str
	for(; idx<strlen(str); idx++){
		if(str[idx] == ',')
			num++;
	}
	
	return num;
}

//Param:
//data:
//It must point to a char of number and be ended with a comma char.
static int acq_param_val_of_URC_str(char *data)
{
	int val = 0, idx = 0;
	
	//No examination of validity of arg data
	for(; data[idx]!=',' && data[idx]!='\r' && data[idx]!='\n' && data[idx]!='\0'; idx++){
		val *= 10;
		val += data[idx] - '0';
	}
	
	return val;
}

//Notice:
//Not set any status variable inside. It simply identify the type of URC report.
static Reg_URC_Id_E_Type identify_reg_URC(const char *rpt)
{
  Reg_URC_Id_E_Type id = REG_URC_UNKNOWN;
  int cmp_result1 = -1, cmp_result2 = -1;

  LOG("  GET URC   <-- \"%s\"  \n", rpt);

  if(rpt == NULL){
    LOG("ERROR: Wrong param input. rpt=null.\n");
    return REG_URC_MAX;
  }
  
  if(*rpt == '\0'){
    LOG("ERROR: Wrong param input. *rpt=0x00.\n");
    return REG_URC_MAX;
  }

  if(*rpt != '+'){
    //Not a URC report
    LOG("ERROR: Wrong param input. *rpt != '+'.\n");
    return id;
  }
  
  if((cmp_result1 = strncmp("+CEREG:", rpt, strlen("+CEREG:"))) == 0 || 
    (cmp_result2 = strncmp("+CGREG:", rpt, strlen("+CGREG:"))) == 0
  ){
    bool isATCmdRspRecved = false;

    if(cmp_result1 == 0){
      if(count_comma_num(rpt) == AT_CEREG_EXT_RSP_COMMA_NUM){
        id = REG_URC_UNKNOWN;
        isATCmdRspRecved = true;
      }
    }else{
      if(count_comma_num(rpt) == AT_CGREG_EXT_RSP_COMMA_NUM){
        id = REG_URC_UNKNOWN;
        isATCmdRspRecved = true;
      }
    }

    if(!isATCmdRspRecved)
    {
      char *p_val = NULL, val = -1;
      p_val = strchr(rpt, ':') + 2;
      val = acq_param_val_of_URC_str(p_val);
 
      switch(val)
      {
      case 0:
        id = REG_URC_REG_FAIL;break;
      case 1:
      {
        if(cmp_result1 == 0)
        {
          id = REG_URC_LTE_REG;
          process_Imodem_CEREG_lac_and_cellid(&rsmp_dev_info , p_val);
        }
        else
        {
          id = REG_URC_REG;
          process_Imodem_CGREG_lac_and_cellid(&rsmp_dev_info , p_val);
        }
        break;
      }
      case 2:
        id = REG_URC_REG_SEARCHING;break;
      case 3:
        id = REG_URC_REG_REJ_V03;break;
      case 4:
        id = REG_URC_REG_UNKNOWN;break;
      case 5:
        id = REG_URC_REG_ROAMING;break;
      default:
        id = REG_URC_UNKNOWN;break;
      }
    }
  }else if((strncmp("+CPIN:", rpt, strlen("+CPIN:"))) == 0)
  {
    cmp_result1 = strncmp("+CPIN: READY", rpt, strlen("+CPIN: READY"));
    if(cmp_result1 == 0){
      id = REG_URC_CARD_READY;
    }else{
      id = REG_URC_CARD_ERROR;
    }
  }else if((strncmp("+REGREJ:", rpt, strlen("+REGREJ:"))) == 0)
  {
    char *p_val = NULL, val = -1;

    p_val = strchr(rpt, ',') + 1;
    val = acq_param_val_of_URC_str(p_val);
    rsmp_net_info.cregrej_val = val; // fix by xiaobin 20160425
    switch(val)
    {
    case 2:
    case 3:
    case 6:
    case 7:
    case 8:
      id = REG_URC_REG_REJ_V01;break;
    case 15:
      id = REG_URC_REG_REJ_V02;break;
    case 111:
      id = REG_URC_REG_REJ_V04;break;
    default:
      id = REG_URC_UNKNOWN;break;
    }
  }else if((strncmp("+CNSMOD:", rpt, strlen("+CNSMOD:"))) == 0){
    char *p_val = NULL, val = -1;

    p_val = strchr(rpt, ':') + 2;
    val = acq_param_val_of_URC_str(p_val);

#ifdef FEATURE_ENABLE_MSGQ
    msq_send_mdm9215_nw_sys_mode_ind(val);
#endif

    switch(val)
    {
    case 4:
    case 5:
    case 6:
    case 7:
      id = REG_URC_SYS_MODE_WCDMA;break;
    case 8:
      id = REG_URC_SYS_MODE_LTE;break;
    default:
      id = REG_URC_SYS_MODE_UNKNOWN;break;
    }
  }
#ifdef FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
  else if((strncmp("+RAB: REESTAB REJ", rpt, strlen("+RAB: REESTAB REJ"))) == 0)
  {
    id = REG_URC_RAB_REESTAB_REJ;
  }
  else if((strncmp("+RAB: REESTAB FAIL", rpt, strlen("+RAB: REESTAB FAIL"))) == 0)
  {
    id = REG_URC_RAB_REESTAB_FAIL;
  }
#endif //FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
  else
  {
    id = REG_URC_UNKNOWN;
  }

  return id;
}

//Description:
//Process the reg-related URC reports. Like "+CPIN: READY", "+CXREG: X,Y" and "+REGREJ: X,Y",etc.
//And it controls the state machine of reg_urc_state.
static void process_URC_array_v01(void)
{
  int idx = 0, preset_ChannelNum = -99;
  bool isRegRejUrcV01Recved = false; // Rej cause #2 #3 #6 #7 #8
  bool isCardErrRecved = false;
  bool isResume = false;

  if(URC_array_len == 0){
    LOG("ERROR: Nothing found in URC_array!\n");
    return;
  }

  for(; idx<URC_array_len; idx++)
  {
    Reg_URC_Id_E_Type URC_id = REG_URC_UNKNOWN;

    if(INVALID_URC == valid_URC_prefix_array[idx])
      continue;

    URC_id = identify_reg_URC(URC_array[idx]);
    LOG("reg_urc_state=%d, URC_id=%d.\n", reg_urc_state, URC_id);

 // add by jackli 20160810 
 // fix status change with 4 and 0. add by jackli 20161213
#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD   
	if ((URC_id == REG_URC_REG ) ||  (URC_id == REG_URC_LTE_REG ) ||  (URC_id == REG_URC_REG_ROAMING ))
	{
		g_qlinkdatanode_run_status_02 = REGIST_NET_OK;

		g_qlinkdatanode_run_status_03 = false;

		#ifdef FEATURE_ENABLE_EmodemUDP_LINK

		if ((memcmp(IP_addr, tcp_IP_addr, 15) == 0) &&  (server_port == tcp_server_port)){
			LOG("Debug: IP1 was use \n");
			g_isip1_use = true;
		}
		else if((memcmp(IP_addr, tcp_IP_addr_2, 15) == 0) && (server_port == tcp_server_port_2)){
			LOG("Debug: IP2 was use \n");
			g_isip2_use = true;

		}
		else{

			LOG("Debug: IP_default was use \n");
			/*g_enable_EmodemTCP_link = false;
			g_enable_TCP_IP01_linkcount = 3;
		  	g_enable_TCP_IP02_linkcount = 3;*/
		}
		  	
	 	#endif 
	}
	else 
	{
		g_qlinkdatanode_run_status_02 = REGIST_NET_ERROR;
	}
	
       LOG("g_qlinkdatanode_run_status_02: 0x%02x.\n", g_qlinkdatanode_run_status_02);
#endif
	
    switch(URC_id)
    {
    case REG_URC_REG:
    case REG_URC_LTE_REG:
    case REG_URC_REG_ROAMING:
    {
	//msq_send_qlinkdatanode_run_stat_ind(30); // 2016/12/07 by jack li

#ifdef FEATURE_ENABLE_CONTRL_EmodemREBOOT_0117DEBUG

               g_is7100modem_register_state = 2;
		 LOG("Debug: Receive CGREG OR CEREG:g_is7100modem_register_state = %d.\n", g_is7100modem_register_state);

#endif
		
      if(REG_URC_STATE_IDLE == reg_urc_state)
      {
        LOG("ERROR: \"+CPIN: READY\" not found yet.\n");
      }else if(REG_URC_STATE_CARD_READY == reg_urc_state
        || REG_URC_STATE_REGISTRATION_FAIL == reg_urc_state
        || REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111 == reg_urc_state
        || REG_URC_STATE_NOT_REGISTERED == reg_urc_state
      ){
        reg_urc_state = REG_URC_STATE_REGISTERED;
      }else if(REG_URC_STATE_REGISTRATION_REJECTED == reg_urc_state)
      {
        LOG("ERROR: Reg is rejected, but URC_id=%d.\n", URC_id);
      }else if(REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15 == reg_urc_state)
      {
        //Reg restored
        reg_urc_state = REG_URC_STATE_REGISTERED;
      }else{
        //Ignore this URC report
      }

      isRegOn = true;
      //Not update mdmImodem_ni.cond! It is updated in handle_reg_urc_state_var().

      break;
    }
    case REG_URC_CARD_ERROR:
    {
      LOG("ERROR: Card error found! Not support for handling this situation.\n");
      reg_urc_state = REG_URC_STATE_CARD_ERROR;
      isCardErrRecved = true;
      isRegOn = false;
      isMifiConnected = false;
      isMdm9215CpinReady = false;
        
#ifdef FEATURE_ENABLE_MSGQ
      msq_send_qlinkdatanode_run_stat_ind(5);
#endif

      clr_pdp_check_act_routine_timer();
      break;
    }
    case REG_URC_CARD_READY:
    {
      isMdm9215CpinReady = true;

 #ifdef FEATURE_ENABLE_REBOOT_DEBUG 
      g_is7100modemstartfail = 2;
      LOG("Debug: Receive cpinready from modem :g_is7100modemstartfail = %d.\n", g_is7100modemstartfail);
#endif
        
#ifdef FEATURE_ENABLE_MSGQ
      msq_send_qlinkdatanode_run_stat_ind(7);
      msq_send_mdm9215_usim_rdy_ind(1);
#endif

      if(REG_URC_STATE_IDLE == reg_urc_state)
      {
        reg_urc_state = REG_URC_STATE_CARD_READY;
        rsmp_net_info.basic_val = 0x01; // Add at 20150909

#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810
	g_qlinkdatanode_run_status_01 = CPIN_READY_OK;
       LOG("g_qlinkdatanode_run_status_01: 0x%02x.\n", g_qlinkdatanode_run_status_01);
#endif
        
#ifdef FEATURE_ENABLE_REPORT_FOR_CPINREDAY  // Add at 20160530  jackli
       LOG("Report cinpready status to server .\n");
	get_dev_and_net_info_for_status_report_session();
       need_to_upload_stat_inf = false;
	trigger_rsmp_status_report();
#endif

        break;
      }else{
        //Not possible to reach here
        //Once virt card has been powered down, reg_urc_state will be set REG_URC_STATE_IDLE

	#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810
		g_qlinkdatanode_run_status_01 = 0x00;
       	LOG("g_qlinkdatanode_run_status_01: 0x%02x.\n", g_qlinkdatanode_run_status_01);
	#endif
	break;
      }
	  


    } // case REG_URC_CARD_READY
    case REG_URC_REG_FAIL:
    {
      if(REG_URC_STATE_IDLE == reg_urc_state)
      {
        //Normal URC report, so ignore it
      }else if(REG_URC_STATE_CARD_READY == reg_urc_state
        || REG_URC_STATE_REGISTERED == reg_urc_state
        || REG_URC_STATE_NOT_REGISTERED == reg_urc_state
      ){
        reg_urc_state = REG_URC_STATE_REGISTRATION_FAIL;
      }else if(REG_URC_STATE_REGISTRATION_FAIL == reg_urc_state)
      {
        //Redundant URC report
      }else if(REG_URC_STATE_REGISTRATION_REJECTED == reg_urc_state
        || REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15 == reg_urc_state
        || REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111 == reg_urc_state) // AT+CFUN=4 may cause "+CXREG: 0"
      {
        //Normal URC report, so ignore it
      }

      isRegOn = false;
      isMifiConnected = false;
      clr_pdp_check_act_routine_timer();

      break;
    } // case REG_URC_REG_FAIL
    case REG_URC_REG_REJ_V01:
    {
				if(REG_URC_STATE_IDLE == reg_urc_state){
					//Not possible to reach here, but the logic is reasonable
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
				}else if(REG_URC_STATE_CARD_READY == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
				}else if(REG_URC_STATE_REGISTERED == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
				}else if(REG_URC_STATE_REGISTRATION_FAIL == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
				}else if(REG_URC_STATE_REGISTRATION_REJECTED == reg_urc_state){
					//Not possible to receive another same reg rejection URC
				}else if(REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15 == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
					//No need to reset isRegRejUrcV02Recved
				}else if(REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111 == reg_urc_state){
					//That shift local virt card is prior to solve rej code #111.
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
				}else if(REG_URC_STATE_NOT_REGISTERED == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
				}
				
				isRegRejUrcV01Recved = true;
				isRegOn = false;
				isMifiConnected = false; // Add by rjf at 20150831

				clr_pdp_check_act_routine_timer();
				
				break;
    } // case REG_URC_REG_REJ_V01
    case REG_URC_REG_REJ_V02:
    {
				if(REG_URC_STATE_IDLE == reg_urc_state){
					//Not possible to reach here, but the logic is reasonable
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
				}else if(REG_URC_STATE_CARD_READY == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
				}else if(REG_URC_STATE_REGISTERED == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
				}else if(REG_URC_STATE_REGISTRATION_FAIL == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
				}else if(REG_URC_STATE_REGISTRATION_REJECTED == reg_urc_state){
					LOG("ERROR: Wrong reg rej rpt order!\n");
					goto __EXIT_OF_CASE_REG_URC_REG_REJ_V02__;
				}else if(REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15 == reg_urc_state){
					//Redundant URC report
				}else if(REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111 == reg_urc_state){
					//Which one ("+REGREJ: 0,111" or "+REGREJ: 0,15") is shown up laterly, that one is prior to be handled.
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
				}else if(REG_URC_STATE_NOT_REGISTERED == reg_urc_state){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
				}
				

      isRegOn = false;
      isMifiConnected = false;
      clr_pdp_check_act_routine_timer();

__EXIT_OF_CASE_REG_URC_REG_REJ_V02__:
      break;
    } // case REG_URC_REG_REJ_V02
    case REG_URC_REG_REJ_V03:
    {
      if(REG_URC_STATE_CARD_READY == reg_urc_state){
        //Not need to change reg_urc_state to REG_URC_STATE_NOT_REGISTERED
      }else if(REG_URC_STATE_REGISTERED == reg_urc_state){
        reg_urc_state = REG_URC_STATE_NOT_REGISTERED;
      }
      //REG_URC_STATE_NOT_REGISTERED is low-priority state.

      isRegOn = false;
      isMifiConnected = false;
      clr_pdp_check_act_routine_timer();

      //Not have specified reg_urc_state, so update ChannelNum in process_URC_array_v01 
      //rather than handle_reg_urc_state_var
      ACQ_CHANNEL_LOCK_WR;
      if(1 == ChannelNum || 4 == ChannelNum){
        if(1 == ChannelNum){
          preset_ChannelNum = -1;
          isResume = true;
        }else{
          ChannelNum = -1;
        }
      }else if(2 == ChannelNum){
        isTermSuspNow = true;
        RcvrMdmEmodemConnWhenSuspTerm = true;
      }else if(3 == ChannelNum){
        ChannelNum = 0;
      }
      //If ChannelNum = 0 or -1, nothing need to be done.
      RLS_CHANNEL_LOCK;;

      break;
    } // case REG_URC_REG_REJ_V03
    case REG_URC_REG_REJ_V04:
    {
				if(REG_URC_STATE_IDLE == reg_urc_state
					|| REG_URC_STATE_CARD_READY == reg_urc_state
					|| REG_URC_STATE_REGISTERED == reg_urc_state
					|| REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15 == reg_urc_state
					|| REG_URC_STATE_REGISTRATION_FAIL == reg_urc_state
					|| REG_URC_STATE_NOT_REGISTERED == reg_urc_state)
				{
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111;
				}
				//REG_URC_STATE_CARD_ERROR: When card status is error, not possible to get reg rej ind.
				//REG_URC_STATE_REGISTRATION_REJECTED: Once reg is rejected permenantly, app need to req a new virt vard.
				//REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111: Maintain reg_urc_state.
				
				isRegOn = false;
				isMifiConnected = false;
				clr_pdp_check_act_routine_timer();
				
				break;
    }// case REG_URC_REG_REJ_V04
    case REG_URC_REG_SEARCHING:
    case REG_URC_REG_UNKNOWN:
    { 
	//msq_send_qlinkdatanode_run_stat_ind(29); // 2016/12/07 by jack li
	// Note: Regard REG_URC_REG_UNKNOWN as registration fail off.
      if(REG_URC_STATE_CARD_READY == reg_urc_state){
        //Not need to change reg_urc_state to REG_URC_STATE_NOT_REGISTERED
      }else if(REG_URC_STATE_REGISTERED == reg_urc_state){
        reg_urc_state = REG_URC_STATE_NOT_REGISTERED;//REG_URC_STATE_NOT_REGISTERED is low-priority state.
      }
      
      if(REG_URC_REG_SEARCHING == URC_id &&
        (REG_URC_STATE_REGISTERED == reg_urc_state ||
        REG_URC_STATE_CARD_READY == reg_urc_state)
      ){
        rsmp_net_info.basic_val = 0x01;
      }

      isRegOn = false;
      isMifiConnected = false;
      clr_pdp_check_act_routine_timer();

      //Not have specified reg_urc_state, so update ChannelNum in process_URC_array_v01 
      //rather than handle_reg_urc_state_var
      ACQ_CHANNEL_LOCK_WR;
      if(0 == ChannelNum || 3 == ChannelNum)
      {
        if(3 == ChannelNum)
          ChannelNum = 0;

        //Fixed by rjf at 20150810 (Only recover mdmEmodem when registration succeeded before.)
        if(REG_URC_STATE_REGISTERED == last_reg_urc_state){
          //Not check if mdmEmodem is busy here. It will be done in RecvCallback().
          preset_ChannelNum = -1;
          isResume = true;
        }
      }else if(1 == ChannelNum || 4 == ChannelNum)
      {
        //Note: That ChannelNum equals 1 or 4 means registration succeeded before.
        if(1 == ChannelNum){
          preset_ChannelNum = -1;
          isResume = true;
        }else{
          ChannelNum = -1;
          isTermRcvrNow = true;
        }
      }else if(2 == ChannelNum)
      {
        //Note: That ChannelNum equals 2 means registration succeeded before.
        isTermSuspNow = true;
        RcvrMdmEmodemConnWhenSuspTerm = true;
      }else
      { // -1 == ChannelNum
        //Fixed by rjf at 20150810 (Only recover mdmEmodem when registration succeeded before.)
        if(REG_URC_STATE_REGISTERED == last_reg_urc_state)
          isTermRcvrNow = true;
      }
      RLS_CHANNEL_LOCK;

      break;
    } // case REG_URC_REG_SEARCHING
    case REG_URC_SYS_MODE_WCDMA:
    case REG_URC_SYS_MODE_LTE:
    case REG_URC_SYS_MODE_UNKNOWN:
    {
      last_reg_sys_mode_state = reg_sys_mode_state;
      if(REG_URC_SYS_MODE_WCDMA == URC_id)
        reg_sys_mode_state = REG_SYS_MODE_WCDMA;
      else if(REG_URC_SYS_MODE_LTE == URC_id)
        reg_sys_mode_state = REG_SYS_MODE_LTE;
      else
        reg_sys_mode_state = REG_SYS_MODE_UNKNOWN;

      break;
    } // case REG_URC_SYS_MODE_*
#ifdef FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
    case REG_URC_RAB_REESTAB_REJ:
    case REG_URC_RAB_REESTAB_FAIL:
      if(isMifiConnected && 1 != sock_inet_conn_test())
      {
        rab_restab_rej_or_fail_counter++;
      }
      break;
#endif //FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
    default:
      //Not handle these URCs now
      break;
    }

    LOG("ChannelNum = %d.\n", ChannelNum);

    if(true == isCardErrRecved ||
      true == isRegRejUrcV01Recved
    ){
      break;
    }
  }//for
 
   
   LOG("Debug:   isResume = %d \n",isResume);
  //Resume mdmEmodem
  if(true == isResume) {
    LOG("Now, go to resume or recover mdmEmodem.\n");
    pthread_mutex_lock(&evtloop_mtx);
    push_ChannelNum(preset_ChannelNum==-1?5:preset_ChannelNum);
    notify_EvtLoop(0x02);
    pthread_mutex_unlock(&evtloop_mtx);
  }

  return;
}

//Return Value:
// 1 or 0 : It is if need to process subsequent unread URCs in buffer.
// -1 : Error
static int handle_reg_urc_state_var(void)
{
  int ret = -1, preset_ChannelNum = -99;
  bool isResume = false;

  if(isMdm9215CpinReady)
  {
    rsmp_net_info.cpin_val = 0x01;
  }

  if(reg_urc_state == last_reg_urc_state){
    goto __EXIT_OF_HANDLE_REG_URC_STATE_VAR__;
  }

__HANDLE_REG_URC_STATE__:
  LOG("last_reg_urc_state=%d,reg_urc_state=%d, ChannelNum=%d.\n", last_reg_urc_state, reg_urc_state, ChannelNum);
  switch(reg_urc_state)
  {
  case REG_URC_STATE_CARD_ERROR:
  case REG_URC_STATE_REGISTRATION_REJECTED:
  {
    isMifiConnected = false;
    if(isWaitToSusp == true){
      LOG("Now, cancel suspension for mdmEmodem.\n");
      isWaitToSusp = false;
    }
    
    //Update ChannelNum
    ACQ_CHANNEL_LOCK_WR;
    if(ChannelNum == 1 || ChannelNum == 4){
      if(ChannelNum == 1){
					preset_ChannelNum = -1;
					isResume = true;
				}else{
					LOG("ChannelNum change to -1 from %d.\n", ChannelNum);
					ChannelNum = -1;
				}
			}else if(ChannelNum == 2){
				isTermSuspNow = true;
				RcvrMdmEmodemConnWhenSuspTerm = true;
			}else if(ChannelNum == 3){
				LOG("ChannelNum change to 0 from 3.\n");
				ChannelNum = 0;
			}
			//If ChannelNum = 0 or -1, nothing need to be done.
			RLS_CHANNEL_LOCK;
			
			//Set LPM, and set uim powerdown
			if(!isRstingUsim){
				isRstingUsim = true;
				sock_update_reg_urc_state(REG_URC_STATE_IDLE); // Changed by rjf at 20150922
				pthread_mutex_lock(&rst_uim_mtx);
				#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
					msq_send_qlinkdatanode_run_stat_ind(6);
				#endif
				proto_update_flow_state(FLOW_STATE_RESET_UIM);
				notify_DmsSender(0x04);
				LOG("NOTICE: Wait 3s for LPM setting to take effect.\n");
				sleep(3); //Add by rjf at 20151026
				notify_UimSender(0x00);
				while(isUimRstOperCplt == false){
					pthread_cond_wait(&rst_uim_cond, &rst_uim_mtx);
				}
				isUimRstOperCplt = false; // reset
				pthread_mutex_unlock(&rst_uim_mtx);
			}else{
				LOG("NOTICE: USIM rst is ongoing!\n");
			}

			rsmp_net_info.basic_val = 0x03;
			if(rsmp_net_info.cnsmod_val == 0x08)
				rsmp_net_info.cereg_val = 0x03;
			else
				rsmp_net_info.cgreg_val = 0x03;
			
			//Prepare for uploading status info and ID authentication msg
			isNeedToSendIDMsg = true; // In order to update local USIM data.

			if(!isApduRspTimeout){
				need_to_upload_stat_inf = false;
				#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
					StatInfReqNum ++;
				#endif
				get_dev_and_net_info_for_status_report_session();
				trigger_rsmp_status_report();
			}else{
				LOG("isApduRspTimeout=1, so cancel status info upload.\n");
			}
			
			ret = 0;
			break;
		} // case REG_URC_STATE_CARD_ERROR and REG_URC_REG_REJ_V01
  case REG_URC_STATE_REGISTERED:// Include reg roaming
  { 
#ifdef FEATURE_ENABLE_MSGQ
    msq_send_mdm9215_reg_status_ind(1);
#endif // FEATURE_ENABLE_MSGQ

    pthread_mutex_lock(&i1r_mtx);
    if(is1stRunOnMdmEmodem == true)
    {
      init_dev_info(&rsmp_dev_info);
    }
    is1stRunOnMdmEmodem = false;
    pthread_mutex_unlock(&i1r_mtx);

    //Prepare for uploading status info and ID authentication msg
    if(REG_URC_STATE_REGISTERED != last_reg_urc_state && false == isApduRspTimeout)
    {
#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
      StatInfReqNum ++;
#endif
      get_dev_and_net_info_for_status_report_session();

      need_to_upload_stat_inf = false;

      trigger_rsmp_status_report();
    }
    else if(REG_URC_STATE_REGISTERED != last_reg_urc_state && true == isApduRspTimeout)
    {
      LOG("isApduRspTimeout=1, so cancel status info upload.\n");
      need_to_upload_stat_inf = false;
    }
    else
    {
      update_rsmp_net_info(); // No matter what values of pre_cnd and pre_mod are, check_RegStat() must be called.
    }

#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
    //Situation Presentation:
    //Usually, the state ACQ_DATA_STATE_RECVING will be ended when app has recved server rsp then.
    //And if no queued node exists and no more thing comes, the state will be changed to ACQ_DATA_STATE_WAITING.
    //So the acq_data_state ACQ_DATA_STATE_RECVING is seen that mdmEmodem is busy at that time (busy recving rsp).
    //isNotNeedIdleTimer is set at beginning of executing qlinkdatanode. So it is impossible to reach here.
    if(0 == check_rsp_timers() 
      && 0 == mdmEmodem_check_busy()
      && false == isQueued
      && false == isSockListenerRecvPdpActMsg
    ){
      //If idle timer exists, not add idle timer here.
      LOG("Set up idle timer for mdmEmodem.\n");
      add_timer_mdmEmodem_idle_timeout(0x01); // Caller is ListenerLoop. So need to trigger EvtLoop to add this timer in.
    }
#endif // FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
            
      break;
    }  // case REG_URC_STATE_REGISTERED
    case REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15:
    {
      //Update ChannelNum
      ACQ_CHANNEL_LOCK_WR;
      switch(ChannelNum)
      {
      case 1:
      {
        preset_ChannelNum = -1;
        isResume = true; // Restore mdmEmodem to transmit APDUs.
        break;
      } // case 1
      case 3:
      {
        LOG("ChannelNum changed to 0 from 3.\n");
        //Situation Presentation:
        //Although mdm9215 reg is rejected for cause #15, mdmEmodem is free from reg var. So network link of mdmEmodem
        //is intact.
        ChannelNum = 0;
        break;
      } // case 3
      case 4:
      {
        LOG("ChannelNum changed to -1 from %d.\n", ChannelNum);
        ChannelNum = -1;
        break;
      } // case 4
      case 2:
      {
        isTermSuspNow = true;
        RcvrMdmEmodemConnWhenSuspTerm = true;
        break;
      }
      default: // -1 or 0
        break;
      }
      RLS_CHANNEL_LOCK;

#ifdef FEATURE_SET_3G_ONLY_ON_REJ_15
      //rej by lte, set umts only
       LOG("isneedtoset4G3Gauto =%d.\n", isneedtoset4G3Gauto);
      if (!isneedtoset4G3Gauto)
      {
		pthread_mutex_lock(&acq_nas_inf_mtx);
      		notify_NasSender(0x04);
     	 	while(0 == nas_inf_acqed){
        	pthread_cond_wait(&acq_nas_inf_cnd, &acq_nas_inf_mtx);
     	 	}
     	 	nas_inf_acqed = 0; // Reset it for using it again
     	 	pthread_mutex_unlock(&acq_nas_inf_mtx);
     		 isUMTSOnlySet = true;
      	}
			 
#endif //FEATURE_SET_3G_ONLY_ON_REJ_15

      break;
    } // case REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15
    case REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111:
    {
      ACQ_CHANNEL_LOCK_WR;
			switch(ChannelNum){
				case 1:{
					preset_ChannelNum = -1;
					isResume = true; // Restore mdmEmodem to transmit APDUs after reset RF part of modem.
					break;
				}
				case 2:{
					isTermSuspNow = true;
					RcvrMdmEmodemConnWhenSuspTerm = true;
					break;
				}
				case 3:{
					LOG("ChannelNum changed to 0 from 3.\n");
					ChannelNum = 0;
					break;
				}
				case 4:{
					LOG("ChannelNum changed to -1 from %d.\n", ChannelNum);
					ChannelNum = -1;
					break;
				}
				default: // -1 or 0
					break;
			}
			RLS_CHANNEL_LOCK;

#ifdef FEATURE_SET_3G_ONLY_ON_REJ_15
      pthread_mutex_lock(&acq_dms_inf_mtx);
      notify_DmsSender(0x02);
      while(0 == dms_inf_acqed){
        pthread_cond_wait(&acq_dms_inf_cnd, &acq_dms_inf_mtx);
      }
      dms_inf_acqed = 0;
      pthread_mutex_unlock(&acq_dms_inf_mtx);

      isUMTSOnlySet = (14 == rsmp_net_info.cnmp_val);
  
      if(isUMTSOnlySet)
      {
        pthread_mutex_lock(&acq_nas_inf_mtx);
        notify_NasSender(0x03);
        while(0 == nas_inf_acqed){
          pthread_cond_wait(&acq_nas_inf_cnd, &acq_nas_inf_mtx);
        }
        nas_inf_acqed = 0; // Reset it for using it again
        pthread_mutex_unlock(&acq_nas_inf_mtx);
        isUMTSOnlySet = false;
      }
#endif // FEATURE_SET_3G_ONLY_ON_REJ_15

      //Not update flow_state. At that time, APDUs transmission should not be blocked.
      pthread_mutex_lock(&rst_rf_mtx);
      isRstRf = true;
      notify_DmsSender(0x04);//AT+CFUN=4
      while(isRfRsted == false){
        pthread_cond_wait(&rst_rf_cond, &rst_rf_mtx);
      }
      isRfRsted = false; // reset
      sleep(3); // Wait 3s for LPM to take effect
      notify_Sender(QMISENDER_SET_ONLINE);
      pthread_mutex_unlock(&rst_rf_mtx);

      break;
    } // case REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_111
    case REG_URC_STATE_REGISTRATION_FAIL:
    {
      //Once APDU rsp timeout, not need to do anything when reg-fail URC rpts are recved,
      //including sending ID msg and status info, power down USIM, etc.
      if(!isApduRspTimeout)
      {
        int rej_cause = -1, mod = -1, rssi = -1;

        //Check if reg rejected
        rej_cause = update_rsmp_cregrej_value();
				LOG("update_rsmp_cregrej_value return %d.\n", rej_cause);
				if(2 == rej_cause || 
					3 == rej_cause || 
					6 == rej_cause ||
					7 == rej_cause ||
					8 == rej_cause
				){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED;
					// ChannelNum will be updated in case of REG_URC_STATE_REGISTRATION_REJECTED
					goto __HANDLE_REG_URC_STATE__;
				}else if(15 == rej_cause){
					reg_urc_state = REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15;
					// ChannelNum will be updated in case of REG_URC_STATE_REGISTRATION_REJECTED_CAUSE_15
					goto __HANDLE_REG_URC_STATE__;
				}

				//Check if reg has really fail off
				mod = mdm9215_check_CNSMOD();
				LOG("mdm9215_check_CNSMOD() return %d.\n", mod);
				if(mod != 0){
					rssi = mdm9215_check_CSQ();
					LOG("mdm9215_check_CSQ() return %d.\n", rssi);
					if(rssi > CSQ_VAL_THRESHOLD_OF_POOR_SIG && rssi != 99){
						//Regard it as normal situation
						LOG("NOTICE: The signal environment is fine. Restore the original reg_urc_state.\n");
						reg_urc_state = last_reg_urc_state; // Fixed by rjf at 20150813
						break;
					}
					//Regard it as poor signal situation
				}
				
				//Update ChannelNum
				//Change relative position of "Update ChannelNum" and "Check if reg has really fail off" at 20151015
				ACQ_CHANNEL_LOCK_WR;
				switch(ChannelNum)
        {
					case 0:
					case 3:{
						if(3 == ChannelNum){
							LOG("ChannelNum changed to 0 from 3.\n");
							ChannelNum = 0;
						}
						
						//Note:
						//The registration of mdm9215 fell off, so we assumed the same thing happened on mdmEmodem.
						//For that, we need to recover the server connection of mdmEmodem.
						
						//Not check if mdmEmodem is busy here. It will be done in RecvCallback().
						preset_ChannelNum = -1;
						isResume = true;
						
						break;
					} // case 0 or 3
					case 1:
					case 4:{
						if(4 == ChannelNum){
							LOG("ChannelNum changed to -1 from 4.\n");
							ChannelNum = -1;
							isTermRcvrNow = true;
						}else{
							preset_ChannelNum = -1;
							isResume = true;
						}
						
						break;
					} // case 1 or 4
					case 2:{
						isTermSuspNow = true;
						RcvrMdmEmodemConnWhenSuspTerm = true;
						break;
					} // case 2
					default:{ // -1 or 0
						if(-1 == ChannelNum){
							isTermRcvrNow = true;
						}else{
							preset_ChannelNum = -1;
							isResume = true;
						}
						
						break;
					} // default
				}
				RLS_CHANNEL_LOCK;

				rsmp_net_info.basic_val = 0x01; //It is sure that it is not a rejection.
				
				//Update some status variables
				isRegOffForPoorSig = true;
				
				//Fixed by rjf at 20150808 (Regard registration of mdmEmodem is falling off.)
				//Prepare for uploading status info and sending out ID authentication msg
				isNeedToSendIDMsg = true;
        
#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
        StatInfReqNum ++;
#endif // FEATURE_ENHANCED_STATUS_INFO_UPLOAD
        
        get_dev_and_net_info_for_status_report_session();

        trigger_rsmp_status_report(); //Send status info now or later

        need_to_upload_stat_inf = false;
				
				//Set LPM, and set uim powerdown
				if(!isRstingUsim){
					isRstingUsim = true;
					sock_update_reg_urc_state(REG_URC_STATE_IDLE); // Changed by rjf at 20150922
					pthread_mutex_lock(&rst_uim_mtx);
					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(6);
					#endif
					proto_update_flow_state(FLOW_STATE_RESET_UIM);
					notify_DmsSender(0x04);
					LOG("NOTICE: Wait 3s for LPM setting to take effect.\n");
					sleep(3); //Add by rjf at 20151026
					notify_UimSender(0x00);
					while(isUimRstOperCplt == false){
						pthread_cond_wait(&rst_uim_cond, &rst_uim_mtx);
					}
					isUimRstOperCplt = false;
					pthread_mutex_unlock(&rst_uim_mtx);
				}else{
					LOG("NOTICE: USIM rst is ongoing!\n");
				}
				
      ret = 0; //Check reg rej at the beginning.
    }

    break;
  } // case REG_URC_STATE_REGISTRATION_FAIL
  case REG_URC_STATE_CARD_READY: // mdmImodem_ni will be updated in the beginning.
  case REG_URC_STATE_NOT_REGISTERED:
  default:
    break;
  }

  //Resume mdmEmodem
  if(isResume){
    LOG("Now, go to resume or recover mdmEmodem.\n");
    pthread_mutex_lock(&evtloop_mtx);
    push_ChannelNum(preset_ChannelNum==-1?5:preset_ChannelNum);
    notify_EvtLoop(0x02);
    pthread_mutex_unlock(&evtloop_mtx);
  }

__EXIT_OF_HANDLE_REG_URC_STATE_VAR__:
  return ret;
}


//Return value:
// 1: Success
// -1: Wrong param found.
//Modication Record:
// 2015-8-11: Move the process of judging if send out request directly or not to RecvCallback().
static int trigger_rsmp_status_report(void)
{
  int length;
  Thread_Idx_E_Type cur_tid = get_thread_idx();
  rsmp_transmit_buffer_s_type *p_rsmp_tranamit_buffer = NULL;

  if(THD_IDX_MAX != cur_tid){
    p_rsmp_tranamit_buffer = &g_rsmp_transmit_buffer[cur_tid];
  }else{
    LOG("ERROR: Illegal tid found! cur_tid=%u.\n", cur_tid);
    p_rsmp_tranamit_buffer = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP]; //FIXME
  }

  pthread_mutex_lock(&evtloop_mtx);
  reset_rsmp_transmit_buffer(p_rsmp_tranamit_buffer);
  p_rsmp_tranamit_buffer->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_STATUS, &sim5360_dev_info, &rsmp_dev_info);
  p_rsmp_tranamit_buffer->size = length;
  p_rsmp_tranamit_buffer->StatInfMsgSn = StatInfMsgSn; // Add by rjf at 20151016
  push_flow_state(FLOW_STATE_STAT_UPLOAD);
  push_thread_idx(cur_tid);
  notify_EvtLoop(0x01);
  pthread_mutex_unlock(&evtloop_mtx);
  
}

//Description:
//It gonna acq cm sys mode and save it into mdmImodem_ni.mod_val.
int mdm9215_check_CNSMOD(void)
{
  pthread_mutex_lock(&acq_dms_inf_mtx);
  notify_DmsSender(0x00);
  while(0 == dms_inf_acqed){
    pthread_cond_wait(&acq_dms_inf_cnd, &acq_dms_inf_mtx);
  }
  dms_inf_acqed = 0; // Reset for using it again
  pthread_mutex_unlock(&acq_dms_inf_mtx);

  return rsmp_net_info.cnsmod_val;
}

int mdm9215_check_CSQ(void)
{
  pthread_mutex_lock(&acq_nas_inf_mtx);
  notify_NasSender(0x00);
  while(0 == nas_inf_acqed){
    pthread_cond_wait(&acq_nas_inf_cnd, &acq_nas_inf_mtx);
  }
  nas_inf_acqed = 0; // Reset it for using it again
  pthread_mutex_unlock(&acq_nas_inf_mtx);

  return rsmp_dev_info.sig_strength;
}

void update_rsmp_net_info(void)
{
  LOG6("~~~~ update_rsmp_net_info ~~~~\n");

  pthread_mutex_lock(&acq_dms_inf_mtx);
  notify_DmsSender(0x05);
  while(0 == dms_inf_acqed){
    pthread_cond_wait(&acq_dms_inf_cnd, &acq_dms_inf_mtx);
  }
  dms_inf_acqed = 0;
  pthread_mutex_unlock(&acq_dms_inf_mtx);
  update_net_info(0x01, rsmp_net_info.cnsmod_val, rsmp_net_info.cgreg_val, rsmp_net_info.cereg_val);

  if(((0x01 == rsmp_net_info.cereg_val || 0x05 == rsmp_net_info.cereg_val) && 8 == rsmp_net_info.cnsmod_val) 
    || ((0x01 == rsmp_net_info.cgreg_val || 0x05 == rsmp_net_info.cgreg_val) && 8 != rsmp_net_info.cnsmod_val) 
  ){
    isRegOn = true;
  }else{
    isRegOn = false;
    isMifiConnected = false;
  }
}

//Return Values:
// >0 : Normal rej cause.
// 0 : Unexpected ret val.
//-1 : Wrong param input.
//-6: sock_send_at_rd_cmd() failed.
//-7: Wrong content in unexp_urc_buf when processing unexpected URC report in recving at cmd rsp.
int update_rsmp_cregrej_value(void)
{
  pthread_mutex_lock(&acq_dms_inf_mtx);
  notify_DmsSender(0x03);
  while(0 == dms_inf_acqed){
    pthread_cond_wait(&acq_dms_inf_cnd, &acq_dms_inf_mtx);
  }
  dms_inf_acqed = 0; // Reset for using it again
  pthread_mutex_unlock(&acq_dms_inf_mtx);

  return rsmp_net_info.cregrej_val;
}

//Description:
//Acquire rssi, GCI(PLMN,TAC or LAC), data flux and battery info.
//Then u can call check_RegStat() to acq cnsmod val ,and cgreg val or cereg val.
static int acqMdmInfo(void)
{
  long long int rx, tx;

  pthread_mutex_lock(&acq_nas_inf_mtx);
  notify_NasSender(0x02);
  while(0 == nas_inf_acqed){
    pthread_cond_wait(&acq_nas_inf_cnd, &acq_nas_inf_mtx);
  }
  nas_inf_acqed = 0;
  pthread_mutex_unlock(&acq_nas_inf_mtx);

  rx = read_net_traffic_info(0);
  tx = read_net_traffic_info(1);
  rsmp_dev_info.flux[0] = (unsigned int)((rx + tx) & 0x00000000ffffffff);
  rsmp_dev_info.flux[1] = (unsigned int)((rx + tx) >> 32 & 0x00000000ffffffff);
  LOG("flux[0]: %u, flux[1]: %u.\n", rsmp_dev_info.flux[0], rsmp_dev_info.flux[1]);


  LOG("Debug: reg_urc_state = %d.\n", reg_urc_state);
  
  if ((reg_urc_state == REG_URC_STATE_CARD_READY) && ( b_enable_softwarecard_ok == true)){

	LOG("Debug: Due to the status is CPINREDAY,don't get battery power info . \n");
	return 1;
  }
  
#ifdef FEATURE_ENABLE_MSGQ
  pthread_mutex_lock(&pwr_inf_qry_mtx);
  msq_qry_pwr_inf();
  LOG("Wait for acquiring battery power info...\n");
  while(0 == pwr_inf_acqed){
    pthread_cond_wait(&pwr_inf_qry_cnd, &pwr_inf_qry_mtx);
  }
  pwr_inf_acqed = 0; // Reset
  pthread_mutex_unlock(&pwr_inf_qry_mtx);
#endif

  return 1;
}

void get_dev_and_net_info_for_status_report_session(void)
{
  LOG6("~~~~~ get_dev_and_net_info_for_status_report_session start ~~~~~\n");

  pthread_mutex_lock(&stat_inf_mtx);
  StatInfMsgSn++;
  acqMdmInfo();
  update_rsmp_net_info();
  pthread_mutex_unlock(&stat_inf_mtx);

  LOG6("~~~~~ get_dev_and_net_info_for_status_report_session end ~~~~~\n");
}

//Return Values:
// 1: The network link (of mdm9215) is available.
// 0: The network link (of mdm9215) is unavailable.
// -1: Internal error.
int sock_inet_conn_test(void)
{
  int sock_fd = -1, ret = -1, flag;
  struct sockaddr_in serv_addr;
  fd_set wfd, rfd;
  struct timeval tv;
  static bool  breboot_flag = false;
  static unsigned char  bruntimes = 0;
  static unsigned int  errortimes = 0;


  //Add this log in case that no log will be printed after calling pdp_act_check_routine_cb(), and it is seen like error.
  LOG6("~~~~ sock_inet_conn_test ~~~~\n");

  sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sock_fd < 0){
    LOG("ERROR: socket() failed. errno=%d.\n", errno);
    return -1;
  }

  if((flag = fcntl(sock_fd, F_GETFL, 0)) < 0){
    LOG("ERROR: fcntl() failed (F_GETFL). errno=%d.\n", errno);
    return -1;
  }
  flag |= O_NONBLOCK;
  if((fcntl(sock_fd, F_SETFL, flag)) < 0){
    LOG("ERROR: fcntl() failed (F_SETFL). errno=%d.\n", errno);
    return -1;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(80);
  ret = inet_pton(AF_INET, "112.80.248.74", &serv_addr.sin_addr.s_addr);
  if(ret == 0){
    LOG("ERROR: inet_pton() failed. Invalid address string found. IP_addr=%s.\n", IP_addr);
    return -1;
  }else if(ret < 0){
    LOG("ERROR: inet_pton() failed. errno=%d.\n", errno);
    return -1;
  }

  ret = connect(sock_fd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr));
  if(ret != 0){
    if(errno != EINPROGRESS){
      close(sock_fd);
      LOG("ERROR: connect() failed. errno=%d.\n", errno);
    if(errno == 101)
    {
    	bruntimes++;
	LOG("ERROR: connect() failed. errno == 101.bruntimes =%d.\n",bruntimes);
       if (bruntimes > 10) // about 10*5sec = 50sec
       {
       	bruntimes = 0;
		if(false == breboot_flag)
	       {
		    notify_RstMonitor(0x00); //  The qlinkdatanode programwill be restart after 3 min.
		    isUimPwrDwnMssage = true;
		    breboot_flag = true;
		    LOG("ERROR: connect() failed. errno == 101.isUimPwrDwnMssage =%d.\n",isUimPwrDwnMssage);
		 }
	}
    }
    return -1;
    }
  }else{ // ret == 0
    goto __EXIT_OF_SOCK_INET_CONN_TEST__;
  }

  //ret != 0 && errno == EINPROGRESS
  FD_ZERO(&wfd);
  FD_ZERO(&rfd);
  FD_SET(sock_fd, &wfd);
  FD_SET(sock_fd, &rfd);
  tv.tv_sec = 5; // In normal network environment, connect() returns in about 100ms.
  tv.tv_usec = 0;

  ret = select(sock_fd+1, &rfd, &wfd, NULL, &tv);
  if(ret < 0){
    LOG("ERROR: select() failed. errno=%d.\n", errno);
    close(sock_fd); // by jackli 20160308
    return -1;
  }else if(ret == 2 || ret == 0){
    //Situation Presentation:
    //connect() failed.
    errortimes++;
#ifdef FEATURE_ENABLE_REBOOT_DEBUG
	errortimes++;
       LOG("ERROR: +errortimes = %d \n", errortimes);
	if (errortimes >= 15){
		errortimes = 0;
		LOG("ERROR: errortimes  reboot.\n");
		close(sock_fd); // by jackli 20160308
	     //  yy_popen_call("reboot");
		yy_popen_call("reboot -f");
		
		
	}
	
 #endif
    
    return 0;
  }
  
  //ret == 1
__EXIT_OF_SOCK_INET_CONN_TEST__:
  close(sock_fd);
  errortimes = 0;
  return 1;
}

static void urc_parse_hdlr(void *userdata)
{
  int read_bytes = -1, ret = -1;
  char rsp_buf[SOCK_READ_BYTES+1] = {0}, *p_buf = NULL;

  if(listen_fd < 0){
    LOG("ERROR: listen_fd is unavailable!\n");
    err_num_s = -4;
    goto __EXIT_OF_LISTENER_HDLR__;
  }

  LOG6("~~~~ urc_parse_hdlr start ~~~~\n");

  memset(rsp_buf, 0, SOCK_READ_BYTES+1);
  
  do{
    if(read_bytes == 0){
      LOG("read() return 0. errno=%d.\n", errno);
    }

    read_bytes = read(listen_fd, rsp_buf, SOCK_READ_BYTES);
  }while(read_bytes == 0 || (read_bytes < 0 && (errno == EINTR || errno == EAGAIN)));
 
  if(read_bytes < 0){
    LOG("ERROR: read() failed. errno=%d.\n", errno);
    goto __EXIT_OF_LISTENER_HDLR__;
  }

  if(urc_fragment[0] == '\0')
    init_URC_array();
  
  p_buf = rsp_buf;
  
__PARSE_RAW_URC_DATA_AGAIN__:
  ret = parse_raw_URC_data(p_buf, read_bytes);
 
#ifdef FEATURE_ENABLE_PRINT_URC_ARRAY
	{
		#ifndef FEATURE_DEBUG_LOG_FILE
			int i = 0;
			LOG("***********************************************\n");
			LOG("URC_array_len = %d\n", URC_array_len);
			for(; i<URC_array_len; i++){
				LOG("URC_array[%d]: %s\n", i, URC_array[i]);
			}
			LOG("***********************************************\n");
		#else
			int i = 0;
			pthread_mutex_lock(&log_file_mtx);
			LOG5("***********************************************\n");
			LOG4("URC_array_len = %d\n", URC_array_len);
			for(; i<URC_array_len; i++){
				LOG4("URC_array[%d]: %s\n", i, URC_array[i]);
			}
			LOG4("***********************************************\n");
			pthread_mutex_unlock(&log_file_mtx);
		#endif
	}
#endif

  LOG("parse_raw_URC_data() return %d.\n", ret);
  if(ret == 0 || ret == 1)
  {
    int valid_URC_num = -1, handle_result = 1;
	
  LOG("Debug: isSimCardNotAvailRecved = %d, isSessIllegalRecved = %d,isUimPwrDwn = %d,\n",isSimCardNotAvailRecved,isSessIllegalRecved,isUimPwrDwn);

    //Handle "+SIMCARD: NOT AVAILABLE" inside.
    //URC frags are found after parse_raw_URC_data(). So consider "memset(urc_fragment, 0, sizeof(urc_fragment));"  before 
    //"goto __EXIT_OF_LISTENER_HDLR__;" from now on.
    valid_URC_num = check_valid_URC_array_elem(valid_URC_prefix_array, valid_URC_prefix_array_len);
    if(isSimCardNotAvailRecved)
    {
      LOG("NOTICE: isSimCardNotAvailRecved = 1, so ignore reg inds except \"+SIMCARD: NOT AVAILABLE\".\n");
      isSimCardNotAvailRecved = false;
      memset(urc_fragment, 0, sizeof(urc_fragment)); //Add by rjf at 20151027(Dump URC frag and its subsequent frag part)
      goto __EXIT_OF_LISTENER_HDLR__;
    }
    else if(isSessIllegalRecved)//Add by rjf at 20151026
    {
      LOG("NOTICE: isSessIllegalRecved = 1, so ignore reg inds except \"+SIMCARD: SESSION ILLEGAL\".\n");
      isSessIllegalRecved = false;
      memset(urc_fragment, 0, sizeof(urc_fragment)); //Add by rjf at 20151027	(Dump URC frag and its subsequent frag part)
      goto __EXIT_OF_LISTENER_HDLR__;
    }
    // Add "true == isApduRspTimeout" by rjf at 20151014 
    //(Ignore reg inds except "+SIMCARD: NOT AVAILABLE" not only after virt USIM was powered down but after APDU rsp timed out and "0x98
    // 0x62" was sent to modem as APDU rsp.
    else if(true == isUimPwrDwn || true == isApduRspTimeout)
    {
      LOG("NOTICE: isUimPwrDwn=%d, isApduRspTimeout=%d. So ignore reg inds.\n", isUimPwrDwn, isApduRspTimeout);
      memset(urc_fragment, 0, sizeof(urc_fragment)); //Add by rjf at 20151027 (Dump URC frag and its subsequent frag part)
      goto __EXIT_OF_LISTENER_HDLR__;
    }

    LOG("%d valid URC(s) found in URC_array.\n", valid_URC_num);

    if(0 == valid_URC_num){
      //Not clr urc_fragment. The URC rpt of urc_fragment may be valid.
      goto __EXIT_OF_LISTENER_HDLR__;
    }

    last_reg_urc_state = reg_urc_state;
    
    process_URC_array_v01();

    LOG("last_reg_sys_mode_state=%d, reg_sys_mode_state=%d.\n", last_reg_sys_mode_state, reg_sys_mode_state);

    if(last_reg_sys_mode_state != reg_sys_mode_state 
      && last_reg_sys_mode_state != REG_SYS_MODE_MIN// Avoid to send out redundant status info in startup process
      && reg_sys_mode_state != REG_SYS_MODE_UNKNOWN// Avoid to send out status info when "+CNSMOD: 0" is shown up
      && isRegOn == true)// Add by rjf at 201509112130
    {
      need_to_upload_stat_inf = true;
    }

    LOG("last_reg_urc_state=%d, reg_urc_state=%d.\n", last_reg_urc_state, reg_urc_state);
    handle_result = handle_reg_urc_state_var();
	
    if(0 == ret && 1 == handle_result)
    {
      init_URC_array();
      p_buf = URC_buf_break_point; // URC fragment is stored in urc_fragment
      goto __PARSE_RAW_URC_DATA_AGAIN__;
    }

    if(true == need_to_upload_stat_inf && false == isApduRspTimeout)
    {
      LOG("need_to_upload_stat_inf=1. Now, go to upload status info.\n");
      need_to_upload_stat_inf = false;

#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
      StatInfReqNum ++;
#endif // FEATURE_ENHANCED_STATUS_INFO_UPLOAD
      
      get_dev_and_net_info_for_status_report_session();
  
      trigger_rsmp_status_report();
	    
    }
    else if(true == need_to_upload_stat_inf && true == isApduRspTimeout)
    {
      LOG("isApduRspTimeout=1, So cancel the status info upload.\n");
      need_to_upload_stat_inf = false;
	 
    }

    last_reg_sys_mode_state = reg_sys_mode_state; // Reset

#ifdef FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
    if(rab_restab_rej_or_fail_counter >= 2 )
    {
      LOG("~~~~~~~~~~~~RAB Restab rej or fail~~~~~~\n");
      rab_restab_rej_or_fail_counter = 0;
      
      //do offline->online
      pthread_mutex_lock(&rst_rf_mtx);
      isRstRf = true;
      notify_DmsSender(0x04);//AT+CFUN=4
      while(isRfRsted == false){
        pthread_cond_wait(&rst_rf_cond, &rst_rf_mtx);
      }
      isRfRsted = false; // reset
      sleep(3); // Wait 3s for LPM to take effect
      notify_Sender(QMISENDER_SET_ONLINE);
      pthread_mutex_unlock(&rst_rf_mtx);
    }
#endif // FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
  }
 
__EXIT_OF_LISTENER_HDLR__:
  LOG6("~~~~ urc_parse_hdlr end ~~~~\n");
}

static void mifi_status_hdlr(void *userdata)
{
  char pre_cnd = 0x00, recv_buf[2] = {0};	
  int ret = -1;

  LOG6("~~~~ mifi_status_hdlr start ~~~~\n");

  do{
    ret = read(SockListener_2_Listener_pipe[0], recv_buf, 1);
  }while(ret == 0 || (ret < 0 && errno == EINTR));

  LOG("recv_buf[0] = 0x%02x.\n", recv_buf[0]);
  if(recv_buf[0] == 0x01)
  {
    isMifiConnected = true;
  //Skip setting ChannelNum to 3 for suspending mdmEmodem soon.

#ifdef FEATURE_ENABLE_MSGQ
    msq_send_dialup_ind();
#endif

    pthread_mutex_lock(&i1r_mtx);
    if(is1stRunOnMdmEmodem == true)
    {
      init_dev_info(&rsmp_dev_info);
    }
    is1stRunOnMdmEmodem = false;
    pthread_mutex_unlock(&i1r_mtx);

    pre_cnd = rsmp_net_info.basic_val;
    if(pre_cnd != 0x04)
    {
      //Note: Re-dial will not trigger status upload, i.e. pre_cnd != 0x04.
      update_net_info(0x02, 0x01, 0x00, 0x00);

      sock_net_info_processor(pre_cnd, rsmp_net_info); //status_info_upload_acq_info() inside.

      sleep(1); //Add by rjf at 20151012 (Not need to suspend mdmEmodem so quickly. Just wait to send out status info msg.)
    }

    if(check_ChannelNum(2)){
      //Not allow to rm check if ChannelNum is 2. Because u can't change ChannelNum from 2 to 3 here.
      //Skip setting ChannelNum to 3.
      LOG("Suspension for mdmEmodem is ongoing!\n");
    }else{
      set_ChannelNum(3);

      //ChannelNum should be 3 in case that isWaitToSusp is set in RecvCallback().
      pthread_mutex_lock(&evtloop_mtx);
      notify_EvtLoop(0x03);
      pthread_mutex_unlock(&evtloop_mtx);
    }
  }else if(recv_buf[0] == 0x00){
    isMifiConnected = false; //ChannelNum will be updated when reg inds recved by app.
  }

  LOG6("~~~~ mifi_status_hdlr end ~~~~\n");
}

//Description:
//The content in evtloop pipe:
//0x00: Req to acq mdm9215 inf.
static void evtloop_req_evt_hdlr(void *userdata)
{
  int ret = -1;
  char recv_buf = 0xff;

__EVTLOOP_REQ_EVT_HDLR_READ_AGAIN__:
	do{
		ret = read(EvtLoop_2_Listener_pipe[0], (void *)(&recv_buf), 1);
	}while(ret == 0 || (ret < 0 && errno == EINTR));
	
	if(ret < 0){
		if(errno == EAGAIN){
			goto __EVTLOOP_REQ_EVT_HDLR_READ_AGAIN__;
		}
		
		LOG("ERROR: read() failed. errno=%d.\n", errno);
		return;
	}else{
		if(recv_buf == 0x00){
			#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
				StatInfReqNum ++;
			#endif
			get_dev_and_net_info_for_status_report_session();
		}else{
			LOG("ERROR: Wrong recv_buf content (0x%02x) found!\n", recv_buf);
			return;
		}
	}
	
	pthread_mutex_lock(&el_req_mtx);
	el_reqed = true;
	pthread_cond_broadcast(&el_req_cnd);
	pthread_mutex_unlock(&el_req_mtx);
	
	return;
}

static void UrcMonitorLoop(void *userdata)
{
  int ret = -1, maxfdp1 = -1;
  fd_set rfds, rfds_bak;

  FD_ZERO(&rfds);
  FD_ZERO(&rfds_bak);
  FD_SET(listen_fd, &rfds_bak);
  FD_SET(SockListener_2_Listener_pipe[0], &rfds_bak);
  FD_SET(EvtLoop_2_Listener_pipe[0], &rfds_bak);
  maxfdp1 = (listen_fd>SockListener_2_Listener_pipe[0])?listen_fd:SockListener_2_Listener_pipe[0];
  maxfdp1 = (EvtLoop_2_Listener_pipe[0]>maxfdp1)?EvtLoop_2_Listener_pipe[0]:maxfdp1;
  maxfdp1++;
  
  init_valid_URC_prefix_array();
  set_valid_prefix_array("+CGREG:");
  set_valid_prefix_array("+CEREG:");
  set_valid_prefix_array("+CPIN:");
  set_valid_prefix_array("+REGREJ:");
  set_valid_prefix_array("+CNSMOD:");
#ifdef FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
  set_valid_prefix_array("+RAB:");//xiaobin
#endif //FEATURE_RAB_REESTAB_REJ_OR_FAIL_HANDLE
  LOG("Valid URC prefix settled. valid_URC_prefix_array_len=%d.\n", valid_URC_prefix_array_len);

  memcpy(special_URC_array[SPEC_URC_ARR_IDX_VIRT_USIM_PWR_DWN], 
        "+SIMCARD: NOT AVAILABLE", 
        sizeof("+SIMCARD: NOT AVAILABLE"));
  memcpy(special_URC_array[SPEC_URC_ARR_IDX_VIRT_USIM_SESSION_ILLEGAL], 
        "+SIMCARD: SESSION ILLEGAL", 
        sizeof("+SIMCARD: SESSION ILLEGAL"));
  monitored_special_URC_num += 2;

  for(;;)
  {
    memcpy(&rfds, &rfds_bak, sizeof(rfds_bak));

		ret = select(maxfdp1, &rfds, NULL, NULL, NULL);
		LOG("select() return %d.\n", ret);
		
		if(ret > 0){
			if(ret == 3){
				
				mifi_status_hdlr(NULL);
				urc_parse_hdlr(NULL);
				evtloop_req_evt_hdlr(NULL);
			}else if(ret == 2){
				// 1st
				
				if(FD_ISSET(SockListener_2_Listener_pipe[0], &rfds)){
					
					mifi_status_hdlr(NULL);
				}
					
				// 2nd
				if(FD_ISSET(listen_fd, &rfds)){
				
					urc_parse_hdlr(NULL);
				}
				
				// 3rd
				if(FD_ISSET(EvtLoop_2_Listener_pipe[0], &rfds)){
					
					evtloop_req_evt_hdlr(NULL);
				}
			}else if(ret == 1){
				
				if(FD_ISSET(listen_fd, &rfds)){
					
					urc_parse_hdlr(NULL);
				}else if(FD_ISSET(SockListener_2_Listener_pipe[0], &rfds)){
				
					mifi_status_hdlr(NULL);
				}else if(FD_ISSET(EvtLoop_2_Listener_pipe[0], &rfds)){
				
					evtloop_req_evt_hdlr(NULL);
				}else{
					LOG("ERROR: select() return %d, but no fd in rfds is ready yet. Continue.\n",
							ret);
					continue;
				}
			}else{
				LOG("ERROR: Unexpected ret val %d found. Continue.\n", ret);
				continue;
			}
		}else{
			LOG("ERROR: Unexpected ret val %d found. errno=%d. Continue.\n", 
					ret, errno);
			continue;
		}
		
	}

  LOG("Loop ends unexpectedly.\n");
  return;
}

//Return Values:
// 1: Success
//-1: Wrong param input. fd is unavailable.
//-2: Fail to send AT command.
//-3: FdListener() failed.
//-4: read() failed in reception of AT response.
static int initialUrcConig(const int fd)
{
  if(fd < 0){
    LOG("ERROR: Wrong param input. fd is unavailable.\n");
    return -1;
  }
  send_at_and_check_response(fd, "AT+CGREG=2\r\n");
  send_at_and_check_response(fd, "AT+CEREG=2\r\n");
  send_at_and_check_response(fd, "AT+CREGREJ=1\r\n");
  send_at_and_check_response(fd, "AT+CNSMOD=1\r\n");
  send_at_and_check_response(fd, "AT+CNLSA=1\r\n");
  return 1;
}

//result = -1: initATChannel() failed.
//             -2: initialUrcConig() failed.
//             -3: pipe() failed.
static void *initUrcMonitorThread(void *user_data)
{
  int ret = -1, result = 1;
  int file_des[2], file_des_2[2];

  ret = initATChannel(&listen_fd, LISTENER_CHANNEL);
	if(ret != 1){
		LOG("initATChannel() failed. ret=%d.\n", ret);
		result = -1;
		goto __INITSOCKTHREAD_COND_BROADCAST__;
	}
	at_fd = listen_fd; //Note: "/dev/smd8" is deprecated. And at_fd and listen_fd is merged into one.
	
	ret = initialUrcConig(listen_fd);
	if (ret != 1) 
  {
        LOG("setListener() error. ret=%d.\n", ret);
		result = -2;
        goto __INITSOCKTHREAD_COND_BROADCAST__;
    }

#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
  pthread_rwlock_init(&ChannelLock, NULL);
#else
  pthread_mutex_init(&ChannelLock, NULL);
#endif
  pthread_mutex_init(&i1r_mtx, NULL);

	ret = pipe(file_des);
	if(ret < 0){
		LOG("ERROR: pipe(file_des) error. errno=%d.\n", errno);
		result = -3;
        goto __INITSOCKTHREAD_COND_BROADCAST__;
	}
	SockListener_2_Listener_pipe[0] = file_des[0];
	SockListener_2_Listener_pipe[1] = file_des[1];
	fcntl(SockListener_2_Listener_pipe[0], F_SETFL, O_NONBLOCK);
	
	ret = pipe(file_des_2);
	if(ret < 0){
		LOG("ERROR: pipe(file_des_2) error. errno=%d.\n", errno);
		result = -3;
      goto __INITSOCKTHREAD_COND_BROADCAST__;
	}
	EvtLoop_2_Listener_pipe[0] = file_des_2[0];
	EvtLoop_2_Listener_pipe[1] = file_des_2[1];
	fcntl(EvtLoop_2_Listener_pipe[0], F_SETFL, O_NONBLOCK);

	pthread_mutex_init(&el_req_mtx, NULL);
	pthread_cond_init(&el_req_cnd, NULL);
	
__INITSOCKTHREAD_COND_BROADCAST__:
	*((int *)user_data) = result;
	
	pthread_mutex_lock(&listener_mutex);
  listenerStarted = 1;
  pthread_cond_broadcast(&listener_cond);
  pthread_mutex_unlock(&listener_mutex);

	if(result < 0){
		LOG("ERROR: Init proc error. result=%d.\n", result);
		return NULL;
	}
	
	UrcMonitorLoop(NULL);
	
	LOG("ERROR: UrcMonitorLoop() ended unexpectedly!\n");
	while(1){
		sleep(0x00ffffff);
	}
	return NULL;
}

int startUrcMonitorThread(void)
{
  int ret;
  pthread_attr_t attr;
  int result;

  listenerStarted = 0;
  pthread_mutex_lock(&listener_mutex);

  pthread_attr_init (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
#ifdef FEATURE_ENABLE_SYSTEM_RESTORATION
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

__SOCK_PTHREAD_CREATE_AGAIN__:
  ret = pthread_create(&listenerloop_tid, &attr, initUrcMonitorThread, (void *)(&result));
  if(ret != 0){
    if(EAGAIN == errno){
      sleep(1);
      goto __SOCK_PTHREAD_CREATE_AGAIN__;
    }

    LOG("ERROR: pthread_create() failed. errno=%d.\n", errno);
    return 0;
  }

  thread_id_array[THD_IDX_LISTENERLOOP] = (unsigned long)listenerloop_tid;

  while(listenerStarted == 0){
    pthread_cond_wait(&listener_cond, &listener_mutex);
  }

  pthread_mutex_unlock(&listener_mutex);

  if(result < 0){
    LOG("ERROR: initSockThread() error.\n.");
    return 0;
  }

  return 1;
}


#ifdef  FEATURE_ENABLE_STATUS_REPORT_10_MIN


#define USER_MAX_FLU_VALUE_LIMIT_256K		256
#define USER_MAX_FLU_VALUE_LIMIT_128K 		128
#define USER_MAX_FLU_VALUE_LIMIT_64K 			64
#define USER_MAX_FLU_VALUE_LIMIT_32K 			32
#define USER_MAX_FLU_VALUE_LIMIT_512K 		512
#define USER_MAX_FLU_VALUE_LIMIT_384K 		384

#define USER_MAX_FLU_VALUE_LIMIT_500M_DEFAULT		(500*1024*1024)
#define USER_MAX_FLU_VALUE_LIMIT_200M_DEFAULT		(200*1024*1024)
 	
#define STATUS_OK		 (1)
#define STATUS_ERROR 	 (-1)
#define STATUS_MIDDLE 	 (2)



extern pthread_mutex_t log_file_mtx;


/**********************************************************************
* SYMBOL	: StrInsert
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/3/2
**********************************************************************/

char* StrInsert(char *str,  char *str_ins ,int n)
{
	int i,j,str_len,str_ins_len;

	char * new_str = NULL;

	str_len = strlen(str);
	str_ins_len = strlen(str_ins);

	/*LOG("StrInsert:  ---------test----------- n = %d.\n", n);
	LOG("StrInsert:  ---------test----------- str_len = %d.\n", str_len);
	LOG("StrInsert:  ---------test----------- str_ins_len = %d.\n", str_ins_len);*/

	new_str = (char*)malloc(str_len+str_ins_len+n+1);
	if(new_str == NULL)
	{
		LOG("ERROR: malloc new_str is fail\n");
		return NULL;
	}

	if (n < 1 || n > str_len )
	{	
		LOG("Protocol_qlinkdatanode.c :  ---------test----------- 01\n");
		//free(new_str);
		return NULL;
	}
	else
	{
		//LOG("Protocol_qlinkdatanode.c :  ---------test----------- 02 \n");
		memcpy(new_str,str,n);
		/*LOG("3333333333333: new_str =%s.\n", new_str);
		LOG("3333333333333: str_ins =%s.\n", str_ins);
		LOG("3333333333333: str =%s.\n", str);*/
		
		memcpy(&new_str[n],str_ins,str_ins_len);
		/*LOG("44444444444444: new_str =%s.\n", new_str);
		LOG("44444444444444: str_ins =%s.\n", str_ins);
		LOG("44444444444444: str =%s.\n", str);*/

		memcpy(&new_str[n+str_ins_len],&str[n],(str_len-n));

		*(new_str+str_len+str_ins_len) = '\0';
		
		/*LOG("55555555555555: new_str =%s.\n", new_str);
		LOG("55555555555555: ststr_insr =%s.\n", str_ins);
		LOG("55555555555555: str =%s.\n", str);*/
		//free(new_str);
		//LOG("Protocol_qlinkdatanode.c :  ---------test----------- OK \n");
		return new_str;
	}
	
}


static void qlinkdatanodem_system_call(const char *command)
{
	//int result = 0;
	//FILE *stream = NULL;
	pid_t status;
	static unsigned char status_count  = 0;

	//LOG("qlinkdatanode_system_call: %s.\n", command);

	status  = system(command);

        if (-1 == status)  
        {  
            LOG("ERROR:socket_qlinkdatanode.c ~~~~qlinkdatanodem_system_call~~~~shutdown system error!\n");  
	     status_count++;
	     if (status_count >= 10)
	     {
	     	 status_count = 0;
		 LOG("ERROR:socket_qlinkdatanode.c ~~~~qlinkdatanodem_system_call~~~~shutdown status_count =  !\n",status_count);  
	     }
	}  
        else  
        {  
           // LOG(" socket_qlinkdatanode.c:exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                  //  LOG("socket_qlinkdatanode.c: ~~~qlinkdatanodem_system_call~~~~run shell script successfully.\n");  
                }  
                else  
              {
                    LOG("ERROR:socket_qlinkdatanode.c ~~~~qlinkdatanodem_system_call run shell script fail, script exit code: %d\n", WEXITSTATUS(status));  
                }  
            }  
            else  
            {  
                LOG("socket_qlinkdatanode.c: ~~~qlinkdatanodem_system_call~~~ exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
	
}

static void qlinkdatanode_flu_limit_default(void)
{

	LOG(" socket_qlinkdatanode.c:~~qlinkdatanode_flu_limit(): the defalut limit speed is 256k\n");
	//qlinkdatanodem_system_call("cd usr/bin/ && ./limit_flu>limit_flu.log");
		
}

static void qlinkdatanode_flu_limit(unsigned int speedvalue)
{
	if (speedvalue == USER_MAX_FLU_VALUE_LIMIT_256K)
	{
		LOG(" socket_qlinkdatanode.c:~~qlinkdatanode_flu_limit(): the UE is limited 256k \n");
		//qlinkdatanodem_system_call("cd usr/bin/ && ./limit_flu 2mbit>limit_flu.log");
	}
	else if (speedvalue == USER_MAX_FLU_VALUE_LIMIT_128K)
	{
		LOG(" socket_qlinkdatanode.c: ~~qlinkdatanode_flu_limit(): the UE is limited 128k \n");
		//qlinkdatanodem_system_call("cd usr/bin/ && ./limit_flu 1mbit>limit_flu.log");
	}
	else
	{
		LOG(" socket_qlinkdatanode.c:~~qlinkdatanode_flu_limit():Don't get limit speed value from server \n");
		//qlinkdatanode_flu_limit_default();
	}
	
}

/**********************************************************************
* SYMBOL	: locat_str
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/3/2
**********************************************************************/

//static int locat_str(char *source_str, char *des_str)
static int locat_str(char *str_father, char *str_child)
{
	char *pf_str = NULL;
	char *pch_str = NULL;
	int num = 0;

	char *pfrist_add = NULL;
	char *pstr_add = NULL;

	pf_str = str_father;
	pch_str = str_child;

	pfrist_add = pf_str;
	pstr_add = strstr(pf_str,pch_str);

	//LOG("locat_str:  ---------test----------- pfrist_add = 0x%02x.\n", pfrist_add);
	//LOG("locat_str:  ---------test----------- pstr_add = 0x%02x.\n", pstr_add);

	num = (pstr_add - pfrist_add);

	//LOG("locat_str:  ---------test----------- num = %d.\n", num);

	return num;
	

}

/**********************************************************************
* SYMBOL	: GetIP_addforTcCommand
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/3/2
**********************************************************************/

char * GetIP_addforTcCommand(int ip_add_num)
{
	int ichild_add,i;
	char *ip_command_exce_str = NULL ;
	char *ip_command_str[60] = {
			"192.168.0.100","192.168.0.101","192.168.0.102","192.168.0.103","192.168.0.104",
			"192.168.0.105","192.168.0.106","192.168.0.107","192.168.0.108","192.168.0.109",
			"192.168.0.110","192.168.0.111","192.168.0.112","192.168.0.113","192.168.0.114",
			"192.168.0.115","192.168.0.116","192.168.0.117","192.168.0.118","192.168.0.119",
			"192.168.0.120","192.168.0.121","192.168.0.122","192.168.0.123","192.168.0.124",
			"192.168.0.125","192.168.0.126","192.168.0.127","192.168.0.128","192.168.0.129",
			"192.168.0.130","192.168.0.131","192.168.0.132","192.168.0.133","192.168.0.134",
			"192.168.0.135","192.168.0.136","192.168.0.137","192.168.0.138","192.168.0.139",
			"192.168.0.140","192.168.0.141","192.168.0.142","192.168.0.143","192.168.0.144",
			"192.168.0.145","192.168.0.146","192.168.0.147","192.168.0.148","192.168.0.149",
			"192.168.0.150","192.168.0.151","192.168.0.152","192.168.0.153","192.168.0.154",
			"192.168.0.155","192.168.0.156","192.168.0.157","192.168.0.158","192.168.0.159",
		};

	char *at_commanf_ip = "tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip dst 192.168.0.120 flowid 1:1";
	char *at_commanf_defalut_ip = "tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip dst  flowid 1:1";

	ichild_add = locat_str(at_commanf_ip,"192.168.0");
	//LOG("get the locate  ichild_add = %d.\n", ichild_add);
	ip_command_exce_str = StrInsert(at_commanf_defalut_ip,ip_command_str[ip_add_num],(ichild_add));

	//LOG("the ip_command_str str: ip_command_exce_str =%s.\n", ip_command_exce_str);
	
	return ip_command_exce_str;
}

/**********************************************************************
* SYMBOL	: qlinkdatanode_flu_limit_No_shell
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/3/2
**********************************************************************/
static int qlinkdatanode_flu_limit_No_shell(int ispeed)
{
	int ret = 0;
	int ichild_add,i,j;
	char *newstr= NULL;
	char *new_ip_str= NULL;
	char *at_commanf_defalut_speed = "tc class add dev eth0 parent 1: classid 1:1 htb rate burst 15k";
	char *at_command_str[5]={

		"tc qdisc del root dev eth0",
		"tc qdisc add dev eth0 root handle 1: htb",
		"tc class add dev eth0 parent 1: classid 1:1 htb rate 128kbit burst 5k",
		"tc qdisc add dev eth0 parent 1:1 handle 10: sfq perturb 10",
		"tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip dst 192.168.0.120 flowid 1:1",

		};
	char *at_commanf_defalut_config = "tc class add dev eth0 parent 1: classid 1:1 htb rate 32kbit burst 5k";
	
	bool bIsFreeP = false;

#ifdef	FEATURE_ENABLE_SYSTEM_FOR_PORT_0223

						// 21 FTP PORT
						// 53 DNS PORT
						// 25 110  MAIL SERVER
						
						//qlinkdatanodem_system_call("iptables -A FORWARD -p tcp --sport 21 -j DROP"); // CLOSE 21 PORT
						
						
#endif

	//GetIP_addforTcCommand(10);
	//LOG("----- at_commanf_defalut_ip =%s.\n", at_commanf_defalut_ip);

	/*int ip_add_num = 0;
	
	char ip_add_str [15];

	char *ip_addfirst_str = "10a";
	
	ip_add_num = atoi(ip_addfirst_str);
	LOG("----22   ip_add_num = %d.\n", ip_add_num);
	ip_add_num++;
	LOG("----  ip_add_num = %d.\n", ip_add_num);
	
	sprintf(ip_add_str,"%d",ip_add_num);

	LOG("----- ip_add_str =%s.\n", ip_add_str);*/

	/*for(i=0;i<230;i++)
	{
		itoa(ip_add_first,(char*)ip_add_str[i],10);
		ip_add_first++;
		LOG("----  i = %d, ip_add_first = %d.\n", i, ip_add_first);
		LOG("----- ip_add_str =%s.\n", ip_add_str[i]);
	}*/

#if 1	

	ichild_add = locat_str(at_command_str[2],"kbit");

	LOG("Debug: ispeed =%d.\n", ispeed);

	if (ispeed == USER_MAX_FLU_VALUE_LIMIT_512K)
	{
		LOG("get the locate  ichild_add = %d.\n", ichild_add);
		newstr = StrInsert(at_commanf_defalut_speed,"512kbit ",(ichild_add-3));
	}
	else if(ispeed == USER_MAX_FLU_VALUE_LIMIT_384K)
	{
		LOG("get the locate  ichild_add = %d.\n", ichild_add);
		newstr = StrInsert(at_commanf_defalut_speed,"384kbit ",(ichild_add-3));
	}
	else if(ispeed == USER_MAX_FLU_VALUE_LIMIT_256K)
	{
		LOG("get the locate  ichild_add = %d.\n", ichild_add);
		newstr = StrInsert(at_commanf_defalut_speed,"256kbit ",(ichild_add-3));
	}
	else if(ispeed == USER_MAX_FLU_VALUE_LIMIT_128K)
	{
		LOG("get the locate  ichild_add = %d.\n", ichild_add);
		newstr = StrInsert(at_commanf_defalut_speed,"128kbit ",(ichild_add-3));
	}
	else if(ispeed == USER_MAX_FLU_VALUE_LIMIT_64K)
	{
		
		ichild_add = locat_str(at_commanf_defalut_config,"kbit");
		LOG("get the locate  ichild_add = %d.\n", ichild_add);
		newstr = StrInsert(at_commanf_defalut_speed,"64kbit ",(ichild_add-2));
	}
	else if(ispeed == USER_MAX_FLU_VALUE_LIMIT_32K)
	{
		ichild_add = locat_str(at_commanf_defalut_config,"kbit");
		LOG("get the locate  ichild_add = %d.\n", ichild_add);
		newstr = StrInsert(at_commanf_defalut_speed,"32kbit ",(ichild_add-2));
	}
	else
	{
		LOG("ERROR: Get the speed value is fail form server ,set default speed.\n");
		newstr = "tc class add dev eth0 parent 1: classid 1:1 htb rate 128kbit burst 5k";
		bIsFreeP = true;
	}

	LOG("the new str: newstr =%s.\n", newstr);

	for (i = 0; i< 5;i++)
	{
		//LOG("shell command:at_command_str[i]  =%s.\n", at_command_str[i]);
		if(i == 2)
		{
			ret = yy_popen_call(newstr);
			LOG("limit flu command soeed\n");

		}
		else if (4 == i)
		{
			for (j = 0; j < 60;j++)
			{
				new_ip_str = GetIP_addforTcCommand(j);
				//LOG("new_ip_str: new_ip_str =%s.\n", new_ip_str);
				ret =yy_popen_call(new_ip_str);
			}
		}
		else
		{
			//LOG("shell command:at_command_str[i]  =%s.\n", at_command_str[i]);
			ret = yy_popen_call(at_command_str[i]);
			LOG("limit flu command\n");
		}	

	}

	if (bIsFreeP != true){

		free(newstr);
	}
	
	free(new_ip_str);

   #endif    

	return ret;
}


 
static void qlinkdatanode_cleanflu_limit(void)
{

#ifdef	FEATURE_ENABLE_SYSTEM_FOR_PORT_0223

						
						// 21 FTP PORT
						// 53 DNS PORT
						// 25 110  MAIL SERVER
						// 21 FTP PORT
						// 21 FTP PORT
						//qlinkdatanodem_system_call("iptables -A FORWARD -p tcp --sport 21 -j ACCEPT"); // OPEN 21 PORT
						
#endif


	qlinkdatanodem_system_call("tc qdisc del root dev eth0");
}
static int ReadSpeciaLine(int linenumber,char * strbuf)
{
	char filename[] = "/var/pingresult.txt";
	FILE *fp;

	int whichline;
	int CurrentIndex = 0;
	char strline[128];
	char * pstrbuf;

	whichline = linenumber;
	pstrbuf = strbuf;
	if (pstrbuf == NULL)
	{
		LOG("ERROR :  ---------test--222.\n");
		return STATUS_ERROR;
	}
	
	if ((fp = fopen(filename,"r")) == NULL)
	{
		LOG("ERROR :  ---------test--111.\n");
		return STATUS_ERROR;
	}
	
	while(!feof(fp))
	{
		if (CurrentIndex == whichline)
		{
			fgets(strline,128,fp);
			LOG("----- strline =%s.\n", strline);
			memcpy(pstrbuf,strline,128);
			fclose(fp);
			return STATUS_OK;
		}

		fgets(strline,128,fp);
		CurrentIndex++;
		LOG("----- CurrentIndex = %d.\n", CurrentIndex);
		
	}

	fclose(fp);
	return STATUS_ERROR;
	
}
static int qlinkdatanodeNetTestShell(void)
{
	//qlinkdatanodem_system_call("cd usr/bin/ && ./net_test>net_test.txt");
//char * str_ip = "www.baidu.com";
//char * str_ping_command = "ping -c 2 $str_ip | grep -q 'ttl='";
 //qlinkdatanodem_system_call("ping 112.80.248.74>/var/pingresult.txt");
 FILE *fp;
 char * str_ping_command  = "ping -c 10 -w 20 www.baidu.com";
 char bufline0[128];
 char bufline1[128];
 char bufline2[128];
 char bufline3[128];
 bool bstristrue0 = false;
 bool bstristrue1 = false;
 int i=0;

 qlinkdatanodem_system_call("ping -c 10 -w 20 www.baidu.com>/var/pingresult.txt");
 ReadSpeciaLine(0,bufline0);
 ReadSpeciaLine(1,bufline1);
 ReadSpeciaLine(2,bufline2);
 ReadSpeciaLine(3,bufline3);
 
 for(i=0;i<128;i++)
 {
 	//LOG("----- bufline1[i]= %c.\n", bufline1[i]);

	if((bufline1[i] == '6') && (bufline1[i+1] == '4') && (bufline1[i+2] == ' ')  && (bufline1[i+3] == 'b')  && (bufline1[i+4] == 'y')  && (bufline1[i+5] == 't')  && (bufline1[i+6] == 'e')  && (bufline1[i+7] == 's'))
	{
		LOG("----- i = %d.\n", i);
		bstristrue0 = true;
	}
	
 }
 
 for(i=0;i<128;i++)
 {
 	//LOG("----- bufline2[i]= %c.\n", bufline2[i]);

	if((bufline2[i] == '6') && (bufline2[i+1] == '4') && (bufline2[i+2] == ' ')  && (bufline2[i+3] == 'b')  && (bufline1[i+4] == 'y')  && (bufline2[i+5] == 't')  && (bufline2[i+6] == 'e')  && (bufline2[i+7] == 's'))
	{
		LOG("----- i = %d.\n", i);
		bstristrue1 = true;
	}
	
 }

 if ((bstristrue1 == true) &&  (bstristrue0 == true) )
 {
 	LOG("OK :  ping ok.\n");
	bstristrue1 = false;
	bstristrue0 = false;
	return STATUS_OK;
 }
 else
 {
	LOG("ERROR :  ping fail.\n");
	bstristrue1 = false;
	bstristrue0 = false;
	return STATUS_ERROR;
 }



 /*if ((fp = popen(str_ping_command,"r")) == NULL)
 {
	LOG("ERROR :  ---------test--0000000000000.\n");
	return STATUS_ERROR;
 }

 fgets(buf,128,fp);

 LOG("----- buf =%s.\n", buf);
 for(i=0;i<128;i++)
 {
 	LOG("----- buf[i]= %c.\n", buf[i]);

	if((buf[i] == 'd') && (buf[i+1] == 'a') && (buf[i+2] == 't')  && (buf[i+3] == 'a'))
	{
		LOG("----- i = %d.\n", i);
		pclose(fp);
		return STATUS_OK;
	}
	
 }

 pclose(fp);
 LOG("ERROR :  ping fail.\n");
 return STATUS_ERROR;;*/
 return STATUS_OK;
}

static void qlinkdatanodeCloseWifiProcess(void)
{
	LOG("Debug:  #############Close WFIF#################.\n");
	qlinkdatanodem_system_call("killall QCMAP_Web_CLIENT");
}

static void qlinkdatanodeRestartWifiProcess(void)
{
	qlinkdatanodem_system_call("QCMAP_Web_CLIENT &");
}

/**********************************************************************
* SYMBOL	: ReadPingReturnValueFromFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/

static int ReadPingReturnValueFromFile(void)
{
	
	FILE *fp;
	char data[200] = "";
	unsigned char num = 0;
	
	//unsigned long flumax_long = 0;

	if ( (fp = fopen("/usr/bin/net_test.txt", "r")) == NULL )
	{
		LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!net_test .txt errno=%s(%d).\n",strerror(errno), errno);
    		return STATUS_ERROR;
	}
	
	

	fgets(data, 200, fp);

	/*for(num = 0;num<20;num++)
	{
		LOG("Protocol_qlinkdatanode.c :  ---------test----------- num = %d.\n", num);
		LOG("Protocol_qlinkdatanode.c :  ---------test----------- data[num] = 0x%02x.\n", data[num]);

	}*/

	if ((data[14] == 'y') && (data[15] == 'e') && (data[16] == 's'))
	{
		LOG("Protocol_qlinkdatanode.c :  ---------test----------- ping command return yes.\n");
		fclose(fp);
		return STATUS_OK;
	}
	else if ((data[14] == 'n') && (data[15] == 'o'))
	{
		LOG("Protocol_qlinkdatanode.c :  ---------test----------- ping command return no.\n");
		fclose(fp);
		return STATUS_MIDDLE;
	}
	else
	{
		LOG("ERROR:Protocol_qlinkdatanode.c :  ---------test----------- ping command return error \n");
		fclose(fp);
		return STATUS_ERROR;
	}

      
	  
}

struct sockaddr_in srv_addr;
int  srv_addr_len = 0;

static int CreatSR_socket(Udp_Config * pConfig)
{

	int ret;
	int flag;
	//struct sockaddr_in srv_addr;
	Udp_Config *pUdp_config;
	char test_IP[16] = "58.96.188.197";
	int test_port = 9999;
	fd_set test_wd,test_rd;
	struct timeval test_value;

	pUdp_config = pConfig;

	
	
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

 	LOG("Debug_UDP: g_enable_TCP_IP01_linkcount = %d.\n", g_enable_TCP_IP01_linkcount);
       LOG("Debug_UDP: g_enable_TCP_IP01_linkcount = %d.\n", g_enable_TCP_IP02_linkcount);
	LOG("Debug_UDP: g_EmodemUdp_link_server_state = %d.\n", g_EmodemUdp_link_server_state);

	 
	if ((tcp_server_port > 0) && (tcp_server_port < 9999) && (g_enable_TCP_IP01_linkcount < 3) && (g_enable_TCP_IP02_linkcount == 0)) {

		
		strncpy(IP_addr, tcp_IP_addr, 15);
	       server_port = tcp_server_port;
		
		LOG("Debug: UDP link use IP1. \n");

		   
	   }else if((tcp_server_port_2 > 0) && (tcp_server_port_2 < 9999) && (g_enable_TCP_IP01_linkcount >= 3) && (g_enable_TCP_IP02_linkcount < 3) && (g_EmodemUdp_link_server_state != true)){

	   
		strncpy(IP_addr, tcp_IP_addr_2, 15);
	       server_port = tcp_server_port_2;
		
		LOG("Debug: UDP link use IP2. \n");

	   }

#endif
	
	strncpy(test_IP, IP_addr, 15);
	test_port = server_port;
	

	if(test_IP[0] == '\0'){
       LOG("ERROR: IP_addr is empty.\n");
	}

       if(test_port > 65535 || test_port < 0){
       LOG("ERROR: server_port is incorrect.\n");
	test_port = 9988;
	   
       }
	  
	
	LOG("socket_qlinkdatanode.c: ~~~~ CreatSR_socket start ~~~~\n");

	#ifndef FEATURE_ENABLE_TCP_SOCK_TEST
		pUdp_config->isTimeout = false;
		
		pUdp_config->sock_des = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		LOG("Debug:: ~~~~ UDP LINK ~~~~\n");
		
	#else
		pUdp_config->sock_des = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	#endif
	
	if(pUdp_config->sock_des < 0){
		LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: socket() failed. errno=%s(%d).\n",
				strerror(errno), errno);
		return -1;
		
	}

	if((flag = fcntl(pUdp_config->sock_des, F_GETFL, 0)) < 0){
		LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: F_GETFL of fcntl() failed. errno=%s(%d).\n",
				strerror(errno), errno);
		return -1;
		
	}
	
	flag |= O_NONBLOCK;
	if((flag = fcntl(pUdp_config->sock_des, F_SETFL, 0)) < 0){
		LOG("ERROR: CreatSR_socket.c: CreatSR_socket: F_SETFL of fcntl() failed. errno=%s(%d).\n",
				strerror(errno), errno);
		return -1;
		
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(test_port);
	srv_addr.sin_addr.s_addr = inet_addr(test_IP);

	LOG("socket_qlinkdatanode.c: CreatSR_socket: Test begin. Connect to %s:%d.\n", test_IP, test_port);

#ifdef FEATURE_ENABLE_TCP_SOCK_TEST
/*	ret = inet_pton(AF_INET, test_IP, &srv_addr.sin_addr.s_addr);
	if(ret == 0){
			LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: inet_pton() failed. Invalid address string. IP_addr=%s.\n", test_IP);
			return -1;
	}else if(ret < 0){
			LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: inet_pton() failed. ret=%d.\n", ret);
			return -1;
	}*/
	
 #endif

	//ret = connect(pUdp_config->sock_des, (struct sockaddr *)(&srv_addr), sizeof(srv_addr));

 #ifdef FEATURE_ENABLE_TCP_SOCK_TEST  	
       if(ret<0){
		if(errno != EINPROGRESS){
			close(pUdp_config->sock_des);
			LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: connect() failed. ret=%d.\n", ret);
			return -1;
		}
	}else if(ret == 0){
		LOG("OK: socket_qlinkdatanode.c: CreatSR_socket: connect() OK. ret=%d.\n", ret);
		return 1;
	}
#endif
	

	FD_ZERO(&test_wd);
	FD_ZERO(&test_rd);
	FD_SET(pUdp_config->sock_des,&test_wd);
	FD_SET(pUdp_config->sock_des,&test_rd);

	test_value.tv_sec = 10;
	test_value.tv_usec = 0;

	ret= select((pUdp_config->sock_des)+1,&test_rd,&test_wd,NULL,&test_value);
	if(ret< 0){
		LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: select. ret=%d.\n", ret);
		return -1;
	}else if ((ret == 0) ||(ret == 2)){
		LOG("ERROR: socket_qlinkdatanode.c: CreatSR_socket: select. ret=%d.\n", ret);
		return 0;
	}else{
		return 1;
	}
	
	
}

static int SRsendData(Udp_Config * pConfig)
{
	Udp_Config *pUdp_config;
	int ret;
	char i;

	srv_addr_len = sizeof(struct sockaddr_in);
	
	pUdp_config = pConfig;

	if(pUdp_config->sock_des < 0){
		LOG("test: socket_qlinkdatanode.c: SRsendData: pUdp_config->sock_des=%d.\n", pUdp_config->sock_des);
		return -1;
		
	}

	
	pthread_mutex_lock(&log_file_mtx);
	LOG2("App -> server: \n");
	i = 1;
	for(; i<=req_config_get.size; i++){
		if(i%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(req_config_get.data, i-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(req_config_get.data, i-1) );
							
		if(i%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i-1)%20 != 0) // Avoid print redundant line break
			LOG3("\n");
	pthread_mutex_unlock(&log_file_mtx);


	ret = sendto(pUdp_config->sock_des,(char*)req_config_get.data, req_config_get.size,0,(struct sockaddr*)&srv_addr,srv_addr_len);
	
	//ret = write(pUdp_config->sock_des,(char*)req_config_get.data, req_config_get.size);
	LOG("Debug: sendto() . ret=%d.\n", ret);

	return 1;
}

void SRcloseSocket(Udp_Config * pConfig)
{
	Udp_Config *pUdp_config;

	pUdp_config = pConfig;

	if(pUdp_config->sock_des >= 0)
		close(pUdp_config->sock_des);
	pUdp_config->sock_des = -1;
	/*memset(&pudp_config.addr, 0, sizeof(pudp_config.addr));*/
	
	return;
}

static unsigned int Get_flu_maxVale()
{
  Dev_Info rsmp_dev_fluValue;
  long long rx_len,tx_len;

  unsigned int flu_maxValue;

  print_cur_gm_time("Get_flu_maxVale::::");

  rx_len = read_net_traffic_info(0);
  tx_len = read_net_traffic_info(1);
  rsmp_dev_fluValue.flux[0] = (unsigned int)((rx_len + tx_len) & 0x00000000ffffffff);
  rsmp_dev_fluValue.flux[1] = (unsigned int)((rx_len + tx_len) >> 32 & 0x00000000ffffffff);
  LOG("flux[0]: %u, flux[1]: %u.\n", rsmp_dev_fluValue.flux[0], rsmp_dev_fluValue.flux[1]);

  flu_maxValue =  rsmp_dev_fluValue.flux[0];

  return flu_maxValue;

}

static  long long Get_CurrentFluVale()
{
  Dev_Info rsmp_dev_fluValue;
  long long rx_len,tx_len;

  long long ulget_currentfluValue;

 //  print_cur_gm_time("Get_flu_maxVale::::");

  rx_len = read_net_traffic_info(0);
  tx_len = read_net_traffic_info(1);

  ulget_currentfluValue = (rx_len + tx_len);

  LOG("rx_len: %lld,tx_len: %lld.\n", rx_len, tx_len);
  LOG("ulget_currentfluValue: %lld \n", ulget_currentfluValue);

  return ulget_currentfluValue;
}


int GetMdmInfo(void)
{

	char i=0;
	int length = 0;
	unsigned char sendBuf_data;


	LOG("socket_qlinkdatanode.c: ~~~~~ GetMdmInfo start ~~~~~\n");
	 
	pthread_mutex_lock(&el_req_mtx);
	notify_ListenerLoop_el(0x00);
	while(!el_reqed){
					pthread_cond_wait(&el_req_cnd, &el_req_mtx);
	}
	el_reqed = false; // reset 
	pthread_mutex_unlock(&el_req_mtx);

	LOG("DEBUG: ----rsmp_di.flux[0] =%u.\n", rsmp_dev_info.flux[0]);
       LOG("DEBUG: ----rsmp_di.flux[1] =%u.\n", rsmp_dev_info.flux[1]);

	
	//proto_update_flow_state(FLOW_STATE_STAT_UPLOAD);
	//init_req_config(&req_config_get);
	req_config_get.StatInfMsgSn = StatInfMsgSn;
	req_config_get.data = proto_encode_raw_data(NULL, &length, REQ_TYPE_STATUS, &sim5360_dev_info, &rsmp_dev_info);
	req_config_get.size = length;

	
	
	LOG("socket_qlinkdatanode.c: ~~~~~ GetMdmInfo end ~~~~~\n");
	
	return 1;
}

/**********************************************************************
* SYMBOL	: ReadDataFromServerUDP
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/10/31
**********************************************************************/
static int ReadDataFromServerUDP(Udp_Config * pConfig ,char * p_recebuf)
{


	int ret;
	int temp;
	char *pread_buf = NULL;
	unsigned char  temp_switch;
	bool isDataRecMore = true;
	unsigned int isDataRecbit = 0;
	unsigned int isDataRecLen = 0;
	int isDataRecLenTotal = 0;
	Udp_Config *pUdp_config;
	int i;
	
	fd_set	fread;
       struct timeval tv_value;
	int  fd_socket; 

	struct timeval tv_timeout = {10,0};
	
	pread_buf = (char *)malloc(2*1024);

	if (pread_buf == NULL){

		LOG("ERROR: MALLOC null\n");
		return STATUS_ERROR;

	}
	
	pUdp_config = pConfig;


	if (pUdp_config->sock_des < 0){
		LOG("test: Socket_qlinkdatanode.c: ReadDataFromServer: pUdp_config->sock_des=%d.\n", pUdp_config->sock_des);
		free(pread_buf);
		return STATUS_ERROR;
		
	}

	//for
while(true == isDataRecMore)
{
	fd_socket = pUdp_config->sock_des;
	FD_ZERO(&fread);
	FD_SET(fd_socket,&fread);
	
	tv_value.tv_sec = 30; // 10S
	tv_value.tv_usec = 0;

	ret= select(fd_socket+1,&fread,NULL,NULL,&tv_value);


	if (ret< 0){
	
		LOG(" ERROR : select() error. errno=%s(%d).\n", strerror(errno), errno);
		free(pread_buf);
		return STATUS_ERROR;
	}
	else if (ret == 0)
	{
		LOG("ERROR:  timeout ~~~~\n");
		free(pread_buf);
		return STATUS_ERROR;
	}
	else 
	{
		
		if (FD_ISSET(fd_socket,&fread))
		{
			
				isDataRecLenTotal = recvfrom(fd_socket, &pread_buf[isDataRecbit], UDP_READ_BYTES, 0,(struct sockaddr*)&srv_addr,(socklen_t*)&srv_addr_len);// receive data from server

				LOG("DEBUG: isDataRecLenTotal = %d \n", isDataRecLenTotal);
				
				
				if ((isDataRecLenTotal) < 0 )
				{
					LOG("ERROR: a error happened in receive ~~~~\n");
					LOG(" ERROR : select() error. errno=%s(%d).\n", strerror(errno), errno);
					free(pread_buf);
					return STATUS_ERROR;
				}
				else if ((isDataRecLenTotal)  == 0)
				{
					LOG("ERROR:  a socket error happened in receive ~~~~\n");
					free(pread_buf);
					return STATUS_ERROR;
				}
				else
				{
					switch(*(pread_buf+0))
					{
					
						case 0xaa:
							isDataRecLen = (unsigned char)(*(pread_buf+1) ) << 24 |
											(unsigned char)(*(pread_buf+2) ) << 16 |
											(unsigned char)(*(pread_buf+3) ) << 8 |
											(unsigned char)(*(pread_buf+4) );
			
							break;
						default:
								LOG("OK: Reveice data contiune  ~~~~\n");
								break;
							
					}

					
					LOG("DEBUG: isDataRecLen = %d \n", isDataRecLen);
					isDataRecbit += isDataRecLenTotal;
					
					if (isDataRecbit >= isDataRecLen)
					{
						LOG("OK: ReadDataFromServer.c: All data has been received.  ~~~~\n");
						LOG("DEBUG: isDataRecbit00 = %d \n",isDataRecbit);
						LOG("DEBUG: isDataRecLen = %d \n",isDataRecLen);
						isDataRecMore = false;

						memcpy(p_recebuf,pread_buf,isDataRecLenTotal);

						pthread_mutex_lock(&log_file_mtx);
						LOG2("server -> APP: \n");
						i = 1;
						if (isDataRecLen > 70) // 
						{
							isDataRecLen = 70;
						}
						for(; i <= isDataRecLenTotal; i++){
							if(i%20 == 1)
								LOG4(" %02x", get_byte_of_void_mem(pread_buf, i-1) );
							else
								LOG3(" %02x", get_byte_of_void_mem(pread_buf, i-1) );
												
							if(i%20 == 0) //Default val of i is 1
								LOG3("\n");
						}
						if((i-1)%20 != 0) // Avoid print redundant line break
								LOG3("\n");
						pthread_mutex_unlock(&log_file_mtx);
					}
					
				}

		}
		else
		{
			LOG("ERROR: FD_ISSET RETURN 0\n");
		}
	}

}
	free(pread_buf);
	return STATUS_OK;
}

static  int statusReport_Timeout_cb(unsigned char bModeCount )
{
	
	int ret = 0;
	static unsigned long  ulucount = 0;
	Udp_Config TestUdp_config;
	char readbuf_udp[2*1024];
	bool read_udp_flag = true;
	unsigned char  read_udp_count = 0;
	unsigned char  read_udp_count5 = 0;
	data_msg_type *get_datafrom_server = NULL;
	

	ulucount++;
	if (50000 == ulucount){
		ulucount = 0;
	}
	print_cur_gm_time("My test::::");

	
	ret = GetMdmInfo();
	ret = CreatSR_socket(&TestUdp_config);
	if(ret == 1){
		SRsendData(&TestUdp_config);
	}

	memset(readbuf_udp,0,20);
	LOG("Debug: clear buf. \n ");

	LOG("Debug: bModeCount = %d \n ",bModeCount);

	while(true == read_udp_flag){
		
		ret = ReadDataFromServerUDP(&TestUdp_config,readbuf_udp);
		
		LOG("Debug: ret = %d \n", ret);
		
		if (ret == STATUS_ERROR ){

			LOG("Debug: read_udp_count = %d \n", read_udp_count);

			read_udp_count5 = 0;
			
			if (read_udp_count < UDP_LINK_COUNT){
				read_udp_count++;
				LOG("ERROR: need to send again.\n ");
				SRsendData(&TestUdp_config);
				sleep(1);
				
			}else{

				if (bModeCount){
					read_udp_count = 0;
					read_udp_flag = true;
					LOG("Debug: bModeCount = true :Need to send again.\n ");
				       SRsendData(&TestUdp_config);
					sleep(1);

				}else{

					read_udp_count = 0;
					read_udp_flag = false;
					SRcloseSocket(&TestUdp_config);
					LOG("ERROR: Fail are 10 times from server \n");
					return STATUS_ERROR;
				}
				
			}

		}else{
				read_udp_count = 0;
			       get_datafrom_server = readbuf_udp;
				LOG("Debug: Get data from server,so start to prase. \n ");
				   
				if (get_datafrom_server->data_statusvalue == 1){

					read_udp_flag = false;
					rsmp_net_info.basic_val = 0x04; //add by jackli 20161212
					rsmp_net_info.cregrej_val = 0; // 2016/12/12 by jack li
					LOG("Debug: Get rsp is right, don't need to send again. \n ");
					
				}else if (get_datafrom_server->data_statusvalue == 2){

					LOG("Debug: Get data from server,need to reboot system. \n ");
					//yy_popen_call("reboot -f");
					yy_popen_call("reboot -f");
					read_udp_flag = false;			   

				}
				else{
						LOG("Debug: read_udp_count5 = %d \n", read_udp_count5);

						if (read_udp_count5 < UDP_LINK_COUNT){
							
							read_udp_count5++;
							LOG("ERROR: Get rsp data is error, need to send again.\n ");
							SRsendData(&TestUdp_config);
							sleep(1);
							
						}else{

							if (bModeCount){
								read_udp_count5 = 0;
								read_udp_flag = true;
								LOG("Debug: bModeCount = true :Need to send again.\n ");
							       SRsendData(&TestUdp_config);
								sleep(1);

							}else{
							
								read_udp_count5 = 0;
								read_udp_flag = false;
								SRcloseSocket(&TestUdp_config);
								LOG("ERROR: Get rsp data is error and read times more than 5. \n");
								return STATUS_ERROR;
								
							}
						}
					
				}
				
		}

	}
	
	SRcloseSocket(&TestUdp_config);
	return 1;

}


void notify_Sender_status(char msg)
{
	int n = -1;
	int WriteStatus_fd = Status_rw_pipe[1],writelen=0;
	
	LOG6("~~~~ notify_Sender_status (%02x) ~~~~~\n", msg);

	
	do{
			writelen = write(WriteStatus_fd, (void *)(&msg), 1);
		}while(writelen == 0 || (writelen < 0 && errno == EINTR));
		if(writelen < 0){
			LOG("ERROR: write() failed. errno=%d.\n", errno);
		}
	return;
}

/**********************************************************************
* SYMBOL	: WriteUpdaterRecordToFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/25
**********************************************************************/

int WriteUpdaterRecordToFile(unsigned int unrecord)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char buf[100];
	int itest;
	
	memset(buf, 0 ,sizeof(buf));

	if ( (file = fopen("/usr/bin/save_updaterecord.txt", "wb+")) < 0 )
	{
		LOG(" ERROR :  ~~~~status_Loop: fopen() error. errno=%s(%d).\n", strerror(errno), errno);
	}

       time(&t);
	time_write = localtime(&t);

	sprintf(buf, "save_updaterecord: %d, %d-%d-%d, %d:%d:%d\n", unrecord, time_write->tm_year+1900, time_write->tm_mon+1, time_write->tm_mday, time_write->tm_hour, time_write->tm_min, time_write->tm_sec);
	itest = fwrite(buf, sizeof(char), strlen(buf), file);
	fflush(file);
	fclose(file);

	return STATUS_OK;
	
}


/**********************************************************************
* SYMBOL	: test
* FUNCTION	: 
* PARAMETER	: void *user_data
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/25
**********************************************************************/

int WriteFluValueToFile(long long ttvalue,unsigned char ucfile_mode)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char buf[100];
	int itest;
	
	memset(buf, 0 ,sizeof(buf));

	if (ucfile_mode == 0)
	{
		if ( (file = fopen("/usr/bin/fludata.txt", "wb+")) < 0 )
		{
			LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen(fludata.txt) error. errno=%s(%d).\n", strerror(errno), errno);
		}
	}
	else if (ucfile_mode == 1)
	{
		if ( (file = fopen("/usr/bin/prefludata.txt", "wb+")) < 0 )
		{
			LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen(prefludata.txt) error. errno=%s(%d).\n", strerror(errno), errno);
		}
	}
	else if (ucfile_mode == 2)
	{
		if ( (file = fopen("/usr/bin/simcardfludata.txt", "wb+")) < 0 )
		{
			LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen(simcardfludata.txt) error. errno=%s(%d).\n", strerror(errno), errno);
		}
	}
	else if (ucfile_mode == 3)
	{
		if ( (file = fopen("/usr/bin/savefluone.txt", "wb+")) < 0 )
		{
			LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen(switchsimcardfludata.txt) error. errno=%s(%d).\n", strerror(errno), errno);
		}
	}

       time(&t);
	time_write = localtime(&t);

	sprintf(buf, "fluMax: %lld, %d-%d-%d, %d:%d:%d\n", ttvalue, time_write->tm_year+1900, time_write->tm_mon+1, time_write->tm_mday, time_write->tm_hour, time_write->tm_min, time_write->tm_sec);
	itest = fwrite(buf, sizeof(char), strlen(buf), file);
	fflush(file);
	fclose(file);
	return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: WriteAPNMessageToFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/12/23
**********************************************************************/

int WriteAPNMessageToFile(char * apnBuf)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char *tempbuf;
	int itest;
	
	tempbuf = apnBuf;

	if ( (file = fopen("/var/apnmessage.txt", "wb+")) < 0 )
	{
		LOG(" ERROR :  apnmessage  fopen() error. errno=%s(%d).\n", strerror(errno), errno);
		return STATUS_ERROR;
	}

	itest = fwrite(tempbuf, sizeof(char), 200, file);
	fflush(file);
	fclose(file);

	return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: Ascii_To_int
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/

long long Ascii_To_int(char * pbuf, char datalen)
{
	long long Asc_num= 0;
	char *pdata = NULL;

	pdata = pbuf;

	/*if (datalen > 10)
	{
		LOG("ERROR: Ascii_To_int: the data too long .\n");
		return STATUS_ERROR;
	}*/

	while(datalen)
	{
		Asc_num *= 10;
		Asc_num+= (*pdata) - '0';
		pdata++;
		datalen--;
	}

	//LOG(" Asc_num =%lld.\n", Asc_num);
	
	return Asc_num;
	
}


/**********************************************************************
* SYMBOL	: hex_to_int
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/
unsigned int hex_to_int( char *pabybuff ,unsigned int i_num)
{
	int i;
	unsigned int value;
	char *pabdata;

	if ((pabybuff == NULL) || (i_num > 4))
		return -1;

	pabdata = pabybuff;

	for(i=0; i < i_num; i++){

		if (*(pabdata+i) != 0){
			LOG(" All value is not 0.\n");
			break;
		}else{

			if (i == (i_num-1)){

				LOG(" All value is  0.\n");
				return 0;
			}

		}

	}

	value = 0;

	for(i=0; i < i_num; i++)
	{
		value <<= 8;
		value |= ( *(pabdata+i) & 0x000000ff);
	}

	return value;
}

long long hex_to_long( char *pabybuff ,unsigned int i_num)
{
	int i;
	long long value;
	char *pabdata;

	if (pabybuff == NULL)
		return -1;

	

	pabdata = pabybuff;

	for(i=0; i < i_num; i++){

		if (*(pabdata+i) != 0){
			LOG(" All value is not 0.\n");
			break;
		}else{

			if (i == (i_num-1)){

				LOG(" All value is  0.\n");
				return 0;
			}

		}

	}

	value = 0;

	for(i=0; i < i_num; i++)
	{
		value <<= 8;
		value |= ( *(pabdata+i) & 0x000000ff);
	}

	return value;
}


/**********************************************************************
* SYMBOL	: ReadFluValueFromFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/

int ReadFluValueFromFile(strflu_record *pflu_data,unsigned char ucfile_mode)
{
	char *data_ad = NULL;
	char *p_first = NULL;

	FILE *fp;
	char data[200] = "";
	unsigned char num = 0;
	strflu_record *pfluvalue_temp;

	pfluvalue_temp = pflu_data;

	//unsigned long flumax_long = 0;
	   
	if (ucfile_mode == 0)
	{
		if ( (fp = fopen("/usr/bin/fludata.txt", "r")) == NULL )
		{
			LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!fludata .txt errno=%s(%d).\n",strerror(errno), errno);
    			return STATUS_ERROR;
		}
	}
	else if (ucfile_mode == 1)
	{
		if ( (fp = fopen("/usr/bin/prefludata.txt", "r")) == NULL )
		{
			LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!prefludata .txt errno=%s(%d).\n",strerror(errno), errno);
    			return STATUS_ERROR;
		}
	}
	else if (ucfile_mode == 2)
	{
		if ( (fp = fopen("/usr/bin/simcardfludata.txt", "r")) == NULL )
		{
			LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!simcardfludata .txt errno=%s(%d).\n",strerror(errno), errno);
    			return STATUS_ERROR;
		}
	}
	else if (ucfile_mode == 3)
	{
		if ( (fp = fopen("/usr/bin/savefluone.txt", "r")) == NULL )
		{
			LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!switchsimcardfludata .txt errno=%s(%d).\n",strerror(errno), errno);
    			return STATUS_ERROR;
		}
	}

	fgets(data, 200, fp);
   
    	LOG("-----------%d---%s------\r\n",strlen(data),data);
		
       if (strlen(data)==0)
       {
	 LOG("ERROR: not found information from the text.txt!\r\n");
	 fclose(fp);
        return STATUS_ERROR;
       }

	// Get flu max from txt file
	p_first = strstr(data,"fluMax:");
	num = 0;
	if(p_first != NULL)
	{
		p_first += strlen("fluMax:");
		p_first += 1;
		data_ad = p_first;
		
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}

		(pfluvalue_temp->flu_value) = (long long)Ascii_To_int(p_first,num);
		
		// get year from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->year_record) = (unsigned int)Ascii_To_int(p_first,num);

		// get month from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->month_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get day from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->day_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get hour from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->hour_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get min from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->min_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get sec from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x0a )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->sec_record) = (unsigned char)Ascii_To_int(p_first,num);
	
		data_ad = NULL;
		p_first = NULL;
	
	}
	else
	{
		data_ad = NULL;
		p_first = NULL;
	       LOG("ERROR: p_first = NULL .\n");
		fclose(fp);
              return STATUS_ERROR;
	}

      fclose(fp);
      return STATUS_OK;
	  
}

 #ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

 

/**********************************************************************
* SYMBOL	: WriteTotalTrafficToFile
* FUNCTION	: 
* PARAMETER	: WriteTotalTrafficToFile
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/07
**********************************************************************/

static pthread_mutex_t fileoption_mutexForFlu = PTHREAD_MUTEX_INITIALIZER;

int WriteEndDateFromServerToFile(unsigned int  ttvalue)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char buf[100];
	int itest;
	
	
	memset(buf, 0 ,sizeof(buf));

	pthread_mutex_lock(&fileoption_mutexForFlu);

	if ( (file = fopen("/usr/bin/saveTotalDate.txt", "wb+")) < 0 )
	{
		LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen() error. errno=%s(%d).\n", strerror(errno), errno);
	}

       time(&t);
	time_write = localtime(&t); 

	
	sprintf(buf, "saveTotalDate: %d, %d-%d-%d, %d:%d:%d\n", ttvalue, time_write->tm_year, time_write->tm_mon, time_write->tm_mday, time_write->tm_hour, time_write->tm_min, time_write->tm_sec);
	itest = fwrite(buf, sizeof(char), strlen(buf), file);
	fflush(file);
	fclose(file);
	pthread_mutex_unlock(&fileoption_mutexForFlu);

	return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: ReadUserTrafficFromFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/07
**********************************************************************/

int ReadEndDateFromServerFile(unsigned int *pGetminTime)
{
	char *data_ad = NULL;
	char *p_first = NULL;

	FILE *fp;
	char data[200] = "";
	unsigned char num = 0;
	unsigned int *pfluvalue_temp;

	pfluvalue_temp = pGetminTime;

	
	if ( (fp = fopen("/usr/bin/saveTotalDate.txt", "r")) == NULL )
	{
		LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!fludata .txt errno=%s(%d).\n",strerror(errno), errno);
		return STATUS_ERROR;
	}
	
	

	fgets(data, 200, fp);
   
    	LOG("-----------%d---%s------\r\n",strlen(data),data);
		
       if (strlen(data)==0)
       {
	 LOG("ERROR: not found information from the text.txt!\r\n");
	 fclose(fp);
        return STATUS_ERROR;
       }

	// Get flu max from txt file
	p_first = strstr(data,"saveTotalDate:");
	num = 0;
	if(p_first != NULL)
	{
		p_first += strlen("saveTotalDate:");
		p_first += 1;
		data_ad = p_first;
		
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}

		*pfluvalue_temp = (unsigned int)Ascii_To_int(p_first,num);
		
	/*	// get year from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->year_record) = (unsigned int)Ascii_To_int(p_first,num);

		// get month from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->month_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get day from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->day_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get hour from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->hour_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get min from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->min_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get sec from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x0a )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->sec_record) = (unsigned char)Ascii_To_int(p_first,num);*/
	
		data_ad = NULL;
		p_first = NULL;
	
	}
	else
	{
		data_ad = NULL;
		p_first = NULL;
	       LOG("ERROR: p_first = NULL .\n");
		fclose(fp);
              return STATUS_ERROR;
	}

      fclose(fp);
      return STATUS_OK;
	  
}

#endif


 #ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

 

/**********************************************************************
* SYMBOL	: WriteUserTrafficToFile
* FUNCTION	: 
* PARAMETER	: WriteUserTrafficToFile
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/07
**********************************************************************/

//static pthread_mutex_t fileoption_mutexForFlu = PTHREAD_MUTEX_INITIALIZER;

int WriteUserTrafficToFile(long long ttvalue)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char buf[100];
	int itest;
	
	
	memset(buf, 0 ,sizeof(buf));

	pthread_mutex_lock(&fileoption_mutexForFlu);

	if ( (file = fopen("/usr/bin/saveUserTraffic.txt", "wb+")) < 0 )
	{
		LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen() error. errno=%s(%d).\n", strerror(errno), errno);
	}

       time(&t);
	time_write = localtime(&t); 

	sprintf(buf, "saveTotalTraffic: %lld, %d-%d-%d, %d:%d:%d\n", ttvalue, time_write->tm_year+1900, time_write->tm_mon+1, time_write->tm_mday, time_write->tm_hour, time_write->tm_min, time_write->tm_sec);
	itest = fwrite(buf, sizeof(char), strlen(buf), file);
	fflush(file);
	fclose(file);
	pthread_mutex_unlock(&fileoption_mutexForFlu);

	return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: ReadUserTrafficFromFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/07
**********************************************************************/

int ReadUserTrafficFromFile(strflu_record *pflu_data)
{
	char *data_ad = NULL;
	char *p_first = NULL;

	FILE *fp;
	char data[200] = "";
	unsigned char num = 0;
	strflu_record *pfluvalue_temp;

	pfluvalue_temp = pflu_data;
	   
	
	if ( (fp = fopen("/usr/bin/saveUserTraffic.txt", "r")) == NULL )
	{
		LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!fludata .txt errno=%s(%d).\n",strerror(errno), errno);
		return STATUS_ERROR;
	}
	
	

	fgets(data, 200, fp);
   
    	LOG("-----------%d---%s------\r\n",strlen(data),data);
		
       if (strlen(data)==0)
       {
	 LOG("ERROR: not found information from the text.txt!\r\n");
	 fclose(fp);
        return STATUS_ERROR;
       }

	// Get flu max from txt file
	p_first = strstr(data,"saveTotalTraffic:");
	num = 0;
	if(p_first != NULL)
	{
		p_first += strlen("saveTotalTraffic:");
		p_first += 1;
		data_ad = p_first;
		
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}

		(pfluvalue_temp->flu_value) = ( long long)Ascii_To_int(p_first,num);
		
		// get year from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->year_record) = (unsigned int)Ascii_To_int(p_first,num);

		// get month from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->month_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get day from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->day_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get hour from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->hour_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get min from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->min_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get sec from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x0a )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->sec_record) = (unsigned char)Ascii_To_int(p_first,num);
	
		data_ad = NULL;
		p_first = NULL;
	
	}
	else
	{
		data_ad = NULL;
		p_first = NULL;
	       LOG("ERROR: p_first = NULL .\n");
		fclose(fp);
              return STATUS_ERROR;
	}

      fclose(fp);
      return STATUS_OK;
	  
}

#endif

 #ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

 

/**********************************************************************
* SYMBOL	: WriteModeToFile
* FUNCTION	: 
* PARAMETER	: WriteModeToFile
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/07
**********************************************************************/

//static pthread_mutex_t fileoption_mutexForFluMode = PTHREAD_MUTEX_INITIALIZER;

int WriteModeToFile(unsigned int  ttvalue)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char buf[100];
	int itest;
	
	
	memset(buf, 0 ,sizeof(buf));

	//pthread_mutex_lock(&fileoption_mutexForFluMode);

	if ( (file = fopen("/usr/bin/saveModeFlag.txt", "wb+")) < 0 )
	{
		LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen() error. errno=%s(%d).\n", strerror(errno), errno);
	}

       time(&t);
	time_write = localtime(&t); 

	sprintf(buf, "saveModeFlag: %d, %d-%d-%d, %d:%d:%d\n", ttvalue, time_write->tm_year+1900, time_write->tm_mon+1, time_write->tm_mday, time_write->tm_hour, time_write->tm_min, time_write->tm_sec);
	itest = fwrite(buf, sizeof(char), strlen(buf), file);
	fflush(file);
	fclose(file);
	//pthread_mutex_unlock(&fileoption_mutexForFluMode);

	return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: ReadModeFromFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/07
**********************************************************************/

int ReadModeFromFile(unsigned int *unPRunMode)
{
	char *data_ad = NULL;
	char *p_first = NULL;

	FILE *fp;
	char data[200] = "";
	unsigned char num = 0;
	unsigned int *unPRunModeTemp;
	
	unPRunModeTemp = unPRunMode;
	   
	
	if ( (fp = fopen("/usr/bin/saveModeFlag.txt", "r")) == NULL )
	{
		LOG("ERROR: ReadFluValueFromFile = OPEN ERROR!fludata .txt errno=%s(%d).\n",strerror(errno), errno);
		return STATUS_ERROR;
	}
	
	

	fgets(data, 200, fp);
   
    	LOG("-----------%d---%s------\r\n",strlen(data),data);
		
       if (strlen(data)==0)
       {
	 LOG("ERROR: not found information from the text.txt!\r\n");
	 fclose(fp);
        return STATUS_ERROR;
       }

	// Get flu max from txt file
	p_first = strstr(data,"saveModeFlag:");
	num = 0;
	if(p_first != NULL)
	{
		p_first += strlen("saveModeFlag:");
		p_first += 1;
		data_ad = p_first;
		
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}

		*unPRunModeTemp = (unsigned int)Ascii_To_int(p_first,num);
		LOG("Debug:   *unPRunModeTemp =  %d .\n", *unPRunModeTemp);

		data_ad = NULL;
		p_first = NULL;
	
	}
	else
	{
		data_ad = NULL;
		p_first = NULL;
	       LOG("ERROR: p_first = NULL .\n");
		fclose(fp);
              return STATUS_ERROR;
	}

      fclose(fp);
      return STATUS_OK;
	  
}

#endif



/**********************************************************************
* SYMBOL	: ReadUpdateRecordFromFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/

int ReadUpdateRecordFromFile(strflu_record *pflu_data)
{
	char *data_ad = NULL;
	char *p_first = NULL;

	FILE *fp;
	char data[200] = "";
	unsigned char num = 0;
	strflu_record *pfluvalue_temp;

	pfluvalue_temp = pflu_data;


    	if ((fp = fopen("/usr/bin/save_updaterecord.txt","r")) == NULL)
    	{
		LOG("ERROR: ReadupdaterecordFromFile = OPEN ERROR!\r\n");
		
    		return STATUS_ERROR;
    	}

	fgets(data, 200, fp);
   
    	LOG("-----------%d---%s------\r\n",strlen(data),data);
		
       if (strlen(data)==0)
       {
	 LOG("ERROR: not found information from the text.txt!\r\n");
	 fclose(fp);
	
        return STATUS_ERROR;
       }

	// Get flu max from txt file
	p_first = strstr(data,"save_updaterecord:");
	num = 0;
	if(p_first != NULL)
	{
		p_first += strlen("save_updaterecord:");
		p_first += 1;
		data_ad = p_first;
		
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}

		(pfluvalue_temp->flu_value) = (long long)Ascii_To_int(p_first,num);
		
		// get year from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->year_record) = (unsigned int)Ascii_To_int(p_first,num);

		// get month from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2d )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->month_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get day from text.txt
		data_ad++;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x2c )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->day_record) = (unsigned char)Ascii_To_int(p_first,num);

		// get hour from text.txt
		data_ad += 2;
		p_first = data_ad;
		num = 0;
		
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		
		(pfluvalue_temp->hour_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get min from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x3a )
		{
			num++;
			data_ad++;
		}
	
		(pfluvalue_temp->min_record) = (unsigned char)Ascii_To_int(p_first,num);
		
		// get sec from text.txt
		data_ad += 1;
		p_first = data_ad;
		num = 0;
		while(*data_ad != 0x0a )
		{
			num++;
			data_ad++;
		}
		
		(pfluvalue_temp->sec_record) = (unsigned char)Ascii_To_int(p_first,num);
	
		data_ad = NULL;
		p_first = NULL;
	
	}
	else
	{
		data_ad = NULL;
		p_first = NULL;
	       LOG("ERROR: p_first = NULL .\n");
		fclose(fp);
              return STATUS_ERROR;
	}

      fclose(fp);
      return STATUS_OK;
	  
}





/**********************************************************************
* SYMBOL	: IsSameDay
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/
int IsSameDay(strflu_record *pflu_time_record,strflu_record *pflu_time_system)
{
	int status = STATUS_ERROR;

	strflu_record *pflu_time1;
	strflu_record *pflu_time2;

	pflu_time1 = pflu_time_record;
	pflu_time2 = pflu_time_system;

	if (pflu_time1->year_record != pflu_time2->year_record)
	{
		return STATUS_ERROR;
	}
	else if (pflu_time1->month_record != pflu_time2->month_record)
	{
		return STATUS_ERROR;
	}
	else if (pflu_time1->day_record != pflu_time2->day_record)
	{
		return STATUS_ERROR;
	}
	else
		return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: Get_CurrentTime(strflu_record *pflu_time)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/

int Get_CurrentTimeSystem(strflu_record *pflu_time)
{
	FILE * file;
	struct tm *pget_time;
	time_t  flu_time;
	strflu_record *p_timesys;

	p_timesys = pflu_time;
	
	

       time(&flu_time);
	pget_time = localtime(&flu_time);

	p_timesys->flu_value = 0;
	p_timesys->year_record =  (pget_time->tm_year+1900);
	p_timesys->month_record =  (pget_time->tm_mon+1);
	p_timesys->day_record =  (pget_time->tm_mday);
	p_timesys->hour_record =  (pget_time->tm_hour);
	p_timesys->min_record =  (pget_time->tm_min);
	p_timesys->sec_record =  (pget_time->tm_sec);

	/*LOG(" Get_CurrentTimeSystem :  time_write->tm_year =%d.\n", pget_time->tm_year+1900);
	LOG(" Get_CurrentTimeSystem :  time_write->tm_mon =%d.\n", pget_time->tm_mon+1);
	LOG(" Get_CurrentTimeSystem :  time_write->tm_mday =%d.\n", pget_time->tm_mday);
	LOG(" Get_CurrentTimeSystem :  time_write->tm_hour =%d.\n", pget_time->tm_hour);
	LOG(" Get_CurrentTimeSystem :  time_write->tm_min =%d.\n", pget_time->tm_min);
	LOG(" Get_CurrentTimeSystem :  time_write->tm_sec =%d.\n", pget_time->tm_sec);*/


	return STATUS_OK;
	
}

/**********************************************************************
* SYMBOL	: reportTrafficStatusProcess(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/02/06
**********************************************************************/
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206


int BaseTimeOutDate(void)
{
	unsigned int ll_usedTimeByUserMintes= 0;
	long long usedTrafficByUser_temp;
	static unsigned char ucTimeCount = 0;

	
	if (ReadEndDateFromServerFile(&ll_usedTimeByUserMintes) == STATUS_ERROR ){

		WriteEndDateFromServerToFile(0);
		ReadEndDateFromServerFile(&ll_usedTimeByUserMintes);
	}

	LOG("Debug:   ll_usedTimeByUserMintes =  %d .\n", ll_usedTimeByUserMintes);

	if (ucTimeCount <= 6){

		ucTimeCount++;
		
	}else{

		ucTimeCount = 0;
	}
	
	
	LOG("Debug:   ucTimeCount =  %d .\n", ucTimeCount);
	
	LOG("Debug:   ll_usedTimeByUserMintes =  %d .\n", ll_usedTimeByUserMintes);
	
	if (ll_usedTimeByUserMintes >= 0){

		if (ll_usedTimeByUserMintes > 0){
			ll_usedTimeByUserMintes--;
		}
		
		LOG("Debug:   ll_usedTimeByUserMintes =  %d .\n", ll_usedTimeByUserMintes);
		
		if (ll_usedTimeByUserMintes == 0){
			
			msqSendToTrafficAndDateYydaemon(NOTE_DATE_OUTOF,0ULL);
			
		}else{
			if ((ucTimeCount == 1) || (ucTimeCount == 6)){

				LOG("Debug:  Send the time of user to yy_daemon .\n");
				if (ucTimeCount == 6){

					ucTimeCount = 0;
				}
				
				msqSendToTrafficAndDateYydaemon(NOTE_SHOW_MIN,(long long)ll_usedTimeByUserMintes);

			}else{

				LOG("Debug:  Don't send the time of user to yy_daemon .\n");
			}

	
		}

		
		WriteEndDateFromServerToFile(ll_usedTimeByUserMintes);
		LOG("Debug:   ll_usedTimeByUserMintes =  %d .\n", ll_usedTimeByUserMintes);
	}else{

		LOG("Debug:   ll_usedTimeByUserMintes is invalid.\n");
		WriteEndDateFromServerToFile(0);

	}

	
	
#if 0

	usedTrafficByUser_temp =0ULL;// hex_to_long(&trafficDataServer.usedTrafficByUser[0],sizeof(trafficDataServer.usedTrafficByUser));
	LOG("Debug:   usedTrafficByUser_temp =  %lld .\n", usedTrafficByUser_temp);
	
	usedTrafficByUser_temp = 0;
	//ll_usedTimeByUserMintes = 14485;
	LOG("8888888888888888888888888888888888 .\n");
	LOG("Debug:   ll_usedTimeByUserMintes =  %d .\n", ll_usedTimeByUserMintes);

	
	usedTimeByUser.day_record =  (unsigned int)(ll_usedTimeByUserMintes /(24* 60)); // day 
	temp =  (unsigned int)(ll_usedTimeByUserMintes % (24* 60)); // min 

	LOG(" temp = %d.\n", temp);

	usedTimeByUser.hour_record =  (unsigned int)(temp / ( 60)); // hour 
	LOG(" usedTimeByUser.hour_record = %d.\n", usedTimeByUser.hour_record);

	usedTimeByUser.min_record =  (unsigned int)(temp % ( 60)); //min
	LOG(" usedTimeByUser.min_record = %d.\n", usedTimeByUser.min_record);

	strGobalstandTime.year_record = 0;
    	strGobalstandTime.month_record = 0;
   	strGobalstandTime.day_record = 0;
   	strGobalstandTime.hour_record = 0;
   	strGobalstandTime.min_record = 0;
  	strGobalstandTime.sec_record = 0;
	
	
	Get_CurrentTimeSystem(&systemCurrentTime);

	LOG(" Debug : Get first time after register net successfull systemCurrentTime.year_record = %d.\n", systemCurrentTime.year_record);
	LOG(" Debug : Get first time after register net successfull systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
	LOG(" Debug : Get first time after register net successfull systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
	LOG(" Debug : Get first time after register net successfull systemCurrentTime.hour_record = %d.\n", systemCurrentTime.hour_record);
	LOG(" Debug : Get first time after register net successfull systemCurrentTime.min_record = %d.\n", systemCurrentTime.min_record);
	LOG(" Debug : Get first time after register net successfull systemCurrentTime.sec_record = %d.\n", systemCurrentTime.sec_record);


	if ( (usedTimeByUser.min_record + systemCurrentTime.min_record ) >= 60){

		systemCurrentTime.hour_record += 1;
		systemCurrentTime.min_record = (usedTimeByUser.min_record + systemCurrentTime.min_record ) - 60;
		LOG(" ssssssssssssssssssssssssssssssssss\n");
		LOG(" Debug : time out date is  :systemCurrentTime.hour_record = %d.\n", systemCurrentTime.hour_record);
		LOG(" Debug : time out date is  :systemCurrentTime.min_record = %d.\n", systemCurrentTime.min_record);
	}else if ((usedTimeByUser.min_record + systemCurrentTime.min_record ) < 60){

		systemCurrentTime.min_record = (usedTimeByUser.min_record + systemCurrentTime.min_record );
		LOG(" aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa  \n");
		LOG(" Debug : time out date is  :systemCurrentTime.hour_record = %d.\n", systemCurrentTime.hour_record);
		LOG(" Debug : time out date is  :systemCurrentTime.min_record = %d.\n", systemCurrentTime.min_record);
	}

//	strGobalstandTime.min_record = systemCurrentTime.min_record;
	

	if ( (usedTimeByUser.hour_record + systemCurrentTime.hour_record ) >= 24){

		systemCurrentTime.day_record += 1;
		systemCurrentTime.hour_record = (usedTimeByUser.hour_record + systemCurrentTime.hour_record ) - 24;
		LOG(" ccccccccccccccccccccccccccccccccccccc  \n");
		LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
		LOG(" Debug : time out date is  :systemCurrentTime.hour_record = %d.\n", systemCurrentTime.hour_record);
	}else if ((usedTimeByUser.hour_record + systemCurrentTime.hour_record ) < 24){

		systemCurrentTime.hour_record = (usedTimeByUser.hour_record + systemCurrentTime.hour_record );
		LOG(" qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq   \n");
		LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
		LOG(" Debug : time out date is  :systemCurrentTime.hour_record = %d.\n", systemCurrentTime.hour_record);
	}

	//strGobalstandTime.hour_record  = usedTimeByUser.hour_record + systemCurrentTime.hour_record;

	if( (systemCurrentTime.month_record == 1) || (systemCurrentTime.month_record == 3) || (systemCurrentTime.month_record == 5) || (systemCurrentTime.month_record == 7) || (systemCurrentTime.month_record == 8) || (systemCurrentTime.month_record == 10) || (systemCurrentTime.month_record == 12) ){

		if ((usedTimeByUser.day_record + systemCurrentTime.day_record ) >= 31){

			systemCurrentTime.month_record += 1;
			systemCurrentTime.day_record = (usedTimeByUser.day_record + systemCurrentTime.day_record ) - 31;
			LOG(" ffffffffffffffffffffffffffffffffffffffffffffffffffffffff   \n");
			LOG(" Debug : time out date is  :systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
			LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
		}else if ((usedTimeByUser.day_record + systemCurrentTime.day_record ) < 31){

			systemCurrentTime.day_record = (usedTimeByUser.day_record + systemCurrentTime.day_record );

			LOG(" bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb   \n");
			LOG(" Debug : time out date is  :systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
			LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
	       }
	}else if (systemCurrentTime.month_record == 2){

		if ((usedTimeByUser.day_record + systemCurrentTime.day_record ) >= 28){

			systemCurrentTime.month_record += 1;
			systemCurrentTime.day_record = (usedTimeByUser.day_record + systemCurrentTime.day_record ) - 28;

			LOG(" mmmmmmmmmmmmmmmmmmmmmmmmmmmmm   \n");
			LOG(" Debug : time out date is  :systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
			LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
		}else if ((usedTimeByUser.day_record + systemCurrentTime.day_record ) < 28){

			systemCurrentTime.day_record = (usedTimeByUser.day_record + systemCurrentTime.day_record );

			LOG(" tttttttttttttttttttttttttttttttttttttttttt   \n");

			LOG(" Debug : time out date is  :systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
			LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
	       }

		//strGobalstandTime.day_record  = usedTimeByUser.hour_record + systemCurrentTime.hour_record;

	}else{

		if ((usedTimeByUser.day_record + systemCurrentTime.day_record ) >= 30){

			systemCurrentTime.month_record += 1;
			systemCurrentTime.day_record = (usedTimeByUser.day_record + systemCurrentTime.day_record ) - 30;

			LOG(" tttttttttttttttttttttttttttttttttttttttttt  00000000000  \n");
			LOG(" Debug : time out date is  :systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
			LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
		}else if ((usedTimeByUser.day_record + systemCurrentTime.day_record ) < 30){

			systemCurrentTime.day_record = (usedTimeByUser.day_record + systemCurrentTime.day_record );

			LOG(" tttttttttttttttttttttttttttttttttttttttttt  1111111111  \n");
			LOG(" Debug : time out date is  :systemCurrentTime.month_record = %d.\n", systemCurrentTime.month_record);
			LOG(" Debug : time out date is  :systemCurrentTime.day_record = %d.\n", systemCurrentTime.day_record);
	       }


	}

	if (systemCurrentTime.month_record > 12){

		systemCurrentTime.year_record += 1;

	}

       strGobalstandTime.year_record = systemCurrentTime.year_record;
    	strGobalstandTime.month_record = systemCurrentTime.month_record;
   	strGobalstandTime.day_record = systemCurrentTime.day_record;
   	strGobalstandTime.hour_record = systemCurrentTime.hour_record;
   	strGobalstandTime.min_record = systemCurrentTime.min_record;
  	strGobalstandTime.sec_record = systemCurrentTime.sec_record;

	LOG(" So the stand time as flow.\n");

	LOG(" Debug :strGobalstandTime.year_record = %d.\n", strGobalstandTime.year_record);
	LOG(" Debug :strGobalstandTime.month_record = %d.\n", strGobalstandTime.month_record);
	LOG(" Debug :strGobalstandTime.day_record = %d.\n", strGobalstandTime.day_record);
	LOG(" Debug :strGobalstandTime.hour_record = %d.\n", strGobalstandTime.hour_record);
	LOG(" Debug :strGobalstandTime.min_record = %d.\n", strGobalstandTime.min_record);
	LOG(" Debug :strGobalstandTime.sec_record = %d.\n", strGobalstandTime.sec_record);

	if (!usedTrafficByUser_temp){

		LOG("Only new order write value to file.\n");
		WriteEndDateFromServerToFile(ll_usedTimeByUserMintes);

	}
	
#endif

	return 1;

}

 int reportTrafficStatusProcess(unsigned int statusReportTimes)
{
	
	long long ll_trafficBuyTotal= 0;
	long long ll_usedTrafficByUserFromServer= 0;
	double getResult;
	int getResult_int = 0;

	int i=0;

	strflu_record TrafficUserValue;

	LOG("Debug:  statusReportTimes =  %d .\n", statusReportTimes);

	if (ReadUserTrafficFromFile(&TrafficUserValue) == STATUS_ERROR ){

		WriteUserTrafficToFile(0);
		ReadUserTrafficFromFile(&TrafficUserValue);
	}

	LOG("Debug:  TrafficUserValue.flu_value = : %lld .\n", TrafficUserValue.flu_value);
	

	ll_trafficBuyTotal = hex_to_long(&trafficDataServer.trafficBuyTotal[0],sizeof(trafficDataServer.trafficBuyTotal));
	ll_usedTrafficByUserFromServer = hex_to_long(&trafficDataServer.usedTrafficByUser[0],sizeof(trafficDataServer.usedTrafficByUser));

	LOG("Debug:   Get trafficDataServer.trafficBuyTotal: %lld .\n", ll_trafficBuyTotal);
	LOG("Debug:   ll_usedTrafficByUserFromServer : %lld .\n", ll_usedTrafficByUserFromServer);


	if ( ll_trafficBuyTotal == 0){

		LOG("Debug:  Default trafficBuyTotal is 0, return directly\n");
		return 0;
	}

	if (ll_usedTrafficByUserFromServer <= TrafficUserValue.flu_value ){
				
		LOG(" Debug: The traffic for use is right form file.  \n");

	}else{
		LOG(" Debug: The traffic for use is not right  form file.  \n");
		TrafficUserValue.flu_value = ll_usedTrafficByUserFromServer;
		WriteUserTrafficToFile(ll_usedTrafficByUserFromServer);
		
	}

	LOG("Debug:  TrafficUserValue.flu_value = : %lld .\n", TrafficUserValue.flu_value);
	
	getResult_int = (int)(100 * (double)TrafficUserValue.flu_value / ll_trafficBuyTotal);

	LOG("Debug:  getResult_int = : %d\n", getResult_int);

	if ((getResult_int  >= 50) && (getResult_int < 70)){

		LOG("Debug:   User data more than 50%.\n");
		msqSendToTrafficAndDateYydaemon(NOTE_SHOW_DATA_RATE,50ULL);
		
	}else if ((getResult_int  >= 70) && (getResult_int < 90)) {
	
		LOG("Debug:   User data more than 70%.\n");
		msqSendToTrafficAndDateYydaemon(NOTE_SHOW_DATA_RATE,70ULL);
		
	}else if( (getResult_int  >= 90) &&  (getResult_int  < 100)){

		LOG("Debug:   User data more than 90%.\n");
		msqSendToTrafficAndDateYydaemon(NOTE_SHOW_DATA_RATE,90ULL);
		
	}else if(getResult_int >= 100){

		LOG("Debug:   User data more than 100%.\n");
		msqSendToTrafficAndDateYydaemon(NOTE_TRAFFIC_OUTOF,0ULL);
		msq_send_flu_to_yydaemon(TrafficUserValue.flu_value,0xaa);
		
		if (statusReportTimes != STATUS_REPORT_COUNT_TIME_60MIN){

			statusReport_Timeout_cb(1);
		}
		isCancelRecoveryNet = true;
		LOG("Debug:   isCancelRecoveryNet = %d \n",isCancelRecoveryNet);
		//qlinkdatanodeCloseWifiProcess();
		notify_DmsSender(0x04);//AT+CFUN=4
		LOG("Debug:  #############Flow end Here#################.\n");
		while(1);

	}else{

		LOG("Debug:   User data less than 50%.\n");

	}
	
	return 1;
}
#endif




static bool flubcount = false;
long long flu_maxValue_current_traffic = 0;
long long flu_maxValue_current = 0;
long long flu_recordValue = 0;
long long flu_net0Value = 0;


#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER
extern Flu_limit_control flucontrol_server;
#endif

#ifdef  FEATURE_ENABLE_FLU_LIMIT
extern bool isgetNptByYydaemon;
#endif
	
void flu_contrl_process(unsigned int modeCount)
{
	strflu_record flu_pre_value;
	strflu_record flu_current_time;
	strflu_record flu_record_time;
	long long flu_maxValue_net0;
	long long flu_maxValue_net0_temp;
	#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER
	unsigned int  flucontrol_data1;
	unsigned int  flucontrol_speed1;
	unsigned int  flucontrol_data2;
	unsigned int  flucontrol_speed2;
	
	#endif
	

	int limitspeedflag = 0;
	static bool isrunflucommand0 = false;
	static bool isrunflucommand1 = false;
	int ret_count = 0;


#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206	
	LOG(" Debug: isBookMode = %d. \n", isBookMode);
#endif

	if (flubcount == false)
	{
		flubcount = true;
		if (ReadFluValueFromFile(&flu_record_time,0) == STATUS_ERROR)
		{
			flu_maxValue_current = 0;
			if (WriteFluValueToFile(flu_maxValue_current,0) == STATUS_ERROR)
			{
				LOG("ERROR: flu_contrl_process.c: ~~~cccccccccc~ WriteFluValueToFile \n");
			}
			
			sleep(2);
			ReadFluValueFromFile(&flu_record_time,0);
		}
		Get_CurrentTimeSystem(&flu_current_time);

		// fix qlinkdatanode restart issue
		if (ReadFluValueFromFile(&flu_pre_value,1) == STATUS_ERROR)
		{
			flu_net0Value = 0;
			LOG("ERROR: flu_contrl_process.c: ~~~ReadFluValueFromFile(&flu_pre_value,1) == STATUS_ERROR \n");
		}
		else
		{
			flu_net0Value = flu_pre_value.flu_value; 
		}

		if (IsSameDay(&flu_record_time,&flu_current_time) == STATUS_ERROR)
		{
			//CleanFluReacord();
			qlinkdatanode_cleanflu_limit();
			msq_send_TestModeisPullWindown_to_yydaemon(0x01);
			isrunflucommand0 = false;
			isrunflucommand1 = false;
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206	

			if(!isBookMode){
				flu_maxValue_current = 0;
			}

#else
			
				flu_maxValue_current = 0;
			
#endif
			
	
			if (WriteFluValueToFile(flu_maxValue_current,0) == STATUS_ERROR)
			{
				LOG("ERROR: flu_contrl_process.c: ~~~dddddd~ WriteFluValueToFile \n");
			}
			
			LOG(" flu_contrl_process.c: ~~~~ IsSameDay(&flu_record_time,&flu_current_time) == STATUS_ERROR \n");	
		}
		else
		{
			LOG("flu_contrl_process.c: ~~~~ IsSameDay is OK \n");
		}

		
	}

	ReadFluValueFromFile(&flu_record_time,0);
	flu_recordValue = flu_record_time.flu_value;
	Get_CurrentTimeSystem(&flu_current_time);
	
	
	if (flu_current_time.hour_record == 0)
	{
		if( (flu_current_time.min_record >= 0) && (flu_current_time.min_record <= FLU_COUNT_VALUE))
		{
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206	

			if(!isBookMode){
				flu_recordValue = 0;
			}

#else
			
				flu_recordValue = 0;
			
#endif
			
			msq_send_TestModeisPullWindown_to_yydaemon(0x01);
			qlinkdatanode_cleanflu_limit();
			isrunflucommand0 = false;
			isrunflucommand1 = false;
			LOG("flu_contrl_process.c: ~~~~ the time is 0:00 \n");
		}
	}

	LOG("flu_contrl_process.c: flu_recordValue =%lld.\n", flu_recordValue);
	

	//20160620 by jackli
	flu_maxValue_net0	= Get_CurrentFluVale();
	flu_maxValue_net0_temp = flu_maxValue_net0;

	LOG("flu_maxValue_net0 =%lld.\n", flu_maxValue_net0);
	LOG("flu_maxValue_net0_temp =%lld.\n", flu_maxValue_net0_temp);
	LOG("flu_net0Value =%lld.\n", flu_net0Value);

	// fix qlinkdatanode restart issue
	if (flu_maxValue_net0 < flu_net0Value )
	{
		flu_net0Value = 0;
	}
	
	flu_maxValue_net0 -= flu_net0Value;
	flu_net0Value = flu_maxValue_net0_temp;
	flu_maxValue_current = (flu_recordValue + flu_maxValue_net0);

	
		
	LOG("  flu_maxValue_net0 =%lld.\n", flu_maxValue_net0);
	LOG("  flu_net0Value =%lld.\n", flu_net0Value);
	LOG("  flu_maxValue_current =%lld.\n", flu_maxValue_current);


	if (WriteFluValueToFile(flu_maxValue_current,0) == STATUS_ERROR)
	{
		LOG("ERROR: flu_contrl_process.c: ~~ffffff~~ WriteFluValueToFile \n");
	}

	LOG("  modeCount =%d.\n", modeCount);
	
 #ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
	if(isBookMode == true){

		WriteUserTrafficToFile(flu_maxValue_current);
		reportTrafficStatusProcess(modeCount);

	}
 
 #endif

#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER

	LOG(" ------ flucontrol_server.flu_limit_data_level1 =%d.\n", flucontrol_server.flu_limit_data_level1);
	LOG(" ------ flucontrol_server.flu_limit_speed_level1 =%d.\n", flucontrol_server.flu_limit_data_level1);
	LOG(" ------ flucontrol_server.flu_limit_data_level1 =%d.\n", flucontrol_server.flu_limit_data_level1);

	flucontrol_data1 = hex_to_int(&flucontrol_server.flu_limit_data_level1[0],4);
	flucontrol_speed1 = hex_to_int(&flucontrol_server.flu_limit_speed_level1[0],4);

	flucontrol_data2 = hex_to_int(&flucontrol_server.flu_limit_data_level2[0],4);
	flucontrol_speed2 = hex_to_int(&flucontrol_server.flu_limit_speed_level2[0],4);

	LOG(" ------ flucontrol_data1 =%d.\n", flucontrol_data1);
	LOG(" ------ flucontrol_data2 =%d.\n", flucontrol_data2);

	LOG(" ------ flucontrol_speed1 =%d.\n", flucontrol_speed1);
	LOG(" ------ flucontrol_speed2 =%d.\n", flucontrol_speed2);

	flucontrol_data1 = flucontrol_data1 * 1024*1024;
	flucontrol_data2 = flucontrol_data2 * 1024*1024;

	LOG(" --s- flucontrol_data1 =%d.\n", flucontrol_data1);
	LOG(" --s- flucontrol_data2 =%d.\n", flucontrol_data2);


#endif

	if ((flu_maxValue_current >= flucontrol_data1) && (flu_maxValue_current < flucontrol_data2) && (flucontrol_data2 != 0) && (flucontrol_data1 != 0))
	{
		LOG(" ------ flucontrol_data1 =%d.\n", flucontrol_data1);
		LOG(" ------ flucontrol_data2 =%d.\n", flucontrol_data2);
		LOG(" ------ flucontrol_speed1 =%d.\n", flucontrol_speed1);
		LOG(" ------ flucontrol_speed2 =%d.\n", flucontrol_speed2);
		LOG(" ------ isrunflucommand0 =%d.\n", isrunflucommand0);
		LOG(" ------ isrunflucommand1 =%d.\n", isrunflucommand1);

	
		
		limitspeedflag = 0xff;
		if (0 != flucontrol_speed1)
		{
			
			if (isrunflucommand0 == false )
			{	
				msq_send_TestModeisPullWindown_to_yydaemon(0x00);
				qlinkdatanode_cleanflu_limit();
				LOG(" DEBUG: clean  limit flu command (speed1).\n");
				
				isrunflucommand0 = true;
				ret_count = qlinkdatanode_flu_limit_No_shell(flucontrol_speed1);
				if (ret_count < 0)
				{	
					LOG(" ERROR: limit flu command is fail times 1.\n");
					qlinkdatanode_cleanflu_limit();
					ret_count = qlinkdatanode_flu_limit_No_shell(flucontrol_speed1);
					if (ret_count < 0)
					{
						LOG(" ERROR: limit flu command is fail times 2.\n");
						qlinkdatanode_cleanflu_limit();
						ret_count = qlinkdatanode_flu_limit_No_shell(flucontrol_speed1);
						if (ret_count < 0)
						{
							LOG(" ERROR: limit flu command is fail times 3.\n");
							qlinkdatanode_cleanflu_limit();
							LOG(" ERROR: limit flu command is fail reboot.\n");
							//yy_popen_call("reboot");
							yy_popen_call("reboot -f");
						}
					}
				}
			}
			
			LOG(" get the limit value is level 1 from server \n");
		}
		else
		{
			LOG(" get the limit value is 0   from server \n");
		}
		
		
	}
	else if ((flu_maxValue_current >= flucontrol_data2) && (flucontrol_data2 != 0) && (flucontrol_data1 != 0))
	{
		LOG(" ------ flucontrol_data1 =%d.\n", flucontrol_data1);
		LOG(" ------ flucontrol_data2 =%d.\n", flucontrol_data2);
		LOG(" ------ flucontrol_speed1 =%d.\n", flucontrol_speed1);
		LOG(" ------ flucontrol_speed2 =%d.\n", flucontrol_speed2);
		LOG(" ------ isrunflucommand0 =%d.\n", isrunflucommand0);
		LOG(" ------ isrunflucommand1 =%d.\n", isrunflucommand1);
		
		limitspeedflag = 0xff;
		if (0 != flucontrol_speed2)
		{	
			
			
			if (isrunflucommand1 == false)
			{	
			       msq_send_TestModeisPullWindown_to_yydaemon(0x00);
				qlinkdatanode_cleanflu_limit();
				LOG(" DEBUG: clean  limit flu command (speed2) .\n");
				
				isrunflucommand1 = true;
				ret_count = qlinkdatanode_flu_limit_No_shell(flucontrol_speed2);
				if (ret_count < 0)
				{	
					LOG(" ERROR: limit flu command is fail times 1.\n");
					qlinkdatanode_cleanflu_limit();
					ret_count = qlinkdatanode_flu_limit_No_shell(flucontrol_speed2);
					if (ret_count < 0)
					{
						LOG(" ERROR: limit flu command is fail times 2.\n");
						qlinkdatanode_cleanflu_limit();
						ret_count = qlinkdatanode_flu_limit_No_shell(flucontrol_speed2);
						if (ret_count < 0)
						{
							LOG(" ERROR: limit flu command is fail times 3.\n");
							qlinkdatanode_cleanflu_limit();
							LOG(" ERROR: limit flu command is fail reboot.\n");
							//yy_popen_call("reboot");
							yy_popen_call("reboot -f");
						}
					}
				}
			}
			
			LOG(" get the limit value is level 2 from server \n");
		}
		else
		{
			LOG(" get the limit value is 0   from server \n");
		}
		
	}
	else 
	{
		limitspeedflag = 0xaa;
		
	}
	
	msq_send_flu_to_yydaemon(flu_maxValue_current,limitspeedflag);
	
}



/**********************************************************************
* SYMBOL	: FluMonitorProcess(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/08/31
**********************************************************************/

 int FluMonitorProcess(void)
{
	int ret = -1;
	
	static long long get_current_fludata = 0;
	static long long get_pre_fludata = 0;

	get_current_fludata = Get_CurrentFluVale();

	LOG("OK:   Get current fludata::: get_current_fludata: %lld .\n", get_current_fludata);
	LOG("OK:   Get get_pre_fludata ::: get_pre_fludata: %lld .\n", get_pre_fludata);

	if (0 == get_pre_fludata){
		ret = 0;
		get_pre_fludata = get_current_fludata;
		LOG("OK:   Get fludata the first time ::: get_pre_fludata: %lld .\n", get_pre_fludata);
		return ret;
	}
	else if(get_current_fludata < get_pre_fludata){
		
		ret = -1;
		LOG("ERROR:   Need to reboot system for fludata is not right .\n");
		//yy_popen_call("reboot");
		yy_popen_call("reboot -f");

	}
	else{

		ret = 0;
		get_pre_fludata = get_current_fludata;
		LOG("OK:   Don't need to reboot system for fludata is right .\n");
	}
	
	return ret;
}


/**********************************************************************
* SYMBOL	: IsRunYy_update(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/06/21
**********************************************************************/

static int IsRunYy_update(void)
{
	int ret = STATUS_ERROR;
	strflu_record isRunYyupdatecurrent_time;
	
	Get_CurrentTimeSystem(&isRunYyupdatecurrent_time);

	LOG("the current time is isRunYyupdatecurrent_time.hour_record = %d \n",isRunYyupdatecurrent_time.hour_record);
	LOG("the current time is isRunYyupdatecurrent_time.min_record = %d \n",isRunYyupdatecurrent_time.min_record);
	
	
	if( (isRunYyupdatecurrent_time.hour_record == 0)||(isRunYyupdatecurrent_time.hour_record == 6)  ||(isRunYyupdatecurrent_time.hour_record == 12)  ||(isRunYyupdatecurrent_time.hour_record == 18))
	{
		if( (isRunYyupdatecurrent_time.min_record <= 10) )
		{
			
			LOG("the current time is after 0 6 12 18 <10 \n");
			ret =  STATUS_ERROR;
			
		}
		else
		{
			LOG("the current time is after 0 6 12 18 > 10 \n");
			ret =  STATUS_OK;
		}
	}
	else if( (isRunYyupdatecurrent_time.hour_record == 23)||(isRunYyupdatecurrent_time.hour_record == 5)  ||(isRunYyupdatecurrent_time.hour_record == 11)  ||(isRunYyupdatecurrent_time.hour_record == 17))
	{
		if( (isRunYyupdatecurrent_time.min_record >= 50) )
		{
			
			LOG("the current time is after 23 5 11 17 >50 \n");
			ret =  STATUS_ERROR;
		}
		else
		{
			LOG("the current time is after 23 5 11 17 < 50 \n");
			ret =  STATUS_OK;
		}
	}
	else
		ret =  STATUS_OK;
		


	return ret;
	
}

static  void status_Loop(void )
{
	int ret;
	int makfd;
	int rd_fd;
	fd_set rfd;
	char buf[2];

	static bool brun_update = false;
	static bool uploadflagtest = false;
	static bool bcount_var = false;
	static bool isareadlylimitcount = false;
	static unsigned int time_counts_for_rs=0;
	static unsigned int time_counts_for_flu=0;
	static unsigned int time_counts_for_flu_monitor=0;
	int read_bytes = -1;
       char recv_buf = 0xff;
       unsigned int flu_maxValue_temp;
	unsigned int uc_updateRecord;
	strflu_record str_updateRecord;

	static unsigned char isgetNptByCount = 0;
	static unsigned char ucRebootCount = 0;
	static unsigned char ucRebootCount02 = 0;
	
	

       struct itimerval new_value, old_value;
       struct timeval it_value;
	   

	for(;;){
				rd_fd = Status_rw_pipe[0];
				makfd = rd_fd+1;
	           		FD_ZERO(&rfd);
				FD_SET(rd_fd,&rfd);
	           		 makfd = rd_fd+1;
				
				it_value.tv_sec = 60;
				it_value.tv_usec = 0;

				
			
				ret= select(makfd,&rfd,NULL,NULL,&it_value);
				if(ret< 0){
					if(errno == EINTR) continue;
					LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: select() error. errno=%s(%d).\n", strerror(errno), errno);
					
					continue;
				}
				else if (ret > 0)
				{
					do{
        				read_bytes = read(rd_fd, &recv_buf, 1);
      					}while(read_bytes == 0 || (read_bytes < 0 && (errno == EAGAIN || errno == EINTR)));
					//LOG(" Socket_qlinkdatanode.c: ~~~~status_Loop: ~~~~ the first~~~~\n");
					bcount_var = true;
					LOG("Socket_qlinkdatanode.c :   recv_buf = %d.\n", recv_buf);
		#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

					LOG("Debug :   isTestMode = %d.\n", isTestMode);

					if (isTestMode){

						msq_send_TestMode_to_yydaemon();
						isCancelRecoveryNet = true;
						LOG("Debug:   isCancelRecoveryNet = %d \n",isCancelRecoveryNet);
						notify_DmsSender(0x04);//AT+CFUN=4
						LOG("Debug:  $$$$$$$$$$$$$$$$$ Flow end Here $$$$$$$$$$$$$$$$$$ n");
						while(1);
					}
		#endif			
		                    
					if (recv_buf == 0x06)
					{
						statusReport_Timeout_cb(0);
						
					}
					if (recv_buf == 0x07)
					{
						//LOG("socket_qlinkdatanode.c: ~ recv_buf == 0x07.\n");  
						qlinkdatanodem_system_call("cd tmp/ && cp qlinkdatanode /usr/bin/ && cp yy_daemon /usr/bin/");
						qlinkdatanodem_system_call("cd usr/bin/ && chmod 777 qlinkdatanode");
						qlinkdatanodem_system_call("cd usr/bin/ && chmod 777 yy_daemon");
					}
					if(uploadflagtest == false)
					{
						uploadflagtest = true;
					}

					if (brun_update == false)
					{
						brun_update = true;
						if (ReadUpdateRecordFromFile(&str_updateRecord) == STATUS_ERROR)
						{

							uc_updateRecord = 0;
							WriteUpdaterRecordToFile(uc_updateRecord);
							ReadUpdateRecordFromFile(&str_updateRecord);
				
						}
						uc_updateRecord = str_updateRecord.flu_value;
						LOG("uc_updateRecord  = %d \n",uc_updateRecord);  

						if (uc_updateRecord >=1)
						{
							LOG("last update fail \n");  
							uc_updateRecord = 0;
							WriteUpdaterRecordToFile(uc_updateRecord);
						}
						else 
						{
							if ((IsRunYy_update() == STATUS_OK) && (recv_buf != 0x06))
							{
								LOG("run yy_update.\n"); 
								qlinkdatanodem_system_call("cd usr/bin/ && ./yy_update");
							}
							else
							{
								LOG("Can't run yy_update dring 0,6,12,18 one day.\n"); 
							}
							
						}

#ifdef	FEATURE_ENABLE_SYSTEM_FOR_PORT_0223

						LOG("iptables  apple/21/22.\n"); 

						qlinkdatanodem_system_call("iptables -I FORWARD -p tcp -m string --string \"apple.com\" --algo kmp -j DROP");

						LOG("iptables  apple/21/22 01.\n"); 
						qlinkdatanodem_system_call("iptables -A FORWARD -p tcp --sport 21 -j DROP"); // CLOSE 21 PORT

						LOG("iptables  apple/21/22 02.\n"); 
						qlinkdatanodem_system_call("iptables -A FORWARD -p tcp --sport 22 -j DROP"); // CLOSE 22 PORT

						LOG("iptables  apple/21/22 end.\n"); 
						
#endif
							
						
					}
					
				}
				else
				{
					if(bcount_var == true)
					{
						time_counts_for_rs++;
						time_counts_for_flu++;
						time_counts_for_flu_monitor++;

						LOG(" Limit speed :isgetNptByYydaemon =%d.\n", isgetNptByYydaemon);
						
						if (isgetNptByYydaemon == true){

							isgetNptByYydaemon = false;
							isgetNptByCount ++;
						}
						
						if (isgetNptByCount >100){

							isgetNptByCount = 1;

						}
						
						LOG(" NTP report :isgetNptByCount =%d.\n", isgetNptByCount);
						LOG(" Status report :time_counts_for_rs =%d.\n", time_counts_for_rs);
						LOG(" Limit speed : time_counts_for_flu =%d.\n", time_counts_for_flu);
						LOG(" Flu montior: time_counts_for_flu_monitor =%d.\n", time_counts_for_flu_monitor);
						
						
						if (time_counts_for_rs == STATUS_REPORT_COUNT_TIME_60MIN) // 
						{
							LOG(" time_counts_for_rs =%d.\n", time_counts_for_rs);
							time_counts_for_rs = 0;
							statusReport_Timeout_cb(0);
						}

						if (isareadlylimitcount == false)
						{
							isareadlylimitcount = true;
							print_cur_gm_time("MY_FLU_CONTROL:: the first~~~");
							LOG(" isgetNptByCount =%d.\n", isgetNptByCount);
							if (isgetNptByCount > 0)
							{
									flu_contrl_process(time_counts_for_rs);
							}
							
						}
						
						if (time_counts_for_flu == FLU_COUNT_TIME_3MIN)
						{
						
#ifdef FEATURE_ENABLE_FLU_LIMIT
							print_cur_gm_time("Limit speed start:");
							LOG(" time_counts_for_flu =%d.\n", time_counts_for_flu);
							LOG(" isgetNptByCount =%d.\n", isgetNptByCount);
							time_counts_for_flu = 0;
							if (isgetNptByCount > 0)
							{
								
								flu_contrl_process(time_counts_for_rs);
								
							}
#endif
						}
						
						if (time_counts_for_flu_monitor == FLU_COUNT_TIME_PINGTEST_5MIN) // 3min
						{
							print_cur_gm_time("FLU Monitor::::");
							LOG("time_counts_for_flu_monitor = %d.\n", time_counts_for_flu_monitor);
							time_counts_for_flu_monitor = 0;
							FluMonitorProcess();
							
						}
						
					}
					else
					{
						LOG("  Socket_qlinkdatanode.c: ~~~~status_Loop: ~~~~ The msg was not received ~~~~\n");
					}
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
				
					LOG(" Debug: isBookMode = %d. \n", isBookMode);
					if(isBookMode){
						LOG(" Debug: Shoud be traffic mode. \n");
						BaseTimeOutDate();
					}
#endif
					

#ifdef FEATURE_ENABLE_REBOOT_DEBUG
					
					LOG("Debug: g_is7100modemstartfail = %d.\n", g_is7100modemstartfail);
					LOG("Debug: g_is7100modem_register_state = %d.\n", g_is7100modem_register_state);

					if (g_is7100modemstartfail == 1){

						ucRebootCount++;
						LOG("Debug: ucRebootCount = %d.\n",ucRebootCount);
						if (ucRebootCount >= 2){
							ucRebootCount = 0;
							LOG("ERROR: auth fail need to reboot.\n");
					       	//yy_popen_call("reboot");
							yy_popen_call("reboot -f");

						}
						
					}else{
						ucRebootCount = 0;
					}
					
					if (g_is7100modem_register_state == 1){

						ucRebootCount02++;
						LOG("Debug: ucRebootCount02 = %d.\n",ucRebootCount02);
						if (ucRebootCount02 >= 5){
							 LOG("Need to reboot system for time out 5 mintes can not register network .\n");
							 ucRebootCount02 = 0;
							//yy_popen_call("reboot");
							yy_popen_call("reboot -f");

						}
						
					}else{
						ucRebootCount02 = 0;

					}
	
 #endif
				}
					
				sleep(5);
			}

	return NULL;
}

static void *initStatusReport(void *user_data)
{
	int ret;
	int Status_rw_fd[2];
	
	
	ret = pipe(Status_rw_fd);
	*((int *)user_data) = ret;

	pthread_mutex_lock(&status_mutex);
    statusStarted = 1;
    pthread_cond_broadcast(&status_cond);
    pthread_mutex_unlock(&status_mutex);

	if(ret < 0){
		LOG(" socket_qlinkdatanode.c: initStatusReport:pipe error.\n");
		return NULL;
	}

	Status_rw_pipe[0] = Status_rw_fd[0];
	Status_rw_pipe[1] = Status_rw_fd[1];
	fcntl(Status_rw_pipe[0],F_SETFL,O_NONBLOCK);
	
    status_Loop();


	return NULL;
}

int startStatusReportThread(void)
{
	int ret;
    pthread_attr_t attr;
    int result;

    statusStarted = 0;
    pthread_mutex_lock(&status_mutex);
	
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&status_parser_tid, &attr, initStatusReport, (void *)(&result));

	
	
	while(statusStarted == 0){
		pthread_cond_wait(&status_cond, &status_mutex);
	}

	pthread_mutex_unlock(&status_mutex);

    if(ret != 0){
		LOG("socket_qlinkdatanode.c: startStatusReportThread: pthread_create() error.\n.");
		return 0;
	}
	if(result < 0){
		LOG("socket_qlinkdatanode.c: startStatusReportThread: initSockThread() error.\n.");
		return 0;
	}
	
    return 1;
}
#endif



