#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h> // poll
#include <sys/file.h> // flock

#include "common_qlinkdatanode.h"
#include "event_qlinkdatanode.h"
#include "at_rsp_qlinkdatanode.h"
#include "qmi_parser_qlinkdatanode.h"
#include "qmi_sender_qlinkdatanode.h"
#include "queue_qlinkdatanode.h"
#include "protocol_qlinkdatanode.h"
#include "feature_macro_qlinkdatanode.h"
#include "at_qlinkdatanode.h"
#include "socket_qlinkdatanode.h" // Udp_Config
#include "msq_qlinkdatanode.h"

#ifndef FEATURE_ENABLE_AT_qlinkdatanode_C_LOG
#define LOG print_to_null
#endif

#define CSQ_REPETITION_TIMES (10)
#define CGREG_REPETITION_TIMES (10)
#define CIPOPEN_REPETITION_TIMES (5)

/******************************External Functions*******************************/
extern void notify_UimSender(char opt);
extern void notify_NasSender(char opt);
extern void notify_DmsSender(char opt);
extern void notify_EvtLoop(const char msg);
extern void push_ChannelNum(const int chan_num);
extern void push_flow_state(const rsmp_state_s_type fs);
extern void push_thread_idx(const Thread_Idx_E_Type tid);
extern void msq_send_online_ind(void);
extern void msq_send_offline_ind(void);
extern void msq_send_pd_rsp(void);
extern void msq_send_custom_at_rsp(void);
extern void msq_send_pd_msg_to_srv(void);
extern void msq_send_qlinkdatanode_run_stat_ind(int evt);
extern void msq_send_imsi_ind(char opt);
extern void msq_send_dev_rst_req(int msg);
extern int gpio_set_value(const unsigned int gpio, const unsigned int value);
extern void gpio_mdmEmodem_startup(void);
extern void gpio_mdmEmodem_shutdown(void);
extern void gpio_mdmEmodem_shutdown_ext(void);
extern void gpio_mdmEmodem_pwr_sw_init(void);
extern void gpio_mdmEmodem_uart_keep_awake(void);
extern int gpio_mdmEmodem_check_if_awake(void);
extern int gpio_mdmEmodem_check_startup(void);
extern void gpio_mdmEmodem_reboot(void);
extern void gpio_mdmEmodem_restart_pwr_ctrl_oper(void);
extern void set_ChannelNum(const int val);
extern int get_ChannelNum(void);
extern Acq_Data_State get_acq_data_state(void);
extern int check_acq_data_state(Acq_Data_State state);
extern void clr_all_rsp_timers(void);
extern void clr_idle_timer(void);
extern void clr_ApduRsp_ext_timer(void);
extern void clr_ID_auth_rsp_timer(void);
extern void clr_remote_USIM_data_rsp_timer(void);
extern void clr_timeout_evt(Evt *tev);
extern void clr_power_down_notification_timer(void);
extern void clr_ate0_rsp_timer(void);
extern void clr_pdp_check_act_routine_timer(void);
extern int check_rsp_timers(void);
extern int check_pdp_act_check_routine_timer(void);
extern int check_idle_timer(void);
extern int check_apdu_rsp_timeout_ext_timer(void);
extern int check_id_auth_rsp_timeout_timer(void);
extern int check_remote_USIM_data_rsp_timeout_timer(void);
extern int check_custom_timer(void);
extern void queue_clr_apdu_node(void);
extern void queue_clr_stat_inf_node(void);
extern int queue_check_if_node_valid(void);
extern void sock_update_reg_urc_state(Reg_URC_State_E_Type state);
extern void print_fmt_data(char *str);
extern int proto_update_flow_state(rsmp_state_s_type state);
extern rsmp_protocol_type proto_get_req_cfg_data_type(rsmp_transmit_buffer_s_type *);
extern void get_dev_and_net_info_for_status_report_session(void);
extern void update_net_info(int tag, int opt1, int opt2, int opt3);
extern void net_info_processor(char pre_cond, Net_Info cur_ni);
extern int write_file(const char *path, unsigned int offset, char *data);
extern Thread_Idx_E_Type get_thread_idx(void);
extern void add_timer_mdmEmodem_idle_timeout(char opt);

extern void notify_RstMonitor(char msg);

/******************************External Variables*******************************/
extern rsmp_transmit_buffer_s_type g_rsmp_transmit_buffer[THD_IDX_MAX+1];
extern rsmp_recv_buffer_s_type g_rsmp_recv_buffer;
extern APDU_Setting APDU_setting;
extern bool isUimPwrDwnMssage;


#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
extern Proc_Msg_At_Cmd_Cfg_S_Type proc_msg_at_config;
#endif

extern rsmp_control_block_type flow_info;

extern Dev_Info sim5360_dev_info;
extern Dev_Info rsmp_dev_info;
extern Net_Info rsmp_net_info;

extern bool isQueued;
extern bool isProcOfInitApp;
extern bool isOnlineMode;
extern bool isMdmEmodemRunning;
extern bool isMdmEmodemPwrDown;
extern bool isFTMPowerOffInd;
extern bool isPwrDwnRspAcq;
extern bool isWaitToPwrDwn;
extern bool isRstReqAcq;
extern bool isRegOffForPoorSig;
extern bool isMifiConnected;
extern bool isRegOn;

#ifdef FEATURE_ENABLE_TIMER_SYSTEM
extern bool isApduRspExtTimerExpired;
extern bool isIdAuthRspTimerExpired;
extern bool isRemoteUsimDataRspTimerExpired;
extern bool isPwrDwnNotificationTimerExpired;
extern bool isAte0RspTimerExpired;
#endif // FEATURE_ENABLE_TIMER_SYSTEM

extern bool isEnableManualApdu;

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
extern bool isMdmEmodemControledByYydaemon;
extern bool isYYDaemonNeedtoSendATCmd;
extern bool isFactoryWriteIMEI5360;
#endif //FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

extern bool PowerUpUsimAfterPowerDown;
extern bool PowerUpUsimAfterRecvingUrcRpt;
extern bool isNeedToSendOutReq0x65;
extern bool isATEvtAct;
extern bool isMdm9215CpinReady;
extern bool isSockListenerRecvPdpActMsg;
extern bool isCipsendFailedV01;

extern int ChannelNum;
#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
extern pthread_rwlock_t ChannelLock;
#else
extern pthread_mutex_t ChannelLock;
#endif

extern bool is1stRunOnMdmEmodem;
extern pthread_mutex_t i1r_mtx;
extern int EvtLoop_2_Listener_pipe[2];
extern pthread_mutex_t el_req_mtx;
extern pthread_cond_t el_req_cnd;
extern bool el_reqed;

extern bool isUimActed;
extern pthread_mutex_t act_uim_mtx;
extern pthread_cond_t act_uim_cond;

extern int dms_inf_acqed;
extern pthread_mutex_t acq_dms_inf_mtx;
extern pthread_cond_t acq_dms_inf_cnd;

extern bool isRstingUsim;
extern bool isUimRstOperCplt;
extern pthread_mutex_t rst_uim_mtx;
extern pthread_cond_t rst_uim_cond;

extern bool isUimPwrDwn;

extern int nas_inf_acqed;
extern pthread_mutex_t acq_nas_inf_mtx;
extern pthread_cond_t acq_nas_inf_cnd;
extern bool isUMTSOnlySet;

extern pthread_mutex_t evtloop_mtx;
extern pthread_mutex_t log_file_mtx;

extern char URC_special_mark_array[MAX_SPEC_URC_ARR_LEN];

extern int StatInfReqNum;
extern unsigned int StatInfMsgSn;

extern pthread_mutex_t apdu_sender_mtx;// 20160304 jackli
extern pthread_cond_t apdu_sender_cond;// 20160304 jackli
extern pthread_mutex_t apdu_mtx; // 20160304 jackli
extern bool isApduSentToMdm;// 20160304 jackli
extern int DMS_wr_fd;

extern int qlinkdatanode_run_stat;

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
extern int BaseTimeOutDate(void);
#endif

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

extern int msqSendToTrafficAndDateYydaemon(char versionf,long long fluRate);


#endif

#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

extern rsp_cclk_time_type rsp_Emodemcclk_ime;

#endif
/******************************Local Variables*******************************/
static const char *InitUnsols[] = {
  "START",
  "+CPIN: READY",
  "OPL UPDATING",
  "PNN UPDATING",
  "SMS DONE",
  "CALL READY",
  "PB DONE"
};

static const int InitUnsolLen[] = {
  sizeof("START"),
  sizeof("+CPIN: READY"),
  sizeof("OPL UPDATING"),
  sizeof("PNN UPDATING"),
  sizeof("SMS DONE"),
  sizeof("CALL READY"),
  sizeof("PB DONE")
};

static int timeoutCount = 0;
static int CSQCount = 0;
static int CGREGCount = 0;
static int CIPOPENCount = 0;
static const char PeerClosedUnsol[] = {"+CIPCLOSE: 0,1"};
static const char NetClosedUnsol[] = {"+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY"};


static const char *ChkNetAT[] = {
  "ATE0",
  "AT+CSQ",
  "AT+CGREG?",
  "AT+CNMP?",
  "AT+CNMP=2"
};
 


static const int ChkNetATLen[] = {
  sizeof("ATE0"),
  sizeof("AT+CSQ"),
  sizeof("AT+CGREG?"),
  sizeof("AT+CNMP?"),
  sizeof("AT+CNMP=2")
};

static const char *ConfAT[] = {
  "AT",
  "AT+IFC=2,2",
  "AT+IPR=460800",
  "AT"
};

static const int ConfATLen[] = {
  sizeof("AT"),
  sizeof("AT+IFC=2,2"),
  sizeof("AT+IPR=460800"),
  sizeof("AT")
};

static char *AcqDataAT[] = {
  "AT", 
  "AT+NETOPEN",

  #ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
  
  "AT", 
  "AT+CIPCLOSE=",

  
  #endif
  
  "AT+CIPOPEN",
  "AT+CIPSEND",
  "AT+CGSN",
  "AT+COPS=3,2",
  "AT+COPS?",
  
  #ifdef  FEATURE_ENABLE_CPOL_CONFIG
   "AT+CPOL=",
   "AT+CPOL=",
  #endif

   #ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG
    "AT+CCLK?",
  #endif
  
  "AT+CGREG=2",
  "AT+CGREG?",

 
  
  "AT+CGREG=0",
  "AT+CATR=3",
  "AT+CATR=0"
};

static char device_path[MAX_DEV_PATH_CHARS] = {'\0'};
//NOTE: At first, server_addr contains whole server address including port string.
//           Then it will only contain server IP address.

static char ATBuf[AT_BUF_LEN];
static char *ATBufCur = ATBuf;
static char *line_head = ATBuf; //Add by rjf, not original

//Funtion:
//If mdmEmodem startup messages are all released by mdmEmodem(no matter received or not), 
//it will be set false.
static bool isProcOfRecvInitInfo = true;
//Function:
//Once one piece of mdmEmodem startup messages have been caught by app, it will be set true.
static bool isInitInfoRecved = false;
static int isRetryToRecv = 0;

static int URC_header_len = 0;
//  Recv beginning data and store them into recv_tmp_buf.
static char recv_tmp_buf[128]; 
static char URC_data[50];
static int URC_header_len_of_remote_UIM_data = 0;
//If a TCP URC header seperates apart to several lines, it will be used to store last recv_len value.
//It will be updated outside reader_recv_start().
static int last_recv_len = 0;

//Description:
//According to acq_data_state or suspend_state, it is for AT+CIPOPEN or AT+CIPCLOSE.
static bool isWaitRspError = false;
//Description:
//0x01: CIPERROR: 0,2 recved.
//0x02: CIPERROR: 0,4 recved.
//0x03: +IP ERROR: Network is already opened recved.
static char error_flag = 0x00;

#ifdef FEATURE_CONFIG_SIM5360
static bool isSIM5360BasicInfoAcqed = false;
static char *mfg_inf_buf = NULL;
static int mfg_inf_buf_offset = 0;
static bool isSIM5360PdpContextSet = false;
static bool isSIM5360PdpContAcqed = false;
static char *pdp_ctx_inf_buf = NULL;
static int pdp_ctx_inf_buf_offset = 0;
#endif // FEATURE_CONFIG_SIM5360

//Description:
//	Mark if the rsp string "ERROR" found when "+NETCLOSE: 9" recved.
static bool isNetclose9Recved = false;

//Description:
//		In acq_data_state of ACQ_DATA_STATE_CIPSEND_1, URC rpt "IPCLOSE: X" is recved. Then this bool var
//	will be set true to wait for sebsequent URC rpt "+CIPERROR: Y" to avoid checking ChannelNum.
static bool isWaitForRptCIPERROR = false;

//Description:
//		If this bool var is false, it doesn't mean the USIM of mdmEmodem is not available. It is applied for encountering the
//	"+CGREG: 0" or "+CGREG: 2" when app is checking registration of mdmEmodem.
static bool isSIM5360CpinReady = false;

#ifdef FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED
static bool isMdmEmodemReattachedWhenRegFailed = false;
#endif // FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED

#ifdef FEATURE_ENABLE_qlinkdatanode_RUN_STAT_IND
//Description:
//		If "AT+CSQ?" is executed and rssi of rsp is less than 4, app will send a notificaiton to 
//	process yy_daemon about it. And this var is to reduce redundant notifications to yy_daemon.
static bool isPoorSigNotified = false;
#endif

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
//Description:
//		If "AT+CGREG?" is executed and reg stat of rsp is neither 1 nor 5, app will send a notificaiton to 
//	process yy_daemon about it. And this var is to reduce redundant notifications to yy_daemon.
static bool isNotRegNotified = false;
//Description:
//		If "AT+NETOPEN" is executed and pdp is not activiated then, app will send a notificaiton to 
//	process yy_daemon about it. And this var is to reduce redundant notifications to yy_daemon.
static bool isPdpNotActNotified = false;
//Description:
//		If "AT+CIPOPEN=..." is executed and serv conn is not built properly, app will send a notificaiton to 
//	process yy_daemon about it. And this var is to reduce redundant notifications to yy_daemon.
static bool isServDisconnNotified = false;
#endif

static pthread_mutex_t standby_switch_mtx = PTHREAD_MUTEX_INITIALIZER;

//Description:
//This is var is set true when no available SIM card for mdmEmodem.
bool isNotNeedIdleTimer = false;

#ifdef FEATURE_ENABLE_REQ_RETRAN_SYS_FOR_ID_AUTH_REJ
static int id_auth_req_retran_times = 0;
#endif

/******************************Global Variables*******************************/

unsigned char g_qlinkdatanode_run_status_01= 0x00;
unsigned char g_qlinkdatanode_run_status_02 = 0x00;



bool g_qlinkdatanode_run_status_03 = false;


bool g_disable_send_downloadSimcard_req = false;

int g_isEmodemcipopen_vaid = 0;
bool g_isEmodemipclose_report = false;
bool g_isat_command_AT = false;
// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

static unsigned char  g_EmodemUdpTcp_socket_linkCount = 2;
bool g_EmodemUdp_socket_link = false;
bool g_EmodemTcp_socket_link = false;

bool g_isip1_use = false;
bool g_isip2_use = false;

bool g_EmodemUdp_flag= false;
bool g_EmodemUdp_link_server_state = false;
bool g_enable_EmodemUdp_link = true;
bool g_enable_EmodemTCP_link = true;
static unsigned char g_enable_EmodemUdp_linkcount = 0;
char Udp_IP_addr[16]={0};
unsigned int udp_server_port = 0;

char Udp_IP_addr_default1[16]={0};
unsigned int udp_server_port_default1 = 0;

char Udp_IP_addr_default2[16]={0};
unsigned int udp_server_port_default2 = 0;

char Udp_IP_addr_default3[16]={0};
unsigned int udp_server_port_default3 = 0;

unsigned int tcp_server_port = 0;
char tcp_IP_addr[16]={0};

unsigned int tcp_server_port_2 = 0;
char tcp_IP_addr_2[16]={0};

unsigned int tcp_server_port_update = 0;
char tcp_IP_addr_update[16]={0};


unsigned char apnMessageBuff[200]={0};


 unsigned char g_enable_TCP_IP01_linkcount = 0;
 unsigned char g_enable_TCP_IP02_linkcount = 0;

 bool g_IdcheckRspis_F3 = false;

#endif

#ifdef FEATURE_ENABLE_F9_CIPCLOSE	

	bool g_closesocket_F9 = false;

#endif

 #ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
  
//  int g_Emodemcipopen_link_mode_udp = -1;
  
  #endif

 unsigned char g_record_ID_timeout_conut = 0;


#ifdef  FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG


unsigned char g_SoftcardFlowState = SOFT_CARD_STATE_FLOW_DEFAULT;
char keybuf[8] = {0};
unsigned int  g_randGetkey = 0;

char keybuf_des[8] = {0};
char server_des_ki[16] = {0};
char server_des_opc[16] = {0};

//char server_des_ki_backup[16] = {0};

#endif

 unsigned char g_is7100modemstartfail = 0;

#ifdef FEATURE_ENABLE_CONTRL_EmodemREBOOT_0117DEBUG

 unsigned char g_is7100modem_register_state = 0;

#endif

char RespArray[256] = {0};

char IP_addr[16]= {0};
int server_port = 0;

#ifdef FEATURE_ENABLE_TIMER_SYSTEM
Evt ate0_rsp_timeout_evt;
Evt ApduRsp_timeout_ext_evt;
Evt ID_auth_rsp_timeout_evt;
Evt remote_USIM_data_rsp_timeout_evt;
Evt power_down_notification_timeout_evt;

Evt Idle_Timer_evt;
Evt pdp_act_check_routine_evt;
Evt Custom_Timer_evt;
#endif //FEATURE_ENABLE_TIMER_SYSTEM

At_Config at_config = 
    {-1, 
    {NULL, NULL, -1, -1, false, {0, 0}, NULL, NULL},
#ifdef FEATURE_ENABLE_TIMER_SYSTEM
    {NULL, NULL, -1, -1, false, {0, 0}, NULL, NULL},
#endif
    {-1, "\0", "\0", -1, 0}, 
  };

Chk_Net_State chk_net_state = CHK_NET_STATE_IDLE;
Conf_State uartconfig_state = CONF_STATE_IDLE;
Acq_Data_State acq_data_state = ACQ_DATA_STATE_IDLE;
pthread_mutex_t acq_data_state_mtx = PTHREAD_MUTEX_INITIALIZER;
Suspend_Mdm_State suspend_state = SUSPEND_STATE_IDLE;

#ifdef FEATURE_CONFIG_SIM5360
Acq_Mfg_Inf_State_E_Type acq_mfg_inf_state = ACQ_MFG_INF_STATE_IDLE;
Check_Pdp_Ctx_State_E_Type check_pdp_ctx_state = CHECK_PDP_CTX_STATE_MIN;
Set_Pdp_Ctx_State_E_Type set_pdp_ctx_state = SET_PDP_CTX_STATE_MIN;
#endif

//Description:
//	Once "+IPCLOSE: 0,1" recved, it will be set true to control r_suspendMdmHdlr().
//	It is for mdmEmodem.
bool isServConnClosed = false;

//Description:
//It is for 2nd-or-more-time ID auth msg.
bool isSendIdAfterAcqIMSI = false;

//Description:
//	Each time wake up mdmEmodem and send out a dequeued req, it need to be after sending
//	out ID auth msg to server. If such a dequeued req existed, isNeedToDequeueAfterIdAuth 
//	will be set true.
//bool isNeedToDequeueAfterIdAuth = false;

//Description:
//	It tells app to upload status info to server. 
bool isNeedToNotifyServCondVar = false;
//Description:
//	It tells app to send ID authentication msg to server.
bool isNeedToSendIDMsg = false;

bool isWaitToSusp = false;
//Description:
//When app need to cancel the suspension of mdmEmodem, this bool var will be set true.
bool isTermSuspNow = false;
//Description:
//When app is trying to recover the server connection, like registration has fell off, ChannelNum equals
// 2 (i.e. The app is suspending mdmEmodem at this moment.). Then this bool var will be set true to help restore
//the server connection of mdmEmodem.
bool RcvrMdmEmodemConnWhenSuspTerm = false;
//Description:
//If mdmEmodem is at standby mode, the operation of waking up it and building up the server connection is called
//"Resume".
//If mdmEmodem is awake, the operation of restoring the server connection is called "Recover" (i.e. rcv.).
bool isWaitToRcvr = false;
//Description:
//Its typical operation is that terminate the current recovering process and start a new recovering process. (Because 
//app encountered a registration fall during the go-to-terminate recovering process .)
bool isTermRcvrNow = false;
//Description:
//When app recved the inds of that reg is searching or sth, it needs to recover the serv conn of mdmEmodem. But at this moment,
//the isRegOffForPoorSig or some other related vars might not be set true. In order to recover the serv conn after the normal 
//process of suspension, this bool var will be true. And it will be set false in r_suspendMdmHdlr when isRegOffForPoorSig equals 
//true.
bool RcvrMdmEmodemConn = false;

#ifdef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
Time_Zone_S timezone_qlinkdatanode;
#endif

#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER
Flu_limit_control flucontrol_server;
#endif

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
trafficFromServercontrol trafficDataServer;
bool isBookMode= false;
bool isCancelRecoveryNet= false;
#endif


#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

bool isTestMode= false;
bool isPdpActive= false;

#endif

//Description:
//No matter if APDU is cmd or rsp, if this var is defined, they will be all dumped.
//If APDU rsp timed out and APDU was sent out, recved 1 APDU rsp would be dumped.
//And this would be only one invalid APDU rsp after APDU rsp timed out.
bool isDumpApdu = false;

//Description:
//It is for the situation where ChannelNum is -1 or 4 in ApduRsp_timeout_ext_cb().
bool MdmEmodemIsRcvrWhenSetPd = false;
//Description:
//It is for syncing sock thread and at thread.
pthread_mutex_t pd_mtx = PTHREAD_MUTEX_INITIALIZER;

#ifdef FEATURE_ENABLE_3GONLY

	bool isneedtoset3Gonly = false;

#endif

#ifdef FEATURE_ENABLE_4G3GAUTO_FOR_US

	bool isneedtoset4G3Gauto = false;

#endif


	unsigned int MdmEmodemCountryType = 0;
       unsigned int MdmEmodemMCCMNC= 0;
	
	
#ifdef  FEATURE_ENABLE_CPOL_CONFIG

	unsigned char atCmdCpolbuf[100];

typedef struct{
	char   *psmcc;
	char   *psmnc;
	char *pscountry;
	unsigned char ucnet_quantity;
}ST_NET_TPYE;

typedef struct{
	unsigned char   cbit;
	char   *pcops;
}ST_EmodemDEFAULT_CONFIG;

ST_EmodemDEFAULT_CONFIG cops_Emodem_list[]={
	{1,"20404"},{2,"26202"},{3,"23415"},{4,"50503"},{5,"20810"},
	{6,"22210"},{7,"23201"},{8,"65501"},{9,"21401"},{10,"53001"},
	
	{11,"22801"},{12,"27201"},{13,"310410"},{14,"45005"},{15,"46001"},
	{16,"40420"},{17,"40411"},{18,"40427"},{19,"40405"},{20,"40446"},
	
	{21,"40443"},{22,"40430"},{23,"26001"},{24,"44010"},{25,"20601"},
	{26,"24201"},{27,"27077"},{28,"26801"},{29,"302220"},{30,"302610"},

	{31,"60202"},{32,"24008"},{33,"23801"},{34,"52505"},{35,"52099"},
	{36,"51011"},{37,"72405"},{38,"25001"},{39,"21670"},{40,"23003"},
	
	{41,"28602"},{42,"63902"},{43,"24405"},{44,"20205"},{45,"50219"},
	{46,"334020"},{47,"42501"},{48,"60402"},{49,"41302"},{50,"22601"},
	
	{51,"27801"},{52,"722310"},{53,"Emodem2"},{54,"51503"},{55,"29340"},
	{56,"71610"},{57,"21902"},{58,"23101"},{59,"42702"},{60,"41902"},
	
	{61,"22005"},{62,"42602"},{63,"46692"},{64,"28401"},{65,"45205"},
	{66,"748010"},{67,"714003"},{68,"27402"},{69,"27602"},{70,"42001"},

	{71,"732123"},{72,"45403"},{73,"73001"},{74,"25501"},{75,"42403"},
	

};

ST_NET_TPYE snetconfig_Emodem[]={

	       
	

        {"457","03","Lao People's Democratic Republic",2},//Row Tier 1
	 {"457","08","Lao People's Democratic Republic",2},//RoW Tier 5


         {"525","05","Singapore",3},// StarHub Preferred Partners
         {"525","03","Singapore",3},//  M1  Row Tier 1
	  {"525","01","Singapore",3},//SingTel RoW Tier 2


          
		  // {"310","470","Guam",3},//Row Tier 5
		  // {"310","140","Guam",3},//Row Tier 5
		  // {"310","370","Guam",3},//RoW Tier 5
          



          {"454","03","Hong Kong",8},//Preferred Partners
          {"454","04","Hong Kong",8},//Preferred Partners
          {"454","16","Hong Kong",8},//Row Tier 1
	   {"454","19","Hong Kong",8},//RoW Tier 1
	   {"454","06","Hong Kong",8},//Row Tier 1
	   {"454","12","Hong Kong",8},//RoW Tier 2
          {"454","00","Hong Kong",8},//Row Tier 5
	   {"454","10","Hong Kong",8},//RoW Tier 5


         
	   {"460","00","China",2},//Preferred Partners
	   {"460","01","China",2},//RoW Tier 1



          
	   {"286","02","Turkey",3},//Vodafone networks
          {"286","03","Turkey",3},//RoW Tier 1
	   {"286","01","Turkey",3},//RoW Tier 3


          {"250","01","Russian Federation",6},//Preferred Partners
          {"250","02","Russian Federation",6},//Row Tier 3
          {"250","99","Russian Federation",6},//Row Tier 5
          {"250","35","Russian Federation",6},//Row Tier 5
	   {"250","03","Russian Federation",6},//Row Tier 5
	   {"250","39","Russian Federation",6},//RoW Tier 5


          
		 
            {"302","610","Canada",6},//Preferred Partners
            {"302","720","Canada",6},//Preferred Partners
            {"302","780","Canada",6},//Preferred Partners
	     {"302","220","Canada",6},//Preferred Partners
	     {"302","500","Canada",6},//RoW Tier 1
	     {"302","320","Canada",6},//Row Tier 5


         /*   {"310","380","United States",23},//Preferred Partners
		   {"310","410","United States",23},//Preferred Partners
		   {"310","260","United States",23},//Preferred Partners
		   {"310","160","United States",23},//Preferred Partners
		   {"310","200","United States",23},//Preferred Partners
		   {"310","610","United States",23},//Preferred Partners
            {"310","210","United States",23},//Preferred Partners
            {"310","220","United States",23},//Preferred Partners
		   {"310","230","United States",23},//Preferred Partners
		   {"310","240","United States",23},//Preferred Partners
		   {"310","250","United States",23},//Preferred Partners
		    {"310","270","United States",23},//Preferred Partners
		   {"310","490","United States",23},//Preferred Partners
		   {"310","420","United States",23},//RoW Tier 1
		   {"311","040","United States",23},//Row Tier 1
		    {"310","690","United States",23},//Row Tier 1
            {"311","370","United States",23},//RoW Tier 3
            {"310","050","United States",23},//RoW Tier 3
            {"310","020","United States",23},//RoW Tier 3
		   {"310","450","United States",23},//Row Tier 3
		   {"310","720","United States",23},//Row Tier 5
            {"311","530","United States",23},//Row Tier 5
		    {"311","330","United States",23},//Row Tier 5*/



            
            {"530","01","New Zealand",2},//Vodafone networks
	     {"530","05","New Zealand",2},//RoW Tier 1


           {"505","03","Australia",3},//Vodafone networks
           {"505","02","Australia",3},//RoW Tier 2
	    {"505","01","Australia",3},//RoW Tier 5



           /*{"572","01","Maldives",2},//Row Tier 5
		   {"472","02","Maldives",2},//RoW Tier 5*/



           {"425","01","Israel",3},//RoW Tier 1
	    {"425","03","Israel",3},//RoW Tier 1
           {"425","02","Israel",3},//RoW Tier 3


            {"456","01","Cambodia",3},//RoW Tier 1
            {"456","06","Cambodia",3},//RoW Tier 1
            {"456","09","Cambodia",3},//RoW Tier 3



             {"413","02","Sri Lanka",5},//Preferred Partners
             {"413","03","Sri Lanka",5},//RoW Tier 2
             {"413","05","Sri Lanka",5},//RoW Tier 5
	      {"413","08","Sri Lanka",5},//RoW Tier 5
	      {"413","01","Sri Lanka",5},//RoW Tier 5


           
	       {"452","02","Viet Nam",4},//Preferred Partners
		{"452","05","Viet Nam",4},//RoW Tier 1
              {"452","07","Viet Nam",4},//RoW Tier 5
		{"452","04","Viet Nam",4},//RoW Tier 5
		  
		   

            {"520","04","Thailand",4},//Preferred Partners
	     {"520","99","Thailand",4},//Preferred Partners
            {"520","01","Thailand",4},//RoW Tier 3
	     {"520","03","Thailand",4},//RoW Tier 3




           {"502","13","Malaysia",5},//Preferred Partners
           {"502","19","Malaysia",5},//Preferred Partners
           {"502","16","Malaysia",5},//RoW Tier 1
	    {"502","12","Malaysia",5},//RoW Tier 5
           {"502","18","Malaysia",5},//RoW Tier 5



         /* {"450","02","Korea, Republic Of",3},//RoW Tier 1
		  {"450","08","Korea, Republic Of",3},//RoW Tier 1
		  {"450","05","Korea, Republic Of",3},//RoW Tier 1*/




            /* {"440","10","Japan",2},//Preferred Partners
		     {"440","20","Japan",2},//RoW Tier 1*/

           
             {"455","00","Macau",3},//RoW Tier 1
	      {"455","03","Macau",3},//RoW Tier 5
	      {"455","01","Macau",3},//RoW Tier 5



             {"466","92","Taiwan, Province of China",3},//Preferred Partners
	      {"466","01","Taiwan, Province of China",3},//RoW Tier 5
	      {"466","97","Taiwan, Province of China",3},//RoW Tier 5
#if 0
		//AoZhou 
		{"505","03","Australia",3}, //Vodafone networks
		{"505","02","Australia",3}, // Row Tier 2
		{"505","01","Australia",3}, // Row Tier 5
#endif 
/*******************************************Asian Start***********************************************/
		//XiangGang
		/*{"454","03","HongKong",6}, //preferred partners 3 
		{"454","04","HongKong",6}, //preferred partners Hutchison
		{"454","06","HongKong",6},//Row Tier 1 SmarTone
		{"454","19","HongKong",6},//Row Tier 1 PCCW
		{"454","00","HongKong",6},//Row Tier 5 CSL
		{"454","10","HongKong",6},//Row Tier 5 CSL*/

		/*{"454","19","HongKong",6},//Row Tier 1 PCCW
		{"454","06","HongKong",6},//Row Tier 1 SmarTone
		{"454","04","HongKong",6}, //preferred partners Hutchison
		{"454","00","HongKong",6},//Row Tier 5 CSL
		{"454","10","HongKong",6},//Row Tier 5 CSL
		{"454","03","HongKong",6}, //preferred partners 3 */

//YinNi
		{"510","01","Indonesia",4},//Row Tier 2 Indosat
		{"510","89","Indonesia",4},//Row Tier 1 3
		{"510","11","Indonesia",4},//Row Tier 1 Excelcom
		{"510","10","Indonesia",4},//Row Tier 5 Telkomsel


#if 0
		//RiBen
		{"440","10","Japan",2}, //preferred partners DoCoMo
		{"440","21","Japan",2},//Row Tier 1 SoftBank 
		//AoMen
		{"450","00","Macau",3},
		{"450","01","Macau",3},
		{"450","03","Macau",3},

		//FeiLiBin
		{"515","03","Philippines",3},//preferred partners SMART GOLD
		{"515","02","Philippines",3},//Row Tier 2 Globe
		{"515","05","Philippines",3},//Row Tier 5 Sun Cellular
		
		//YinNi
		{"510","89","Indonesia",4},//Row Tier 1 3
		{"510","11","Indonesia",4},//Row Tier 1 Excelcom
		{"510","01","Indonesia",4},//Row Tier 2 Indosat
		{"510","10","Indonesia",4},//Row Tier 5 Telkomsel
		
		//MaLaiXiYa
		{"502","13","Malaysia",5},//preferred partners Celcom
		{"502","19","Malaysia",5},//preferred partners Celcom
		{"502","16","Malaysia",5},//Row Tier 1 DiGi
		{"502","12","Malaysia",5},//Row Tier 5 Maxis-Mobile 
		{"502","18","Malaysia",5},//Row Tier 5 U-Mobile 
		
		//HanGuo
		{"455","02","Korea",3}, //Row Tier 1 KT 
		{"455","05","Korea",3}, //Row Tier 1 SK
		{"455","08","Korea",3},//Row Tier 1 KT

		//MaErDaiFu
		{"472","01","Maldives",2}, //Row Tier 5 DhiMobile
		{"472","02","Maldives",2}, //Row Tier 5 Wataniya
		//ZhongGuo
		{"460","00","China",3},
		{"460","01","China",3},
		{"460","02","China",3},

#endif
#if 0
		
		//YueNan		
		{"452","02","Viet Nam",4},//preferred partners VinaPhone
		{"452","02","Viet Nam",4},//Row Tier 1 VietNaMobile
		{"452","04","Viet Nam",4},//Row Tier 5 BeeLine
		{"452","07","Viet Nam",4},//Row Tier 5 Viettel
		//XinJiaPo
		{"525","05","Singapore",3},//preferred partners Star Hub
		{"525","03","Singapore",3},//Row Tier 1 M1
		{"525","01","Singapore",3},//Row Tier 2 SingTel
		//TaiWan
		{"466","92","Taiwan",3},//preferred partners Chungwa
		{"466","01","Taiwan",3},//Row Tier 5 Far EasTone
		{"466","97","Taiwan",3},//Row Tier 5 TaiWan mobile
		//TaiGuo
		{"520","04","Thailand",4},//preferred partners RealFuture
		{"520","99","Thailand",4},//preferred partners TrueMove
		{"520","01","Thailand",4},//Row Tier 3 AIS
		{"520","03","Thailand",4},//Row Tier 3 AIS 

		//TuErQi
		{"286","02","TurKey",3},//Vodafone networks Vodafone
		{"286","03","TurKey",3},//Row Tier 1 AVEA
		{"286","01","TurKey",3},//Row Tier 3 TurKcell
/*******************************************Asian End***********************************************/
/******************************************Europe Start************************************************/

		//YingGuo
		{"234","15","United Kingdom",4},//Vodafone networks Vodafone
		{"234","10","United Kingdom",4},//EU28+2 others O2
		{"234","30","United Kingdom",4},//EU28+2 others T-Mobile
		{"234","33","United Kingdom",4},//EU28+2 others Everything everywhere

		//LuoMaNiYa
		{"226","01","Romania",4},//Vodafone networks Vodafone
		{"226","03","Romania",4},//EU28+2 others Cosmote
		{"226","05","Romania",4},//EU28+2 others DigiMobil
		{"234","10","Romania",4},//EU28+2 others Orange

		//FaGuo
		{"208","10","France",3},//preferred partners RealFuture
		{"208","01","France",3},//EU28+2 others Orange
		{"208","20","France",3},//EU28+2 others Bouygues

		//BoLan
		{"260","01","Poland",4},//preferred partners Plus
		{"260","02","Poland",4},//EU28+2 others Era
		{"260","03","Poland",4},//EU28+2 others Orange
		{"260","06","Poland",4},//EU28+2 others Play

		//RuiShi
		{"228","01","Switzerland",3},//preferred partners Swisscom
		{"228","03","Switzerland",3},//EU28+2 others Orange
		{"228","02","Switzerland",3},//EU28+2 others Sunrise

		//RuiDian
		{"240","04","Sweden",3},//preferred partners Telenor
		{"240","05","Sweden",3},//EU28+2 others Tele 2
		{"240","01","Sweden",3},//EU28+2 others TeliaSonera

		//YiDaLi
		{"222","10","Italy",2},//Vodafone networks Vodafone
		{"222","88","Italy",2},//EU28+2 others Wind
		//Fan di gang
		/*{"222","10","Vatican City",2},//Vodafone networks Vodafone
		{"222","88","Vatican City",2},//EU28+2 others Wind*/

		//
		{"292","01","San Marino",1},//Row Tier 1  San Marino Telecom


		//DeGuo
		{"262","02","Germany",4},//Vodafone networks Vodafone
		{"262","01","Germany",4},//EU28+2 others T-Mobile
		{"262","03","Germany",4},//EU28+2 others E-Plus
		{"262","07","Germany",4},//EU28+2 others O2

		//MoNaGe
		{"208","10","Monaco",2},//preferred partners SFR
		{"208","01","Monaco",2},//EU28+2 others Orange

		//LaTuoWeiYa
		{"247","05","Latvia",3},//preferred partners Bite Latvija
		{"247","01","Latvia",3},//EU28+2 others LMT
		{"247","02","Latvia",3},//EU28+2 others Tele 2

		//XiLa
		{"202","05","Greece",3},//Vodafone networks Vodafone
		{"202","01","Greece",3},//EU28+2 others Cosmote
		{"202","10","Greece",3},//EU28+2 others Wind

		//A Er Ba Ni Ya
		{"276","02","Albania",1},//Vodafone networks Vodafone

		

		//NuoWei
		{"242","01","Norway",3},//preferred partners Telenor
		{"242","02","Norway",3},//EU28+2 others Netcom
		{"242","05","Norway",3},//EU28+2 others NetWork Norway

		//Sai er wei ya
		{"220","05","Serbia",3},//preferred partners VIP
		{"220","03","Serbia",3},//Row Tier 1  mts
		{"220","01","Serbia",3},//Row Tier 2 Telenor

		//BaoJiaLiYa
		{"284","01","Bulgaria",3},//preferred partners M-Tel
		{"284","03","Bulgaria",3},//EU28+2 others Vivacom
		{"284","05","Bulgaria",3},//EU28+2 others Globul

		//Ai Sha ni ya
		{"248","02","Estonia",3},//preferred partners  Elsia
		{"248","01","Estonia",3},//EU28+2 others EMT
		{"248","03","Estonia",3},//EU28+2 others TELE 2

		//Li Tao Wan
		{"246","02","Lithuania",3},//preferred partners  Bit GSM
		{"246","01","Lithuania",3},//EU28+2 others Omnitel
		{"246","03","Lithuania",3},//EU28+2 others TELE 2

		//He Lan
		{"204","04","Netherlands",1},//Vodafone networks Vodafone

		//Ai er lan
		{"272","01","Ireland",2},//Vodafone networks Vodafone
		{"272","02","Ireland",2},//EU28+2 others O2

		//Jie Ke
		{"230","03","Czech Repulic",3},//preferred partners  Bit GSM
		{"230","01","Czech Repulic",3},//EU28+2 others T-Mobile
		{"230","02","Czech Repulic",3},//EU28+2 others O2

		//Si luo fa ke
		{"231","01","Slovakia",2},//EU28+2 others orange
		{"231","02","Slovakia",2},//EU28+2 others T-Mobile

		//Pu Tao Ya
		{"268","01","Portugal",3},//Vodafone networks Vodafone
		{"268","03","Portugal",3},//EU28+2 others Optimus
		{"268","06","Portugal",3},//EU28+2 others TMN

		//Si luo wen ni ya
		{"293","40","Slovenia",3},//preferred partners SI Mobil 
		{"293","41","Slovenia",3},//EU28+2 others Mobitel
		{"293","70","Slovenia",3},//EU28+2 others Tusmobil

		//Ma qi dun
		{"294","03","Macedonia",2},//Row Tier 2 Vip opertor
		{"294","01","Macedonia",2},//Row Tier 5 T-Mobile

		//ke luo di ya
		{"219","10","Croatia",3},//preferred partners VIP net
		{"219","01","Croatia",3},//EU28+2 others Croatian Telecom
		{"219","02","Croatia",3},//EU28+2 others Tele 2

		
		//Bi li shi
		{"206","01","Belgium",3},//preferred partners Proximus
		{"206","10","Belgium",3},//EU28+2 others Mobistar 
		{"206","20","Belgium",3},//EU28+2 others Base

		//Ma Er ta
		{"278","01","Malta",3},//Vodafone networks Vodafone
		{"278","21","Malta",3},//EU28+2 others Go mobile
		{"278","77","Malta",3},//EU28+2 others Melita mobile

		
		//Dan Mai
		{"238","01","Denmark",3},//preferred partners TDC
		{"238","02","Denmark",3},//EU28+2 others  Telenor
		{"238","20","Denmark",3},//EU28+2 others  Telia

		//Lu sen bao 
		{"270","77","Luxembourg",3},//preferred partners Tango
		{"270","01","Luxembourg",3},//EU28+2 others  orange
		{"270","99","Luxembourg",3},//EU28+2 others  LuxGSM

		//BoHei 
		{"218","90","Bosnia",3},//Row Tier 1 Bh Telecom
		{"218","03","Bosnia",3},//Row Tier 5 Eronet
		{"218","05","Bosnia",3},//Row Tier 5 mtel

		//xi ban ya
		{"214","01","Spain",3},//Vodafone networks Vodafone
		{"214","03","Spain",3},//EU28+2 others orange
		{"214","07","Spain",3},//EU28+2 others Movistar

		//xiong ya li
		{"216","70","Hungray",2},//Vodafone networks Vodafone
		{"216","01","Hungray",2},//EU28+2 others Telenor

		//bing dao
		{"274","02","Iceland",3},//preferred partners Tango
		{"274","01","Iceland",3},//Row Tier 3 siminn
		{"274","11","Iceland",3},//Row Tier 3 Nova

		//an dao er 
		{"213","03","Andorra",1},//Row Tier 5 Mobiland

		//Fin lan 
		{"244","05","Finland",4},//preferred partners  Elisa
		{"244","12","Finland",4},//EU28+2 others DNA
		{"244","14","Finland",4},//EU28+2 others Alands
		{"244","91","Finland",4},//EU28+2 others Telia

		//E luo si
		{"250","01","Russian",4},//preferred partners  MTS
		{"250","02","Russian",4},//Row Tier 3 Megafon
		{"250","03","Russian",4},//Row Tier 5 NCC
		{"250","99","Russian",4},//Row Tier 5 Beeline

		//ao di li
		{"232","01","Austria",3},//preferred partners Mobilkom
		{"232","03","Austria",3},//EU28+2 others T-mobile
		{"232","05","Austria",3},//EU28+2 others Orange

		//wu ke lan
		{"255","06","Ukraine",3},//Row Tier 1 Life
		{"255","07","Ukraine",3},//Row Tier 1 UKR telecom
		{"255","01","Ukraine",3},//Row Tier 2 MTS

		//mo er duo wa 
		{"259","01","Moldova",2},//Row Tier 5 Orange
		{"259","02","Moldova",2},//Row Tier 5 Moldcell

		//hei shan
		{"297","03","Montenegro",4},//Row Tier 1 Mtel
		{"297","01","Montenegro",4},//Row Tier 2 Telenor
		{"297","02","Montenegro",4},//Row Tier 5 T-mobile
		{"297","04","Montenegro",4},//Row Tier 5 T-mobile
		
/******************************************Europe END************************************************/

/******************************************America start************************************************/

		//Mei Guo
		{"310","410","America",7},//preferred partners AT&T
		{"310","260","America",7},//preferred partners T-Mobile
		{"310","420","America",7},//Row Tier 1 Cincinnati Bell
		{"310","690","America",7},//Row Tier 1 Immix
		{"310","020","America",7},//Row Tier 3 Union telephone
		{"310","450","America",7},//Row Tier 3 Viaero
		{"310","050","America",7},//Row Tier 3 Alaskan Wireless

		
		{"311","040","America",4},//Row Tier 1 Comment Wireless
		{"311","370","America",4},//Row Tier 3 Alaskan Wireless
		{"311","530","America",4},//Row Tier 5 Newcore
		{"311","330","America",4},//Row Tier 5 Bug Tussel Wireless
		

#endif

/******************************************America END************************************************/		
		

		
		
};

/*void IntToStr(int ilen, char *string)
{
	int power,j;

	j = ilen;
	
	for(power = 1; j >= 10; j /= 10 )
		
	power *= 10;
	
	for(; power > 0; power /= 10 )
	{
		*string++ = '0' + ilen/power;
		ilen % = power;
	}

	*string = '\0';
		
}*/


const unsigned char close_at_command[] = {"AT+CIPCLOSE="};
const unsigned char cpol_at_command[] = {"AT+CPOL="};
unsigned int getcpolAtCommandNumber = 0;
unsigned int getcpolAtnumber = 0;
//unsigned int unirecordruntimes = 0;
bool bcpolatIsSend = false;
bool bcpolatIsSenddefault = false;
bool bgetAtCmdH= false;

bool b_enable_softwarecard_auth = false;
bool b_enable_softwarecard_ok = false;
#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

bool b_enable_softwarecard_send= false;

char  c_softwarecard_auth_buf[80];
unsigned int  c_softwarecard_auth_len = 0;

BYTE roundKeys[11][4][4];

/*--------------------- Rijndael S box table ----------------------*/   
BYTE S[256] = {   
    99,124,119,123,242,107,111,197, 48,  1,103, 43,254,215,171,118,   
        202,130,201,125,250, 89, 71,240,173,212,162,175,156,164,114,192,   
        183,253,147, 38, 54, 63,247,204, 52,165,229,241,113,216, 49, 21,   
        4,199, 35,195, 24,150,  5,154,  7, 18,128,226,235, 39,178,117,   
        9,131, 44, 26, 27,110, 90,160, 82, 59,214,179, 41,227, 47,132,   
        83,209,  0,237, 32,252,177, 91,106,203,190, 57, 74, 76, 88,207,   
        208,239,170,251, 67, 77, 51,133, 69,249,  2,127, 80, 60,159,168,   
        81,163, 64,143,146,157, 56,245,188,182,218, 33, 16,255,243,210,   
        205, 12, 19,236, 95,151, 68, 23,196,167,126, 61,100, 93, 25,115,   
        96,129, 79,220, 34, 42,144,136, 70,238,184, 20,222, 94, 11,219,   
        224, 50, 58, 10, 73,  6, 36, 92,194,211,172, 98,145,149,228,121,   
        231,200, 55,109,141,213, 78,169,108, 86,244,234,101,122,174,  8,   
        186,120, 37, 46, 28,166,180,198,232,221,116, 31, 75,189,139,138,   
        112, 62,181,102, 72,  3,246, 14, 97, 53, 87,185,134,193, 29,158,   
        225,248,152, 17,105,217,142,148,155, 30,135,233,206, 85, 40,223,   
        140,161,137, 13,191,230, 66,104, 65,153, 45, 15,176, 84,187, 22,   
}; 

/*------- This array does the multiplication by x in GF(2^8) ------*/   
BYTE Xtime[256] = {   
    0,  2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,   
        32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,   
        64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94,   
        96, 98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,   
        128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,   
        160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,   
        192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,   
        224,226,228,230,232,234,236,238,240,242,244,246,248,250,252,254,   
        27, 25, 31, 29, 19, 17, 23, 21, 11,  9, 15, 13,  3,  1,  7,  5,   
        59, 57, 63, 61, 51, 49, 55, 53, 43, 41, 47, 45, 35, 33, 39, 37,   
        91, 89, 95, 93, 83, 81, 87, 85, 75, 73, 79, 77, 67, 65, 71, 69,   
        123,121,127,125,115,113,119,117,107,105,111,109, 99, 97,103,101,   
        155,153,159,157,147,145,151,149,139,137,143,141,131,129,135,133,   
        187,185,191,189,179,177,183,181,171,169,175,173,163,161,167,165,   
        219,217,223,221,211,209,215,213,203,201,207,205,195,193,199,197,   
        251,249,255,253,243,241,247,245,235,233,239,237,227,225,231,229   
}; 




/*-------------------------------------------------------------------  
 *  Function to compute OPc from OP and K.  Assumes key schedule has  
    already been performed.  
 *-----------------------------------------------------------------*/   
void ComputeOPc( BYTE op[16], BYTE key[16], BYTE op_c[16] ) {   
  BYTE i;   
  
  RijndaelKeySchedule(key);  
  RijndaelEncrypt( op, op_c );   
  for (i=0; i<16; i++) {  
    op_c[i] ^= op[i];  
  }  
       
  return;   
} // end of function ComputeOPc  
/*-------------------------------------------------------------------  
 *  Rijndael key schedule function.  Takes 16-byte key and creates   
 *  all Rijndael's internal subkeys ready for encryption.  
 *-----------------------------------------------------------------*/   
void RijndaelKeySchedule( BYTE key[16] ) {   
  BYTE roundConst;   
  int i, j;   
   
  // first round key equals key    
  for (i=0; i<16; i++) {  
    roundKeys[0][i & 0x03][i>>2] = key[i];  
  }  
  roundConst = 1;   
  // now calculate round keys */   
  for (i=1; i<11; i++) {   
    roundKeys[i][0][0] = S[roundKeys[i-1][1][3]]   
                         ^ roundKeys[i-1][0][0] ^ roundConst;   
    roundKeys[i][1][0] = S[roundKeys[i-1][2][3]]   
                         ^ roundKeys[i-1][1][0];   
    roundKeys[i][2][0] = S[roundKeys[i-1][3][3]]   
                         ^ roundKeys[i-1][2][0];   
    roundKeys[i][3][0] = S[roundKeys[i-1][0][3]]   
                         ^ roundKeys[i-1][3][0];   
    for (j=0; j<4; j++) {   
      roundKeys[i][j][1] = roundKeys[i-1][j][1] ^ roundKeys[i][j][0];   
      roundKeys[i][j][2] = roundKeys[i-1][j][2] ^ roundKeys[i][j][1];   
      roundKeys[i][j][3] = roundKeys[i-1][j][3] ^ roundKeys[i][j][2];   
    }   
    // update round constant */   
    roundConst = Xtime[roundConst];   
  }   
   
  return;   
} // end of function RijndaelKeySchedule  
   
// Round key addition function  
void KeyAdd(BYTE state[4][4], BYTE roundKeys[11][4][4], int round) {   
  int i, j;   
   
  for (i=0; i<4; i++) {  
      for (j=0; j<4; j++) {  
        state[i][j] ^= roundKeys[round][i][j];  
      }  
  }  
      
  return;   
}   
   
// Byte substitution transformation  
int ByteSub(BYTE state[4][4]) {   
  int i, j;   
   
  for (i=0; i<4; i++) {  
      for (j=0; j<4; j++) {  
        state[i][j] = S[state[i][j]];  
      }  
         
  }  
  
  return 0;   
}   


//Row shift transformation  
void ShiftRow(BYTE state[4][4]) {   
  BYTE temp;   
   
  // left rotate row 1 by 1    
  temp = state[1][0];   
  state[1][0] = state[1][1];   
  state[1][1] = state[1][2];   
  state[1][2] = state[1][3];   
  state[1][3] = temp;   
   
  //left rotate row 2 by 2  
  temp = state[2][0];   
  state[2][0] = state[2][2];   
  state[2][2] = temp;   
  temp = state[2][1];   
  state[2][1] = state[2][3];   
  state[2][3] = temp;   
   
  // left rotate row 3 by 3  
  temp = state[3][0];   
  state[3][0] = state[3][3];   
  state[3][3] = state[3][2];   
  state[3][2] = state[3][1];   
  state[3][1] = temp;   
   
  return;   
}   
   
// MixColumn transformation  
void MixColumn(BYTE state[4][4]) {   
  BYTE temp, tmp, tmp0;   
  int i;   
   
  // do one column at a time  
  for (i=0; i<4;i++) {   
    temp = state[0][i] ^ state[1][i] ^ state[2][i] ^ state[3][i];   
    tmp0 = state[0][i];   
    // Xtime array does multiply by x in GF2^8  
    tmp = Xtime[state[0][i] ^ state[1][i]];   
    state[0][i] ^= temp ^ tmp;   
   
    tmp = Xtime[state[1][i] ^ state[2][i]];   
    state[1][i] ^= temp ^ tmp;   
   
    tmp = Xtime[state[2][i] ^ state[3][i]];   
    state[2][i] ^= temp ^ tmp;   
   
    tmp = Xtime[state[3][i] ^ tmp0];   
    state[3][i] ^= temp ^ tmp;   
  }   
   
  return;   
}   
   
/*-------------------------------------------------------------------  
 *  Rijndael encryption function.  Takes 16-byte input and creates   
 *  16-byte output (using round keys already derived from 16-byte   
 *  key).  
 *-----------------------------------------------------------------*/   
void RijndaelEncrypt( BYTE input[16], BYTE output[16] ) {   
  BYTE state[4][4];   
  int i, r;   

  // initialise state array from input byte string  
  for (i=0; i<16; i++) {  
    state[i & 0x3][i>>2] = input[i];  
  }  
  // add first round_key  
  KeyAdd(state, roundKeys, 0);   
  // do lots of full rounds  
  for (r=1; r<=9; r++) {   
    ByteSub(state);   
    ShiftRow(state);   
    MixColumn(state);   
    KeyAdd(state, roundKeys, r);   
  }   
  // final round  
  ByteSub(state);   
  ShiftRow(state);   
  KeyAdd(state, roundKeys, r);   
  // produce output byte string from state array   
  for (i=0; i<16; i++) {   
    output[i] = state[i & 0x3][i>>2];   
  }   
   
  return;   
} // end of function RijndaelEncrypt  

/*-------------------------------------------------------------------  
 *                            Algorithm f1  
 *-------------------------------------------------------------------  
 *  
 *  Computes network authentication code MAC-A from key K, random  
 *  challenge RAND, sequence number SQN and authentication management  
 *  field AMF.  
 *  
 *-----------------------------------------------------------------*/   
   
void f1( BYTE op_c[16], BYTE key[16], BYTE rand[16], BYTE sqn[6], BYTE amf[2], BYTE mac_a[8] ) {   
  //BYTE op_c[16];   
  BYTE temp[16];   
  BYTE in1[16];   
  BYTE out1[16];   
  BYTE rijndaelInput[16];   
  BYTE i;   
  
  RijndaelKeySchedule( key );   
  for (i=0; i<16; i++) {  
    rijndaelInput[i] = rand[i] ^ op_c[i];  
  }  
  RijndaelEncrypt( rijndaelInput, temp );   
  for (i=0; i<6; i++) {   
    in1[i]    = sqn[i];   
    in1[i+8]  = sqn[i];   
  }   
  for (i=0; i<2; i++) {   
    in1[i+6]  = amf[i];   
    in1[i+14] = amf[i];   
  }   
  // XOR op_c and in1, rotate by r1=64, and XOR  
  // on the constant c1 (which is all zeroes)  
  for (i=0; i<16; i++) {  
    rijndaelInput[(i+8) % 16] = in1[i] ^ op_c[i];   
  }  
  // XOR on the value temp computed before  
  for (i=0; i<16; i++) {  
    rijndaelInput[i] ^= temp[i];   
  }  
  RijndaelEncrypt( rijndaelInput, out1 );   
  for (i=0; i<16; i++) {  
    out1[i] ^= op_c[i];  
  }  
  for (i=0; i<8; i++) {  
    mac_a[i] = out1[i];   
  }  
  
  return;   
} // end of function f1  

 /*-------------------------------------------------------------------  
 *                            Algorithms f2-f5  
 *-------------------------------------------------------------------  
 *  
 *  Takes op_c key K and random challenge RAND, and returns response RES,  
 *  confidentiality key CK, integrity key IK and anonymity key AK.  
 *  
 *-----------------------------------------------------------------*/   
void f2345 ( BYTE op_c[16], BYTE key[16], BYTE rand[16], BYTE res[8], BYTE ck[16], BYTE ik[16], BYTE ak[6] ) {    
  BYTE temp[16];   
  BYTE out[16];   
  BYTE rijndaelInput[16];   
  BYTE i;   
   
  RijndaelKeySchedule( key );   
  for (i=0; i<16; i++) {  
    rijndaelInput[i] = rand[i] ^ op_c[i];   
  }   
  RijndaelEncrypt( rijndaelInput, temp );   
  // To obtain output block OUT2: XOR OPc and TEMP,    
  // rotate by r2=0, and XOR on the constant c2 (which   
  // is all zeroes except that the last bit is 1).   
  for (i=0; i<16; i++) {  
    rijndaelInput[i] = temp[i] ^ op_c[i];  
  }  
  rijndaelInput[15] ^= 1;   
  RijndaelEncrypt( rijndaelInput, out );   
  for (i=0; i<16; i++) {  
    out[i] ^= op_c[i];  
  }  
  for (i=0; i<8; i++) {  
    res[i] = out[i+8];  
  }  
  for (i=0; i<6; i++) {  
    ak[i]  = out[i];  
  }  
  // To obtain output block OUT3: XOR OPc and TEMP,     
  // rotate by r3=32, and XOR on the constant c3 (which    
  // is all zeroes except that the next to last bit is 1).  
  for (i=0; i<16; i++) {  
    rijndaelInput[(i+12) % 16] = temp[i] ^ op_c[i];   
  }   
  rijndaelInput[15] ^= 2;   
  RijndaelEncrypt( rijndaelInput, out );   
  for (i=0; i<16; i++) {  
    out[i] ^= op_c[i];  
  }  
  for (i=0; i<16; i++) {  
    ck[i] = out[i];  
  }  
  // To obtain output block OUT4: XOR OPc and TEMP,   
  // rotate by r4=64, and XOR on the constant c4 (which     
  // is all zeroes except that the 2nd from last bit is 1).  
  for (i=0; i<16; i++) {  
    rijndaelInput[(i+8) % 16] = temp[i] ^ op_c[i];  
  }  
  rijndaelInput[15] ^= 4;   
  RijndaelEncrypt( rijndaelInput, out );   
  for (i=0; i<16; i++) {  
    out[i] ^= op_c[i];  
  }  
  for (i=0; i<16; i++) {  
    ik[i] = out[i];  
  }  
  
  return;   
} // end of function f2345  

/*-------------------------------------------------------------------  
 *                            Algorithm f1*  
 *-------------------------------------------------------------------  
 *  
 *  Computes resynch authentication code MAC-S from key K, random  
 *  challenge RAND, sequence number SQN and authentication management  
 *  field AMF.  
 *  
 *-----------------------------------------------------------------*/   
void f1star(BYTE op_c[16], BYTE key[16], BYTE rand[16], BYTE sqn[6], BYTE amf[2], BYTE mac_s[8] ) {   
  BYTE temp[16];   
  BYTE in1[16];   
  BYTE out1[16];   
  BYTE rijndaelInput[16];   
  BYTE i;   
   
  RijndaelKeySchedule( key );  
  //ComputeOPc( op_c );   
  for (i=0; i<16; i++) {  
    rijndaelInput[i] = rand[i] ^ op_c[i];  
  }   
  RijndaelEncrypt( rijndaelInput, temp );   
  for (i=0; i<6; i++) {   
    in1[i]    = sqn[i];   
    in1[i+8]  = sqn[i];   
  }   
  for (i=0; i<2; i++) {   
    in1[i+6]  = amf[i];   
    in1[i+14] = amf[i];   
  }   
  // XOR op_c and in1, rotate by r1=64, and XOR  
  // on the constant c1 (which is all zeroes)  
  for (i=0; i<16; i++) {  
    rijndaelInput[(i+8) % 16] = in1[i] ^ op_c[i];  
  }   
  // XOR on the value temp computed before  
  for (i=0; i<16; i++) {  
    rijndaelInput[i] ^= temp[i];  
  }  
  RijndaelEncrypt( rijndaelInput, out1 );   
  for (i=0; i<16; i++) {  
    out1[i] ^= op_c[i];  
  }  
  for (i=0; i<8; i++) {  
    mac_s[i] = out1[i+8];  
  }   
   
  return;   
} // end of function f1star  
     
/*-------------------------------------------------------------------  
 *                            Algorithm f5*  
 *-------------------------------------------------------------------  
 *  
 *  Takes key K and random challenge RAND, and returns resynch  
 *  anonymity key AK.  
 *  
 *-----------------------------------------------------------------*/   
void f5star( BYTE op_c[16], BYTE key[16], BYTE rand[16], BYTE ak[6] ) {   
  BYTE temp[16];   
  BYTE out[16];   
  BYTE rijndaelInput[16];   
  BYTE i;   
   
  RijndaelKeySchedule( key );    
  for (i=0; i<16; i++) {  
    rijndaelInput[i] = rand[i] ^ op_c[i];  
  }   
  RijndaelEncrypt( rijndaelInput, temp );   
  // To obtain output block OUT5: XOR OPc and TEMP,  
  // rotate by r5=96, and XOR on the constant c5 (which    
  // is all zeroes except that the 3rd from last bit is 1).  
  for (i=0; i<16; i++) {  
    rijndaelInput[(i+4) % 16] = temp[i] ^ op_c[i];  
  }  
  rijndaelInput[15] ^= 8;   
  RijndaelEncrypt( rijndaelInput, out );   
  for (i=0; i<16; i++) {  
    out[i] ^= op_c[i];   
  }  
  for (i=0; i<6; i++) {  
    ak[i] = out[i];  
  }  
    
  return;   
} // end of function f5star  

/*void vsm_print_hex_100(char *pbuf, int data_len)
{
	int itemp_len = 1;
	char *ptemp_buf;

	ptemp_buf = (char *)malloc(data_len);
	if (ptemp_buf == NULL)
		return;
	memcpy(ptemp_buf, pbuf,data_len);

	LOG5("********** Print data start **********\n");
				
	for(; itemp_len <= data_len;itemp_len++){
		if(itemp_len % 20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(ptemp_buf, itemp_len-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(ptemp_buf, itemp_len-1) );
					
		if(itemp_len % 20 == 0) //Default val of i is 1
					LOG3("\n");
	}

	if((itemp_len-1)%20 != 0) // Avoid print redundant line break

		LOG3("\n");	
					
	free(ptemp_buf);
	
	LOG4("********** Print data end **********\n");
				
}*/

#if 1
void vsm_print_hex
(
  unsigned char *msg,
  unsigned int     msg_len
)
{
  #define VSM_PRINT_MAX_BYTES_PER_LINE 16
  const char hex_chart[] = {'0', '1', '2', '3',
                            '4', '5', '6', '7',
                            '8', '9', 'A', 'B',
                            'C', 'D', 'E', 'F'}; // Do not change this array
  unsigned char buf[VSM_PRINT_MAX_BYTES_PER_LINE * 3 + 1];
  unsigned int buf_idx = 0;                 // 0 to QCRIL_PRINT_MAX_BYTES_PER_LINE * 3
  unsigned int msg_idx = 0;                 // 0 to msg_len - 1
  unsigned int bytes_per_line_idx = 0;      // 0 to QCRIL_PRINT_MAX_BYTES_PER_LINE - 1
  do
  {
    if (NULL == msg || msg_len <= 0)
    {
      break;
    }
    while (msg_idx < msg_len)
    {
      for (bytes_per_line_idx = 0, buf_idx = 0;
          (bytes_per_line_idx < VSM_PRINT_MAX_BYTES_PER_LINE) && (msg_idx < msg_len);
           bytes_per_line_idx++, msg_idx++)
      {
        buf[buf_idx] = hex_chart[(msg[msg_idx] >> 4) & 0x0F];
        buf_idx++;
        buf[buf_idx] = hex_chart[msg[msg_idx] & 0x0F];
        buf_idx++;
        buf[buf_idx] = ' ';
        buf_idx++;
      }
      buf[buf_idx] = '\0';
      LOG2("%s \n", buf);
    }
  } while (false);
} /* qcril_qmi_print_hex */


#endif

test_uim_authentication_data_type_v01 auth_reslt;
BYTE auth_key[16];// K which from softsim value
BYTE auth_opc[16];//OPc from softsim value
SEQTYPE usim_seq_ms[32]; //3//gpp 33.102 c.3.1

void Vsm_init_data()
{
	int i,j;

	LOG("Debug: clean usim_seq_ms .\n");

	for (i = 0; i < 32; i++){

		for (j = 0; j < 6; j++)
			usim_seq_ms[i].seq[j] = 0;			

	}

}
//BYTE usim_sqn_L[];
void vsm_qmi_softsim_authentication(BYTE rand[16],BYTE autn[16], char ki_buf[16], char opc_buf[16])
{
	BYTE op_c[16]; // to get from USIM
	//BYTE op_c[16] ={0xFC,0xA0,0xED,0xBB,0x1F,0xBE,0x82,0x5B,0xC3,0xC6,0xE9,0x60,0xA8,0x3D,0xBF,0xEB};
	BYTE k[16];  // key value from USIM
	//BYTE k[16] = {0x13,0xDB,0x1E,0x51,0x40,0x74,0x18,0x3D,0x2F,0xFA,0x1E,0x7B,0x0A,0x35,0x59,0xE9};
	BYTE res[8];
	BYTE ck[16];
	BYTE ik[16];
	BYTE ak[6];	// generated by f5
	BYTE ak_star[6];//generated by f5star
	BYTE sqn_ak[6];	//sqn from autn
	BYTE sqn[6];// caculated with sqn_ak and ak
	BYTE sqn_ms[6];
	BYTE seq[6];
	BYTE seq_max;
	BYTE amf[2];   //from autn
	BYTE mac_a[8]; //caculated for mac check
	BYTE mac_s[8]; //caculated for synch failure
	BYTE mac[8];   //from autn
	BYTE kc[8];    //gsm key
	BYTE auts[14];
	BYTE output[55];
	int  seq_ind;
	int output_len;
	int ind;
	int i,j;
	int64_t seq_int64,seq_max_int64,seq_ms_int64;

	char i_temp = 0;
	int mac_len = 0;


	memcpy (k,ki_buf,16);
	memcpy (op_c,opc_buf,16);


	LOG5("**********Ki use for auth**********\n");

	i_temp = 1;
	
       for(; i_temp <= sizeof(k); i_temp++){
		if(i_temp%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(k, i_temp-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(k, i_temp-1) );
		
		if(i_temp%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i_temp-1)%20 != 0) // Avoid print redundant line break
		LOG3("\n");
	LOG4("****************************************\n");

	LOG5("**********OPC use for auth**********\n");

	i_temp = 1;
	
       for(; i_temp <= sizeof(op_c); i_temp++){
		if(i_temp%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(op_c, i_temp-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(op_c, i_temp-1) );
		
		if(i_temp%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i_temp-1)%20 != 0) // Avoid print redundant line break
		LOG3("\n");
	LOG4("****************************************\n");

	LOG5("**********rand use for auth**********\n");

	i_temp = 1;
	
       for(; i_temp <= 16; i_temp++){
		if(i_temp%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(rand, i_temp-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(rand, i_temp-1) );
		
		if(i_temp%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i_temp-1)%20 != 0) // Avoid print redundant line break
		LOG3("\n");
	LOG4("****************************************\n");

	LOG5("**********autn use for auth**********\n");

	i_temp = 1;
	
       for(; i_temp <= 16; i_temp++){
		if(i_temp%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(autn, i_temp-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(autn, i_temp-1) );
		
		if(i_temp%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i_temp-1)%20 != 0) // Avoid print redundant line break
		LOG3("\n");
	LOG4("****************************************\n");

	/*for(i_temp = 0; i_temp < 16; i_temp++)
	{

		LOG(" Debug: ki_buf[i_temp] = 0x%02x \n",ki_buf[i_temp]);

	}


	for(i_temp = 0; i_temp < 16; i_temp++)
	{

		LOG(" Debug: k[i_temp] = 0x%02x \n",k[i_temp]);

	}

	for(i_temp = 0; i_temp < 16; i_temp++)
	{

		LOG(" Debug: opc_buf[i_temp] = 0x%02x \n",opc_buf[i_temp]);

	}


	for(i_temp = 0; i_temp < 16; i_temp++)
	{

		LOG(" Debug: op_c[i_temp] = 0x%02x \n",op_c[i_temp]);

	}*/

//get SQN
	memcpy (sqn_ak,autn,6);
	LOG(" softsim print sqn_ak\n ");
	vsm_print_hex(sqn_ak,6);


//get AMF
	memcpy (amf,(autn+5+1),2);
	LOG(" softsim print amf \n");
	vsm_print_hex(amf,2);

//get MAC
	memcpy (mac,(autn+7+1),8);
	LOG(" softsim print mac \n");
	vsm_print_hex(mac,8);


//run f2/f3/f4/f5 to calculate , res, ck,ik,ak	
	f2345(op_c,k,rand,res,ck,ik,ak );
	LOG(" softsim print res \n");
	vsm_print_hex(res,8);
	
	LOG(" softsim print ck \n");
	vsm_print_hex(ck,16);

	LOG(" softsim print ik \n");
	vsm_print_hex(ik,16);

	LOG(" softsim print ak \n");
	vsm_print_hex(ak,6);

//caculate sqn via sqn_ak xor ak
	for(i=0;i<6;i++)
	{	
	sqn[i] = sqn_ak[i]^ak[i];
	}

//run f1 to calculate mac_a(xmac)
	f1(op_c,k,rand,sqn,amf,mac_a);
	LOG(" softsim print mac_a \n ");
	vsm_print_hex(mac_a,8);

	
//compare MAC with XMAC
	bool mac_eaqual = true;
	for(i=0;i<8;i++)
		{
		if(mac[i]!= mac_a[i])
			mac_eaqual = false;
		}
	if(!mac_eaqual)
		{
		LOG("MAC check failure, return error '98''62' \n ");
		//'98'	'62'	-	Authentication error, incorrect MAC
		output[0]= SW1_SECURITY;
		output[1]= SW2_APPLN_SPECIFIC_AUTH_ERR;
		output_len = 2;
		auth_reslt.context = 0x03;
		//auth_reslt.context = UIM_AUTH_CONTEXT_3G_SEC_V01;
		auth_reslt.data_len = output_len;
		memcpy(auth_reslt.data,output,output_len);
		mac_len = 55;
		proto_encode_software_data(auth_reslt.data, &mac_len);
		return;
				
		}
	
//check if SQN is in the correct range
/*
C.2.2	Verification of sequence number freshness in the USIM
The USIM shall maintain an array of a previously accepted sequence number components: SEQMS (0), SEQMS (1),\A1\AD SEQMS (a-1). The initial sequence number value in each array element shall be zero.
To verify that the received sequence number SQN is fresh, the USIM shall compare the received SQN with the sequence number in the array element indexed using the index value IND contained in SQN, i.e. with the array entry SEQMS (i) where i = IND is the index value.
(a)	If SEQ > SEQMS (i) the USIM shall consider the sequence number to be guaranteed fresh and subsequently shall set SEQMS (i) to SEQ.
(b)	If SEQ \A1\DC SEQMS (i) the USIM shall generate a synchronisation failure message using the highest previously accepted sequence number anywhere in the array, i.e. SQNMS.
The USIM shall also be able to put a limit L on the difference between SEQMS and a received sequence number component SEQ. If such a limit L is applied then, before verifying the above conditions (a) and (b), the sequence number shall only be accepted by the USIM if SEQMS - SEQ < L. If SQN can not be accepted then the USIM shall generate a synchronisation failure message using SQNMS.

*/


	//caculate ind
    ind = sqn[5]& 0x1F;
	//caculate recieved seq
	bool check_sqn = false;
	for(i=0;i<6;i++)
		{
		seq[i] = sqn[i];
		}
	seq[5] &= 0xE0; 


	//seek for the highest seq which in the array
	seq_ind=0;
	for(i=0;i<6;i++)
		{
		seq_max=usim_seq_ms[0].seq[i];
		for(j=1;j<32;j++)
			{
			if(seq_max<usim_seq_ms[j].seq[i])
				{
				seq_max=usim_seq_ms[j].seq[i];
				seq_ind=j;
				}
			}
		if(seq_ind)
			break;
		}
	
	//transfer seqmax to int64
	seq_max_int64=0;
	seq_max_int64 ^= usim_seq_ms[seq_ind].seq[0];
	for(i=1;i<6;i++)
		{
		seq_max_int64 <<=8;
		seq_max_int64 ^= usim_seq_ms[seq_ind].seq[i];
		}	



	//transfer seq to int64
	seq_int64=0;
	seq_int64 ^= seq[0];
	for(i=1;i<6;i++)
		{
		seq_int64 <<=8;
		seq_int64 ^= seq[i];
		}
	


	//transfer usim_seq_ms[ind].seq[i] to int64
	seq_ms_int64=0;
	seq_ms_int64 ^= usim_seq_ms[ind].seq[0];
	for(i=1;i<6;i++)
		{
		seq_ms_int64 <<=8;
		seq_ms_int64 ^= usim_seq_ms[ind].seq[i];
		}	

	/*
	check whether the seq meet the criteria
	SEQ > SEQMS (i)/SEQ-SEQMS <=delta/ SEQMS - SEQ < L.  delata set to 2^28, L set to 2^20. normally L and delta can be  provided by the operator.
	*/

	if((seq_int64>seq_ms_int64) && (seq_int64-seq_max_int64<=delta) && (seq_max_int64-seq_int64<limit))
		check_sqn=true;

	//SQN not in the correct range, report SYNCH FAILURE 
	if(!check_sqn)
	
		{
		/* debug info for SQN checking*/
		LOG(" softsim SQN checking print seqmax \n");
		vsm_print_hex(usim_seq_ms[seq_ind].seq,6);
		
		LOG(" softsim SQN checking print seq \n");
		vsm_print_hex(seq,6);

		LOG(" softsim SQN checking print seqms \n");
		vsm_print_hex(usim_seq_ms[ind].seq,6);		
			
			
		//caculate sqn_ms from seqms and ind, SQNMS=seqms||ind
		for(i=0;i<6;i++)
		{
		sqn_ms[i]=usim_seq_ms[seq_ind].seq[i];
		}
		sqn_ms[5]|= (seq_ind&0x1F);
		//caculate mac_s
		f1star(op_c, k,  rand,  sqn_ms,  amf,  mac_s);
		//caculate ak_star
		f5star(op_c,  k,  rand,  ak_star);
		//populate auts with sqnms
		for(i=0;i<6;i++)
		{
		auts[i]=sqn_ms[i]^ak_star[i];
		}
		//populate auts with mac_s
		for(i=0;i<8;i++)
			{
			auts[i+6]= mac_s[i];
			}
		//populate the AUTS which report to upper layer
		output[0]= 0xDC;
		output[1]= 0x0E;
		for(i=2;i<(14+2);i++)
			output[i]=auts[i-2];

		//populate sw1/sw2
		output[16]= SW1_NORMAL_END;
		output[17]= SW2_NORMAL_END;		
		output_len=18;
		}
	else
		{
		//update SEQMS with current recieved SEQ
		for(i=0;i<6;i++)
		usim_seq_ms[ind].seq[i]= seq[i];


	//authentication succesful "DB"
	output[0]=0xDB;
	//RES
	output[1]=0x08;
	for(i=2;i<(8+2);i++)
		output[i]=res[i-2];
	//CK
	output[8+2]=0x10;
	for(i=(8+3);i<(8+16+3);i++)
		output[i]=ck[i-8-3];
	//IK
	output[8+16+3]=0x10;
	for(i=(8+16+4);i<(8+16+16+4);i++)
		output[i]=ik[i-8-16-4];

//create GSM Kc  3gpp 33.102 c3: Kc[GSM] = CK1 xor CK2 xor IK1 xor IK2
	memcpy(kc,ck,8);
	for(i=0;i<8;i++)
		{
		kc[i]^=ck[i+8];
		}
	for(i=0;i<8;i++)
		{
		kc[i]^=ik[i];
		}
	for(i=0;i<8;i++)
		{
		kc[i]^=ik[i+8];
		}

	output[8+16+16+4]=0x08;
	for(i=(8+16+16+5);i<(8+16+16+13);i++)
		output[i]=kc[i-8-16-16-5];
		
	// add SW1 and SW2	
	output[8+16+16+13]= SW1_NORMAL_END;
	output[8+16+16+14]= SW2_NORMAL_END;
	output_len = 55;
		
		}

	//auth_reslt.context = UIM_AUTH_CONTEXT_3G_SEC_V01;
	auth_reslt.context = 0x03;
	auth_reslt.data_len = output_len;
	memcpy(auth_reslt.data,output,output_len);
	LOG(" softsim print auth_result \n ");
	vsm_print_hex(auth_reslt.data,output_len);	

	LOG(" softsim print auth_result,output_len = %d \n", output_len); 
	proto_encode_software_data(auth_reslt.data, &output_len);


	
	//proto_encode_raw_data(auth_reslt.data, &output_len,REQ_TYPE_APDU, NULL, NULL);        
	//t_buf->data = proto_encode_raw_data(buf, &length, REQ_TYPE_APDU, NULL, NULL); 


	

	return;
}

#endif


/**********************************************************************
* SYMBOL	: Int_To_Ascii
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/05
**********************************************************************/
 int Int_To_Ascii(char *pbuf,unsigned int uidata)
{
	
	char *pdata = NULL;
	unsigned char uctemp = 0;

	pdata = pbuf;

	while(uidata > 0)
	{
		*(pdata+uctemp) = (uidata%10)+ '0';
		uctemp++;
		uidata /= 10;
	}
	
	return uctemp;
	
}
/**********************************************************************
* SYMBOL	: Atcommand_CoplByMCCMNC
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/05
**********************************************************************/
int Atcommand_CoplByMCCMNC(unsigned int uncoutype,char *pbuf,unsigned int *at_commandLen)
{
	unsigned char j,atbit,len;
	unsigned char at_commandbuf[100];
	int itemp;
	char *pat_buf = NULL;
	unsigned char i = 0;
	unsigned char uctempbuf[6]={0};

	pat_buf = pbuf;
	
	atbit = 0;

	
	
	for(i=0;i< sizeof(cops_Emodem_list)/sizeof(ST_EmodemDEFAULT_CONFIG);i++)
	{
		itemp = atoi(cops_Emodem_list[i].pcops);
	
#ifdef FEATURE_ENABLE_CPOL_CONFIG_FOR_HK
		//if( (uncoutype == itemp) && ((uncoutype == 45403 ) || (uncoutype == 51011)))
		if(uncoutype == itemp)
		{
#endif
			//LOG("Get MCCMNC i = %d\n",i);

			//itemp = sizeof(cpol_at_command)-1;
			//LOG("Get MCC.777777 itemp= %d\n",itemp);
			memcpy((unsigned char *)&at_commandbuf[atbit],(unsigned char *)cpol_at_command,(sizeof(cpol_at_command) -1));

			//itemp = sizeof(cpol_at_command) -1;
			//LOG("Get MCC.66666 temp = %d\n",itemp);

			len = Int_To_Ascii(uctempbuf,cops_Emodem_list[i].cbit);
			LOG("Get  len = %d\n",len);
			LOG("Get uctempbuf[0] = 0x%02x \n",uctempbuf[0]);
			LOG("Get uctempbuf[1] = 0x%02x \n",uctempbuf[1]);
			
			if(len < 2)
			{
				at_commandbuf[atbit+sizeof(cpol_at_command) -1] = uctempbuf[0];
				//itemp = atbit+sizeof(cpol_at_command) -1;
				//LOG("Get MCC.00000 itemp = %d\n",itemp);
				//LOG("aaaaaaaaaaaaaaaa at_commandbuf[itemp] =0x%02x\n",at_commandbuf[itemp]);
				
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+1] = ',';
				//itemp = atbit+sizeof(cpol_at_command) -1+1;
				//LOG("Get MCC.00000 itemp = %d\n",itemp);
				//LOG("aaaaaaaaaaaaaaaa at_commandbuf[itemp] = 0x%02x\n",at_commandbuf[itemp]);
				
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+2] = '2';
				//itemp = atbit+sizeof(cpol_at_command) -1+2;
				////LOG("Get MCC.00000 itemp = %d\n",itemp);
				//LOG("aaaaaaaaaaaaaaaa at_commandbuf[itemp] = 0x%02x\n",at_commandbuf[itemp]);

				
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+3] = ',';
				//itemp = atbit+sizeof(cpol_at_command) -1+3;
				///LOG("Get MCC.00000 itemp = %d\n",itemp);
				//LOG("aaaaaaaaaaaaaaaa at_commandbuf[itemp] = 0x%02x\n",at_commandbuf[itemp]);

				
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+4] = '"';
				//itemp = atbit+sizeof(cpol_at_command) -1+4;
				//LOG("Get MCC.00000 itemp = %d\n",itemp);
				//LOG("aaaaaaaaaaaaaaaa at_commandbuf[itemp] = 0x%02x\n",at_commandbuf[itemp]);

				

			//	itemp = strlen(cops_Emodem_list[i].pcops);
			   //    LOG("Get MCC.88888 temp = %d\n",itemp);
				for(j=0;j<(strlen(cops_Emodem_list[i].pcops));j++)
				{
					at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+j] = *(cops_Emodem_list[i].pcops+j);
					//itemp = atbit+(sizeof(cpol_at_command) -1)+5+j;
					//LOG("Get MCC.00000 itemp = %d\n",itemp);
					//LOG("aaaaaaaaaaaaaaaa at_commandbuf[itemp] = 0x%02x\n",at_commandbuf[itemp]);

					at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+j] += 0x01; // the value can be set anything.

				}

				//LOG("Get MCC.00000 atbit = %d\n",atbit);
				
				at_commandbuf[(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)] = '"';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+1] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+2] = '1';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+3] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+4] = '0';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+5] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+6] = '1';

				itemp = (sizeof(cpol_at_command) -1)+5+strlen(cops_Emodem_list[i].pcops)+7;

				/*LOG("dddddddddddddddddddddddddddddddddddddddd temp = %d\n",itemp);
				for(j=0;j<itemp;j++)
				{
					LOG("dddddddddddddddddddddddddddddddddddddddd j = %d\n",j);
					LOG("dddddddddddddddddddddddddddddddddddddddd at_commandbuf[] = 0x%02x \n",at_commandbuf[j]);
				}*/
				
				memcpy(pat_buf,(unsigned char *)at_commandbuf,itemp);
				*at_commandLen = itemp;
				
				return 1;
			}
			else
			{
				at_commandbuf[atbit+sizeof(cpol_at_command) -1] = uctempbuf[1];
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+1] = uctempbuf[0];
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+2] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+3] = '2';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+4] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5] = '"';
				
				//itemp = strlen(cops_Emodem_list[i].pcops);
			    //   LOG("Get MCC.9999 temp = %d\n",itemp);
				   
				for(j=0;j<(strlen(cops_Emodem_list[i].pcops));j++)
				{
					at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+j] = *(cops_Emodem_list[i].pcops+j);
					at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+j] += 0x01; // the value can be set anything.

				}
				// LOG("Get MCC.00000 atbit = %d\n",atbit);
				at_commandbuf[(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)] = '"';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+1] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+2] = '1';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+3] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+4] = '0';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+5] = ',';
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+6] = '1';

			       itemp = (sizeof(cpol_at_command) -1)+6+strlen(cops_Emodem_list[i].pcops)+7;
			       //LOG("Get MCC.bbbbb temp = %d\n",itemp);
				memcpy(pat_buf,(unsigned char *)at_commandbuf,itemp);
				*at_commandLen = itemp;
				return 1;
			}	
		}
	}

	memset(at_commandbuf,0,100);
	memcpy(pat_buf,(unsigned char *)at_commandbuf,30);
	*at_commandLen = 0;
	
	return -1;
}

/**********************************************************************
* SYMBOL	: ConfCPOLByCountryType
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/05
**********************************************************************/
int ConfCPOLByCountryType(unsigned int uncoutype,char *pbuf,unsigned int *at_commandLen,unsigned int *at_commandCount)
{
	unsigned char j,atbit;
	unsigned char at_commandbuf[100];
	int itemp;
	char *pat_buf = NULL;
	unsigned int i = 0;
	static bool bl_conut = false;
	static unsigned char firstbit = 0x31;
	

	pat_buf = pbuf;
	atbit = 0;
	
	i = *at_commandCount;
	LOG("Debug:   i  = %d\n", i);
	
	for(;i< sizeof(snetconfig_Emodem)/sizeof(ST_NET_TPYE);i++)
	{
		//LOG("Get MCC.i = %d\n",i);
		
		itemp = atoi(snetconfig_Emodem[i].psmcc);
		//LOG("Get MCC.itemp = %d\n",itemp);
		//LOG("Get MCC.uncoutype = %d\n",uncoutype);
		if (uncoutype == itemp)
		{
			*at_commandCount = i;
			LOG("Debug *at_commandCount == %d\n",*at_commandCount);
			LOG("Debug:uncoutype == itemp   i  = %d\n", i);

			
			
			if (false == bl_conut)
			{
				bl_conut = true;	
				getcpolAtCommandNumber = snetconfig_Emodem[i].ucnet_quantity;
				//unirecordruntimes = snetconfig_Emodem[i].ucnet_quantity;
			}
			
			LOG("Debug: *getcpolAtCommandNumber = %d\n",getcpolAtCommandNumber );
			memcpy((unsigned char *)&at_commandbuf[atbit],(unsigned char *)cpol_at_command,(sizeof(cpol_at_command) -1));
			
			at_commandbuf[atbit+sizeof(cpol_at_command) -1] = firstbit;
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+1] = ',';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+2] = '2';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+3] = ',';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+4] = '"';
				   
			for(j=0;j<strlen(snetconfig_Emodem[i].psmcc);j++)
			{
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+j] = *(snetconfig_Emodem[i].psmcc+j);

			}

			for(j=0;j<strlen(snetconfig_Emodem[i].psmnc);j++)
			{
				at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+j] = *(snetconfig_Emodem[i].psmnc+j);

			}
				
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc)] = '"';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc) + 1] = ',';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc) + 2] = '1';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc) + 3] = ',';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc) + 4] = '0';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc) + 5] = ',';
			at_commandbuf[atbit+(sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc) + 6] = '1';
	
			itemp = (sizeof(cpol_at_command) -1)+5+strlen(snetconfig_Emodem[i].psmcc)+strlen(snetconfig_Emodem[i].psmnc)+7;
			//LOG("nnnnnnnnnnnnnnnnnnnn temp = %d\n",itemp);
			//atbit += itemp;
			firstbit++;
			LOG("Debug: atbit = %d\n",atbit);
			LOG("Debug: firstbit = %d\n",firstbit);
			memcpy(pat_buf,(unsigned char *)at_commandbuf,itemp);
			*at_commandLen = itemp;
			return 1;
		}
	}
	
	memset(at_commandbuf,0,100);
	memcpy(pat_buf,(unsigned char *)at_commandbuf,30);
	*at_commandLen = 0;
	return -1;
}

 #endif

/******************************Advanced Declaration*******************************/
static int reader(int fd, char **data_ptr);
static int writer(const int fd, const void *data, const unsigned int len);
static void openDev(char opt);

void w_checkNetHdlr(void);
void r_checkNetHdlr(void *userdata);
void w_uartConfigHdlr(void);
void r_uartConfigHdlr(void *userdata);
void w_acqDataHdlr(void);
void r_acqDataHdlr(void *userdata);
void w_suspendMdmHdlr(void);
void r_suspendMdmHdlr(void *userdata);
void w_resumeMdmHdlr(void);
void r_resumeMdmHdlr(void *userdata);

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK
void w_closeUdpHdlr(void);
void r_closeUdpHdlr(void *userdata);

#endif

int mdmEmodem_check_if_running(void);
void add_timer_ate0_rsp_timeout(void);
void add_timer_pdp_act_check_routine(void);
void add_timer_Custom_Timer_timeout(int sec, int usec, char cb_type, char opt);
static int dequeue_and_send_req(rsmp_transmit_buffer_s_type *rc);

#ifdef FEATURE_ENABLE_RECV_BUF_LOG
	#ifndef FEATURE_DEBUG_QXDM
		#ifndef FEATURE_DEBUG_LOG_FILE
			#define RECV_LOG_PRINT_ATBUFCUR(round, len) \
			{ \
				int i; \
				LOG("Round%d ->ATBufCur\n", round); \
				for(i=0; i<len; i++) \
					if(ATBufCur[i] == 0x0A) LOG2("\\n"); \
					else if(ATBufCur[i] == 0x0D 0A) LOG2("\\r"); \
					else if(ATBufCur[i] == '\0') LOG2("\\0"); \
					else LOG2("%c", ATBufCur[i]); \
				LOG2("\n\n"); \
			}

			#define RECV_LOG_PRINT_LINEHEAD(round, len) \
			{ \
				int i; \
				LOG("Round%d ->line_head\n", round); \
				for(i=0; i<len; i++) \
					if(line_head[i] == '\n'){ LOG2("\\n"); \
					}else if(line_head[i] == '\r'){ LOG2("\\r"); \
					}else if(line_head[i] == '\0'){ LOG2("\\0"); \
					}else{ LOG2("%c", line_head[i]); \
				} \
				LOG2("\n\n"); \
			}
		#else
			#error code not present
		#endif /*FEATURE_DEBUG_LOG_FILE*/
	#else
		#error code not present
	#endif /*FEATURE_DEBUG_QXDM*/
#else
void recv_log_null(int r, int l){}
#define RECV_LOG_PRINT_ATBUFCUR(round, len)  recv_log_null(round, len);
#define RECV_LOG_PRINT_LINEHEAD(round, len)  recv_log_null(round, len);
#endif //FEATURE_ENABLE_RECV_BUF_LOG

#define INIT_ATBUF \
{ \
  memset(ATBuf, 0, AT_BUF_LEN); \
  ATBufCur = ATBuf; \
  line_head = ATBuf; \
}

//Fixed by rjf at 20150810 (Change "return" to "break". This macro is only called by r_acqDataHdlr().)
#define GO_TO_NEXT_STATE_R(state, hdlr, oper) \
{ \
  acq_data_state = (state); \
  INIT_ATBUF; \
  set_evt(&at_config.AT_event, at_config.serial_fd, false, hdlr, NULL); \
  add_evt (&at_config.AT_event, oper); \
  break; \
}

//Fixed by rjf at 20150810 (Change "return" to "break". This macro is only called by r_acqDataHdlr().)
//Applicable Situation:
//In r_acqDataHdlr() or w_acqDataHdlr().
#define GO_TO_NEXT_STATE_W(state, hdlr) \
{ \
  acq_data_state = (state); \
  hdlr(); \
  break; \
}

//Ret:
// 1: mdmEmodem is busy.
// 0: mdmEmodem is idle.
// -1: mdmEmodem is at standby mode.
int mdmEmodem_check_busy(void)
{
  if(!check_acq_data_state(ACQ_DATA_STATE_WAITING) 
      || chk_net_state != CHK_NET_STATE_MAX 
      || uartconfig_state != CONF_STATE_MAX
  ){
    LOG("mdmEmodem is busy. acq_data_state=%d, chk_net_state=%d, uartconfig_state=%d.\n", 
                          get_acq_data_state(), chk_net_state, uartconfig_state);
    return 1;
  }

  return 0;
}

//Note:
//No need to ref add_node in event_qlinkdatanode.c
void add_req_into_queue(rsmp_transmit_buffer_s_type *rc, rsmp_state_s_type state){
  add_node(rc, state);
}

//Function:
//Call read() to clear read buf and wait for some time to make sure no more data in kernel buffer.
//It is for handling the situation in which wrong data of APDU or remote USIM data is recved.
static void clr_read_buf_v02(int timeout_sec, int timeout_usec)
{
  char tmp_buf[512] = {0};
  fd_set rfd;
  struct timeval tv;
  
  int fd = at_config.serial_fd, ret = -1, read_bytes = -1;

  FD_ZERO(&rfd);
  FD_SET(fd, &rfd);

__CLR_READ_BUF_V02_SELECT_AGAIN__:
  tv.tv_sec = timeout_sec;
  tv.tv_usec = timeout_usec;
  ret = select(fd+1, &rfd, NULL, NULL, &tv);
  if(ret < 0){
    //It should not reach here.
    LOG("ERROR: select() failed. errno=%d.\n", errno);
    return;
  }else if(ret > 0){
__CLR_READ_BUF_V02_READ_AGAIN__:
    read_bytes = read(fd, tmp_buf, sizeof(tmp_buf));
    if(read_bytes < 0)
    {
      if(errno == EINTR || errno == EAGAIN)
        goto __CLR_READ_BUF_V02_READ_AGAIN__;

      LOG("ERROR: read() failed. errno=%d.\n", errno);
    }else if(read_bytes > 0){
      LOG("Clr read buf in progress...\n");
      goto __CLR_READ_BUF_V02_SELECT_AGAIN__;
    }
  }else
  {//ret ==0 select timeout
    
  }

  return;
}

//Return Values:
// 1: Set AT_event to waiting state.
// 0: Not set AT_event to waiting state
//Note:
// Not acquire acq_data_state_mutex inside (but outside).
//Modication Records:
// 2015-9-11 : 1. Add the conditional judgement of isQueued.
//             2. Change return type from void to int.
int set_at_evt_waiting(void)
{
  int ret = -1;

  LOG("isQueued = %d, isFTMPowerOffInd = %d.\n", isQueued, isFTMPowerOffInd);

  if(true == isQueued && false == isFTMPowerOffInd)
  { // Add by rjf at 20151008
    if(1 == queue_check_if_node_valid()){
      if(1 == dequeue_and_send_req(&g_rsmp_transmit_buffer[THD_IDX_EVTLOOP])){
        ret = 0;
      }else{ // dequeue_and_send_req() returns 0 or -1
        goto __SET_AT_EVT_WAITING__;
      }
    }else{
      // Note: When queue_check_if_node_valid() is just for checking invalid APDU, this operation is valid.
      queue_clr_apdu_node();
      goto __SET_AT_EVT_WAITING__;
    }
  }else{
__SET_AT_EVT_WAITING__:
    acq_data_state = ACQ_DATA_STATE_WAITING;
    INIT_ATBUF;
    set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
    add_evt (&at_config.AT_event, 'r');

    ret = 1;
  }

  return ret;
}

//Applicable Situation:
//In QMI sender. (So acquire acq_data_state_mtx inside.)
void go_to_next_state_after_recving_IMSI(void)
{
	ACQ_ACQ_DATA_STATE_MTX;
	acq_data_state = ACQ_DATA_STATE_NETOPEN;
	w_acqDataHdlr();
	RLS_ACQ_DATA_STATE_MTX;
	trigger_EventLoop();
	return;
}

//Applicable Situation:
// In QMI parser. 
//Notice:
// Please acquire acq_data_state_mtx inside. And ChannelLock has been acqed outside.
void go_to_next_state_after_removing_USIM_files(void)
{
  int length;

  LOG("g_disable_send_downloadSimcard_req = %d\n", g_disable_send_downloadSimcard_req);

  if (g_disable_send_downloadSimcard_req == true)
  {
  	LOG("return here \n");
	return;
  }

  if(!isRstReqAcq)
  {
    queue_clr_apdu_node(); // Fixed by rjf at 20150803

    rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

		reset_rsmp_transmit_buffer(p_req_cfg);
		p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_REMOTE_UIM_DATA, NULL, NULL);
		p_req_cfg->size = length;
		
		if(ChannelNum != 0 && ChannelNum != 3){
			LOG("ChannelNum = %d.\n", ChannelNum);
			add_node(p_req_cfg, FLOW_STATE_REMOTE_UIM_DATA_REQ);
			//Wait for mdmEmodem network restoration in state of ACQ_DATA_STATE_WAITING
		}else{
			LOG("flow_state changed to %d from %d.\n", FLOW_STATE_REMOTE_UIM_DATA_REQ, flow_info.flow_state);
			proto_update_flow_state(FLOW_STATE_REMOTE_UIM_DATA_REQ);
			
			ACQ_ACQ_DATA_STATE_MTX;
			acq_data_state = ACQ_DATA_STATE_CIPSEND_1;
			w_acqDataHdlr();
			RLS_ACQ_DATA_STATE_MTX;
			trigger_EventLoop();
		}
	}else{
		LOG("isRstReqAcq = 1!\n");
		//Not reset var for "Factory Reset"
	}
	
  return;
}

//Note:
// No need replace req_config with a temp struct Req_Config although it is called by QMI sender.
// Because at_event is temporarily shut down for acquring IMSI.
static void at_send_id_auth_msg(char opt)
{
  int length;
  rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

  reset_rsmp_transmit_buffer(p_req_cfg);
  p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_ID, &sim5360_dev_info, NULL);
  p_req_cfg->size = length;
  proto_update_flow_state(FLOW_STATE_ID_AUTH);

  if(0x01 == opt){
    ACQ_ACQ_DATA_STATE_MTX;
    acq_data_state = ACQ_DATA_STATE_CIPSEND_1;
    w_acqDataHdlr();
    RLS_ACQ_DATA_STATE_MTX;
    trigger_EventLoop();
  }else{
    acq_data_state = ACQ_DATA_STATE_CIPSEND_1;
    w_acqDataHdlr();
  }
  return;
}

void send_id_auth_msg_after_recving_IMSI(void)
{
  LOG6("~~~~ send_id_auth_msg_after_recving_IMSI ~~~~\n");
  at_send_id_auth_msg(0x01);
  return;
}

//Note:
//This must be called by msq thread.
void at_assembly_pd_msg(void)
{
  int length;
  rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

  proto_update_flow_state(FLOW_STATE_PWR_DOWN_UPLOAD);
  reset_rsmp_transmit_buffer(p_req_cfg);
  p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_PWR_DOWN, NULL, NULL);
  p_req_cfg->size = length;
}

//Description:
//Send power-down msg to server.
void at_send_pd_msg_to_srv(void)
{
  acq_data_state = ACQ_DATA_STATE_CIPSEND_1;
  w_acqDataHdlr();
  trigger_EventLoop();
}

static void mdmEmodem_uart_keep_awake(void)
{
  gpio_mdmEmodem_uart_keep_awake();
}

//Return Values:
//	1: mdmEmodem is poweron.
//	0: mdmEmodem is poweroff.
//  -1: select() error.
static int mdmEmodem_check_startup(void)
{
	int cmd_fd = at_config.serial_fd, written_bytes = -1, fdp1 = -1;
	int ret = -1, result = -1;
	char cmd[] = "AT\r\n";
	struct timeval tv;
	fd_set rfd;
	
	mdmEmodem_uart_keep_awake();
	
	do{
		written_bytes = write(cmd_fd, cmd, strlen(cmd));
		if(written_bytes == 0 || (written_bytes == -1 && errno == EINTR)){
			continue;
		}else if(written_bytes < 0){
			LOG("ERROR: write() failed. errno=%s(%d).\n",
					strerror(errno), errno);
		}
	}while(written_bytes != strlen(cmd));
	
	fdp1 = cmd_fd+1;
			
	FD_ZERO(&rfd);
	FD_SET(cmd_fd, &rfd);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	ret = select(fdp1, &rfd, NULL, NULL, &tv);
	if(ret == 0){
		result = 0;
	}else if(ret == 1){
		result = 1;
	}else{
		LOG("ERROR: select() error. errno=%s(%d).\n",
				strerror(errno), errno);
		return -1;
	}
	
	return result;
}

#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_STARTUP
void mdmEmodem_power_switch_init(void)
{
  gpio_mdmEmodem_pwr_sw_init();
}

//Description:
//	If app fails to start mdmEmodem at first time, it need to call this at second try.
void mdmEmodem_restart_power_ctrl_oper(void)
{
  gpio_mdmEmodem_restart_pwr_ctrl_oper();
}

//Param:
//	opt = 0: The process of shutting down mdmEmodem.
//	opt = 1: The process of starting up mdmEmodem.
//Ret:
//	  1: Power switch succeed.
//	  0: Power switch failed. For example, app is trying to start mdmEmodem. When app retrys to send AT cmd and recv nothing for 10 times, it
//		  will be considered as a failure.
//	-1: Wrong param input.
//Note:
//	When app call this function, the channel of sending at cmd and recving rsp is alreay set up.
int mdmEmodem_power_switch(char opt)
{
	char cmd[] = "AT\r\n";
	char rsp_buf[128] = {0};
	int written_bytes = -1, read_bytes = -1, cmd_fd = at_config.serial_fd, retry_times = 0;
	fd_set rfd;
	
	LOG6("~~~~ mdmEmodem_power_switch (opt: 0x%02x) ~~~~\n", opt);
	
	if(opt != 0x00 && opt != 0x01){
		LOG("Wrong param input. opt=0x%02x.\n", opt);
		return -1;
	}
	
	if(opt == 0x00){
		gpio_mdmEmodem_shutdown();
	}else{
		gpio_mdmEmodem_startup();
	}
	
	do{
		if(retry_times == 10){
			return 0;
		}
		
__MDMEmodem_POWER_SWITCH_RESEND_CMD__:
		written_bytes = write(cmd_fd, cmd, sizeof(cmd)-1);
		if(written_bytes != sizeof(cmd)-1){
			if(written_bytes == 0 || (written_bytes < 0 && errno == EINTR)){
				goto __MDMEmodem_POWER_SWITCH_RESEND_CMD__;
			}else{
				LOG("write() failed. errno=%s(%d).\n", strerror(errno), errno);
				continue;
			}
		}else{ // written_bytes == sizeof(cmd) - 1
			int ret = -1, fdp1 = cmd_fd+1;
			struct timeval tv;
			
			FD_ZERO(&rfd);
			FD_SET(cmd_fd, &rfd);
			memset(&tv, 0, sizeof(tv));
			
			__MDMEmodem_POWER_SWITCH_SELECT_AGAIN__:
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			ret = select(fdp1, &rfd, NULL, NULL, &tv);
			if(ret < 0){
				LOG("ERROR: select() error. errno=%d.\n", errno);
				goto __MDMEmodem_POWER_SWITCH_SELECT_AGAIN__;
			}else if(ret == 0){
				if(0x01 == opt){
					LOG("mdmEmodem is not started up yet! Continue...\n");
					retry_times ++;
					continue;
				}else{ // 0x00 == opt
					LOG("NOTICE: mdmEmodem has been shut down.\n");
					gpio_mdmEmodem_shutdown_ext(); // Add by rjf at 20150818
					break;
				}
			}else{ // ret == 1
				__MDMEmodem_POWER_SWITCH_READ_AGAIN__:
				read_bytes = read(cmd_fd, rsp_buf, sizeof(rsp_buf)-1);
				if(read_bytes == 0 
					|| (read_bytes < 0 && errno == EINTR)
					|| (read_bytes < 0 && errno == EAGAIN)
				){
					goto __MDMEmodem_POWER_SWITCH_READ_AGAIN__;
				}else if(read_bytes < 0){
					LOG("ERROR: read() failed. errno=%d.\n", errno);
					goto __MDMEmodem_POWER_SWITCH_READ_AGAIN__;
				}
				
				if(0x00 == opt){
					LOG("mdmEmodem is not shut down yet! Continue...\n");
					retry_times ++;
					continue;
				}else{ // 0x01 == opt
					LOG("NOTICE: mdmEmodem has been started up.\n");
					break;
				}
			} // select() ret == 1
				
		} // write() succeed
	}while(1);
	
	if(opt == 0x00){
		isMdmEmodemPwrDown = true;
		isMdmEmodemRunning = false; // Add by rjf at 20150804
	}else{
		isMdmEmodemPwrDown = false;
		isMdmEmodemRunning = true; // Add by rjf at 20150804
	}
	return 1;
}

//Param:
//	opt:
//		0: Start mdmEmodem at first time.
//		1: Start mdmEmodem again for a startup failure.
static void mdmEmodem_init(char opt)
{
	int ret = -99;
	
	mdmEmodem_power_switch_init();
	
	if(1 == opt){
		mdmEmodem_restart_power_ctrl_oper();
	}
	
	do{
		//Fixed by rjf at 20150915
		if(0 == ret){
			//	No matter what opt is, call mdmEmodem_restart_power_ctrl_oper() once mdmEmodem_power_switch(0x01)
			//failed once.
			mdmEmodem_restart_power_ctrl_oper();
		}
		
		ret = mdmEmodem_power_switch(0x01);
	}while(ret == 0);
	poll(NULL, 0, 2000);
//	mdmEmodem_uart_keep_awake(); //This operation is moved into mdmEmodem_check_startup().
	isMdmEmodemRunning = true;
	return;
}

void mdmEmodem_shutdown(void)
{
	int ret = -99;
	
	do{
		ret = mdmEmodem_power_switch(0x00);
	}while(ret == 0);
  
	isMdmEmodemRunning = false;
	
	#ifdef FEATURE_MDMEmodem_BOOT_STAT_QRY
		write_file(MDMEmodem_BOOT_STAT_FILE_PATH, 0, "0");
	#endif
	
	return;
}
#endif // FEATURE_ENABLE_MDMEmodem_SOFT_STARTUP

//Description:
//	Check if mdmEmodem is awake or not. And update isMdmEmodemRunning inside.
//Return Values:
//	1 : MdmEmodem is awake.
//	0 : MdmEmodem is standby.
int mdmEmodem_check_if_running(void)
{
  int status = -1;

  if(isMdmEmodemRunning)
    status = 1;
  else
    status = 0;

  return status;
}

int mdmEmodem_check_serial_port_available(void)
{
	#define MCSPA_RSP_BUF_LEN (64)
	#define MAX_QRY_NUM (3) //Changed by rjf at 20151104
	
	char cmd[] = "AT\r\n", rsp_buf[MCSPA_RSP_BUF_LEN] = {0};
	int ret = -1, written_bytes = -1, read_bytes = -1, 
		fd = at_config.serial_fd, maxfdplus1 = fd+1, 
		qry_num = 0;
	fd_set rfd;
	struct timeval tv;
	
	FD_ZERO(&rfd);
	FD_SET(fd, &rfd);
	memset(&tv, 0, sizeof(tv));
	
	__WRITE_CMD__:
	written_bytes = write(fd, cmd, strlen(cmd));
	if(written_bytes < 0 || written_bytes == 0){
		if(written_bytes < 0 && (errno == EINTR || errno == EAGAIN))
			goto __WRITE_CMD__;
		if(written_bytes == 0)
			goto __WRITE_CMD__;
		
		LOG("ERROR: write() failed. errno=%d.\n", errno);
		return -1;
	}
	
	//written_bytes > 0
__SELECT_FD__:
	tv.tv_sec = 0;
	tv.tv_usec = 500*1000; //Changed by rjf at 20151104
	ret = select(maxfdplus1, &rfd, NULL, NULL, &tv);
	LOG("select() returns %d.\n", ret);
	if(ret < 0){
		if(errno == EINTR)
			goto __SELECT_FD__;
		
		LOG("ERROR: select() failed. errno=%d.\n", errno);
		return -1;
	}else if(ret > 0){
		__READ_RSP__:
		memset(rsp_buf, 0, sizeof(rsp_buf));
		read_bytes = read(fd, rsp_buf, MCSPA_RSP_BUF_LEN);
		if(read_bytes < 0 || read_bytes == 0){
			if(read_bytes < 0 && (errno == EINTR || errno == EAGAIN))
				goto __READ_RSP__;
			if(read_bytes == 0)
				goto __READ_RSP__;
			
			LOG("ERROR: read() failed. errno=%d.\n", errno);
			return -1;
		}
		
		//read_bytes > 0
		LOG("       <-- \"%s\"\n", rsp_buf);
	}else{ //  if(ret == 0)
		qry_num ++;
		if(qry_num > MAX_QRY_NUM){
			return 0;
		}
		
		//qry_num <= MAX_QRY_NUM
		LOG("No rsp recved! Try to send AT cmd again...\n");
		goto __WRITE_CMD__;
	}
	
	return 1;
}

void mdmEmodem_reset_context(void)
{
  suspend_state = SUSPEND_STATE_IDLE;
  chk_net_state = CHK_NET_STATE_IDLE;
  uartconfig_state = CONF_STATE_IDLE;
  acq_data_state = ACQ_DATA_STATE_IDLE;
  isProcOfInitApp = false;
  isProcOfRecvInitInfo = true;
  isInitInfoRecved = false;
  is1stRunOnMdmEmodem = true;
  isSIM5360CpinReady = false;
}

void mdmEmodem_set_standby(void)
{
  pthread_mutex_lock(&standby_switch_mtx);

  LOG6("~~~~ mdmEmodem_set_standby ~~~~\n");

  gpio_set_value(19, 1);

  poll(NULL, 0, 2000); // Wait for 2s

  pthread_mutex_unlock(&standby_switch_mtx);
}

//Return Values:
// 1 : mdmEmodem has been waken up!
// 0 : mdmEmodem has been restarted.
// -1 : Internal error.
int mdmEmodem_set_awake(void)
{
#define MAX_OPER_NUM (3)

  int oper_num = 0, check_result = -1;
  bool isRetryToOperate = false;

  pthread_mutex_lock(&standby_switch_mtx);

  LOG6("~~~~ mdmEmodem_set_awake ~~~~\n");

__MDMEmodem_SET_AWAKE_GPIO_SET_VAL_0_AGAIN__:
	if(isRetryToOperate){
		gpio_set_value(19, 1);
		poll(NULL, 0, 500);
	}
	gpio_set_value(19, 0);
	poll(NULL, 0, 500); // Wait for 500ms

	check_result = mdmEmodem_check_serial_port_available();
	LOG("mdmEmodem_check_serial_port_available() returns %d.\n", check_result);
	if(-1 == check_result){
//		LOG("ERROR: mdmEmodem_check_serial_port_available() failed. \n");
		return -1;
	}else if(0 == check_result){
		oper_num ++;
		if(oper_num > MAX_OPER_NUM){
			LOG("NOTICE: Fail to wake up mdmEmodem for %d times! Now, go to restart it.\n", MAX_OPER_NUM);
			mdmEmodem_reset_context();
			gpio_mdmEmodem_reboot();
			openDev(1);
			return 0;
		}
		
		isRetryToOperate = true;
		LOG("Serial port is still not available. Now, go to pull gpio19 low again.\n");
		goto __MDMEmodem_SET_AWAKE_GPIO_SET_VAL_0_AGAIN__;
	}
	
	#ifdef FEATURE_MDMEmodem_MULTIPLE_SIGNAL_QUERY_AFTER_STANDBY_MODE_SWITCH
		#define BUFSZ (128)
		#define MAX_QRY_TIMES (2) //Changed by rjf at 20151104
		fd_set rfd;
		struct timeval tv;
		int read_bytes = -1, retry_times = 1, ret = -1, at_fd = at_config.serial_fd;
		char rsp_buf[BUFSZ] = {0};
		
		FD_ZERO(&rfd);
		FD_SET(at_config.serial_fd, &rfd);
		memset(&tv, 0, sizeof(tv));
		
		__WRITE_CMD__:
		writer(at_fd, "AT+CSQ", strlen("AT+CSQ"));
		
		__SELECT_FD__:
		tv.tv_sec = 0;
		tv.tv_usec = 100*1000; //Changed by rjf at 20151104
		ret = select(at_fd+1, &rfd, NULL, NULL, &tv);
		if(ret < 0){
			if(errno == EINTR)
				goto __SELECT_FD__;
		
			LOG("ERROR: select() failed.(1st) errno=%d.\n", errno);
			return -1;
		}else if(ret > 0){
			__READ_RSP__:
			memset(rsp_buf, 0, sizeof(rsp_buf));
			read_bytes = read(at_fd, rsp_buf, BUFSZ);
			if(read_bytes < 0 || read_bytes == 0){
				if(read_bytes < 0 && (errno == EINTR || errno == EAGAIN))
					goto __READ_RSP__;
				if(read_bytes == 0)
					goto __READ_RSP__;
				
				LOG("ERROR: read() failed.(1st) errno=%d.\n", errno);
				return -1;
			}
		
			//read_bytes > 0
			#ifndef FEATURE_DEBUG_LOG_FILE
				LOG("\n++++++++++++++++++++++++++++++ (retry_times: %d)\n", retry_times);
				LOG2("%s\n", rsp_buf);
				LOG2("++++++++++++++++++++++++++++++\n");
			#else
				pthread_mutex_lock(&log_file_mtx);
				LOG2("\n++++++++++++++++++++++++++++++ (retry_times: %d)\n", retry_times);
				LOG3("%s\n", rsp_buf);
				LOG3("++++++++++++++++++++++++++++++\n");
				pthread_mutex_unlock(&log_file_mtx);
			#endif
			
			if(NULL == strstr(rsp_buf, "OK")){
				LOG("Rsp \"OK\" not found. Continue to recv...\n");
				goto __SELECT_FD__;
			}else{				
				if(retry_times < MAX_QRY_TIMES){
					retry_times ++;
					goto __WRITE_CMD__;
				}
				
			}
		}else{ //  if(ret == 0)
			retry_times ++;
			if(retry_times < MAX_QRY_TIMES){	
				LOG("No rsp recved! Try to send \"AT+CSQ\" again... (retry_times: %d)\n", retry_times);
				goto __WRITE_CMD__;
			}
			
		}
		
		//retry_times <= MAX_QRY_TIMES
		//Read out all async rsp after sending 3 cmds "AT+CSQ".
		__SELECT_FD_2__:
		ret = select(at_fd+1, &rfd, NULL, NULL, &tv);
		if(ret < 0){
			if(errno == EINTR)
				goto __SELECT_FD_2__;
		
			LOG("ERROR: select() failed.(2nd) errno=%d.\n", errno);
			return -1;
		}else if(ret > 0){
			__READ_RSP_2__:
			memset(rsp_buf, 0, sizeof(rsp_buf));
			read_bytes = read(at_fd, rsp_buf, BUFSZ);
			if(read_bytes < 0 || read_bytes == 0){
				if(read_bytes < 0 && (errno == EINTR || errno == EAGAIN))
					goto __READ_RSP_2__;
				if(read_bytes == 0)
					goto __READ_RSP_2__;
				
				LOG("ERROR: read() failed.(2nd) errno=%d.\n", errno);
				return -1;
			}
		
			//read_bytes > 0
			#ifndef FEATURE_DEBUG_LOG_FILE
				LOG("\n------------------------------ (retry_times: %d)\n", retry_times);
				LOG2("%s\n", rsp_buf);
				LOG2("------------------------------\n");
			#else
				pthread_mutex_lock(&log_file_mtx);
				LOG2("\n------------------------------ (retry_times: %d)\n", retry_times);
				LOG3("%s\n", rsp_buf);
				LOG3("------------------------------\n");
				pthread_mutex_unlock(&log_file_mtx);
			#endif
		}
	#endif
	
	pthread_mutex_unlock(&standby_switch_mtx);
	
	return 1;
}

//Description:
//	Check if standby switch is available for utilizing.
int mdmEmodem_check_if_standby_switch_available(void)
{
	int ret, try_lck_result = pthread_mutex_trylock(&standby_switch_mtx);
	
	if(try_lck_result == 0){
		ret = 1;
		pthread_mutex_unlock(&standby_switch_mtx);
	}else{
		if(errno == EBUSY){
			ret = 0;
			
			//Note:
			//		Setting mdmEmodem standby took place in EvtLoop thread. If errno was EBUSY, meanwhile EvtLoop had been
			//	terminated, mutex would be locked forever without unlocking.
			pthread_mutex_unlock(&standby_switch_mtx);
		}else{
			LOG("ERROR: pthread_mutex_trylock() failed. errno=%d.\n", errno);
			ret = -1;
		}
	}
		
	return ret;
}

//Description:
//		This function is for manufaturing test. So, even if ChannelNum was changed to -1, app would not
//	try to resume mdmEmodem.
//Param:
//	0x00 : Wake up the mdmEmodem.
//	0x01 : Set mdmEmodem to standby mode.
void mdmEmodem_standby_mode_switch(int ctrl_word)
{
	if(ctrl_word != 0 && ctrl_word != 1)
		return; //  Maintain the state of mdmEmodem
	
	if(0 == ctrl_word){
		if(isMdmEmodemRunning)
			return;
		
		if(1 == mdmEmodem_set_awake())
			isMdmEmodemRunning = true;
		
		//FIXME: What should we do when we fail to wake up mdmEmodem?
	}else{
		if(!isMdmEmodemRunning)
			return;
		
		mdmEmodem_set_standby();
		
		ACQ_CHANNEL_LOCK_WR;
		if(0 == ChannelNum){
			ChannelNum = -1; // See "Description"
		}else if(3 == ChannelNum){
			ChannelNum = 1;
		}
		RLS_CHANNEL_LOCK;
		isMdmEmodemRunning = false;
	}
	
	return;
}

//Return Values:
//	>=0 : Rssi val.
//	  -1  : Internal error.
int mdmEmodem_check_CSQ(void)
{
	int written_bytes = -1, rssi = -1;
	char cmd[] = "AT+CSQ\r\n";
	bool need_recv_OK = false;
	struct timeval tv, *ptv = NULL;
	
	__MDMEmodem_CHECK_CSQ_WRITE_AGAIN__:
	written_bytes = write(at_config.serial_fd, cmd, strlen(cmd));
	if(written_bytes < 0 || written_bytes == 0){
		if(written_bytes < 0 && errno != EINTR && errno != EAGAIN){
			LOG("ERROR: write() failed. errno=%d.\n", errno);
			return -1;
		}
		
		usleep(1000);
		goto __MDMEmodem_CHECK_CSQ_WRITE_AGAIN__;
	}else{
		int read_bytes = -1, maxfdp1 = at_config.serial_fd+1, select_result = -1;
		char rsp_buf[256] = {0};
		fd_set rfd;
		
		FD_ZERO(&rfd);
		FD_SET(at_config.serial_fd, &rfd);
		
__MDMEmodem_CHECK_CSQ_SELECT_AGAIN__:
		select_result = select(maxfdp1, &rfd, NULL, NULL, ptv);
		if(select_result < 0){
			if(errno == EINTR || errno == EAGAIN){
				goto __MDMEmodem_CHECK_CSQ_SELECT_AGAIN__;
			}else{
				LOG("ERROR: select() failed. errno=%d.\n", errno);
				return -1;
			}
		}else if(select_result == 0){
			LOG("select() returns 0. need_recv_OK=%d, rssi=%d.\n",
					need_recv_OK, rssi);
			if(true == need_recv_OK || -1 != rssi){
				goto __EXIT_OF_MDMEmodem_CHECK_CSQ__;
			}else{
				ptv = NULL;
				goto __MDMEmodem_CHECK_CSQ_WRITE_AGAIN__;
			}
		}
		
__MDMEmodem_CHECK_CSQ_READ_AGAIN__:
		memset(rsp_buf, 0, sizeof(rsp_buf));
		read_bytes = read(at_config.serial_fd, rsp_buf, sizeof(rsp_buf));
		if(read_bytes < 0 || read_bytes == 0){
			if(read_bytes < 0 && errno != EINTR && errno != EAGAIN){
				LOG("ERROR: read() failed. errno=%d.\n", errno);
				return -1;
			}
			
			usleep(1000);
			goto __MDMEmodem_CHECK_CSQ_READ_AGAIN__;
		}else{
			char *cur = NULL;
			
			cur = strstr(rsp_buf, "+CSQ:");
			if(cur == NULL){
				//"OK"
				if(NULL != strstr(rsp_buf, "OK")){
					if(need_recv_OK){
						//"+CSQ: X,Y" recved already.
						need_recv_OK = false;
						LOG("Rsp \"OK\" recved.\n");
						goto __EXIT_OF_MDMEmodem_CHECK_CSQ__;
					}else{
						//"+CSQ: X,Y" not recved.
						LOG("NOTICE: \"+CSQ: X,Y\" missed but \"OK\" recved. Now write cmd again.\n");
						ptv = NULL;
						goto __MDMEmodem_CHECK_CSQ_WRITE_AGAIN__;
					}
				}
				
				//Wait 1s for recving rsp of AT cmd "AT+CSQ"
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				ptv = &tv;
				
				//Neither "+CSQ: X,Y" nor "OK"
				LOG("ERROR: Unexpected rsp str found!Rsp: \"%s\". Continue to read...\n", rsp_buf);
				goto __MDMEmodem_CHECK_CSQ_READ_AGAIN__;
			}else{
				char *p_OK = NULL;
				
				for( cur+=6, rssi=0; *cur!=','; cur++){
					rssi *= 10;
					rssi += *cur-'0';
				}
				
				p_OK = strstr(cur, "OK");
				if(p_OK == NULL){
					need_recv_OK = true;
					LOG("Rsp str \"OK\" missed. Continue to read more.\n");
					//Avoid waiting for rsp str "OK" forever.
					tv.tv_sec = 1;
					tv.tv_usec = 0;
					ptv = &tv;
					goto __MDMEmodem_CHECK_CSQ_READ_AGAIN__;
				}
			} // cur != NULL
		} // read_bytes > 0
	} // written_bytes > 0
	
__EXIT_OF_MDMEmodem_CHECK_CSQ__:
  return rssi;
}

//Return Values:
//	>=0 : Reg stat.
//	  -1  : Internal error.
int mdmEmodem_check_CGREG(void)
{
	int written_bytes = -1, val = -1;
	char cmd[] = "AT+CGREG?\r\n";
	bool need_recv_OK = false;
	struct timeval tv, *ptv = NULL;
	
	__MDMEmodem_CHECK_CGREG_WRITE_AGAIN__:
	written_bytes = write(at_config.serial_fd, cmd, strlen(cmd));
	if(written_bytes < 0 || written_bytes == 0){
		if(written_bytes < 0 && errno != EINTR && errno != EAGAIN){
			LOG("ERROR: write() failed. errno=%d.\n", errno);
			return -1;
		}
		
		usleep(1000);
		goto __MDMEmodem_CHECK_CGREG_WRITE_AGAIN__;
	}else{
		int read_bytes = -1, maxfdp1 = at_config.serial_fd+1, select_result = -1;
		char rsp_buf[256] = {0};
		fd_set rfd;
		
		FD_ZERO(&rfd);
		FD_SET(at_config.serial_fd, &rfd);
		
__MDMEmodem_CHECK_CGREG_SELECT_AGAIN__:
		select_result = select(maxfdp1, &rfd, NULL, NULL, ptv);
		if(select_result < 0){
			if(errno == EINTR || errno == EAGAIN){
				goto __MDMEmodem_CHECK_CGREG_SELECT_AGAIN__;
			}else{
				LOG("ERROR: select() failed. errno=%d.\n", errno);
				return -1;
			}
		}else if(select_result == 0){
			LOG("select() returns 0. need_recv_OK=%d, val=%d.\n",
					need_recv_OK, val);
			if(true == need_recv_OK || -1 != val){
				goto __EXIT_OF_MDMEmodem_CHECK_CGREG__;
			}else{
				ptv = NULL;
				goto __MDMEmodem_CHECK_CGREG_WRITE_AGAIN__;
			}
		}
		
__MDMEmodem_CHECK_CGREG_READ_AGAIN__:
		memset(rsp_buf, 0, sizeof(rsp_buf));
		read_bytes = read(at_config.serial_fd, rsp_buf, sizeof(rsp_buf));
		if(read_bytes < 0 || read_bytes == 0){
			if(read_bytes < 0 && errno != EINTR && errno != EAGAIN){
				LOG("ERROR: read() failed. errno=%d.\n", errno);
				return -1;
			}
			
			usleep(1000);
			goto __MDMEmodem_CHECK_CGREG_READ_AGAIN__;
		}else{
			char *cur = NULL;
			
			cur = strstr(rsp_buf, "+CGREG:");
			if(cur == NULL){
				//"OK"
				if(NULL != strstr(rsp_buf, "OK")){
					if(need_recv_OK){
						//"+CGREG: X,Y" recved already.
						need_recv_OK = false;
						LOG("Rsp \"OK\" recved.\n");
						goto __EXIT_OF_MDMEmodem_CHECK_CGREG__;
					}else{
						//"+CGREG: X,Y" not recved.
						LOG("NOTICE: \"+CGREG: X,Y\" missed but \"OK\" recved. Now write cmd again.\n");
						ptv = NULL;
						goto __MDMEmodem_CHECK_CGREG_WRITE_AGAIN__;
					}
				}
				
				//Wait 1s for recving rsp of AT cmd "AT+CGREG?"
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				ptv = &tv;
				
				//Neither "+CGREG: X,Y" nor "OK"
				LOG("ERROR: Unexpected rsp str found!Rsp: \"%s\". Continue to read...\n", rsp_buf);
				goto __MDMEmodem_CHECK_CGREG_SELECT_AGAIN__;
			}else{
				char *p_OK = NULL;
				
				for( cur+=10, val=0; *cur!='\r'&&*cur!='\n'&&*cur!='\0'; cur++){
					val *= 10;
					val += *cur-'0';
				}
				
				p_OK = strstr(cur, "OK");
				if(p_OK == NULL){
					need_recv_OK = true;
					LOG("Rsp str \"OK\" missed. Continue to read more.\n");
					//Avoid waiting for rsp str "OK" forever.
					tv.tv_sec = 1;
					tv.tv_usec = 0;
					ptv = &tv;
					goto __MDMEmodem_CHECK_CGREG_SELECT_AGAIN__;
				}
			} // cur != NULL
		} // read_bytes > 0
	} // written_bytes > 0
	
__EXIT_OF_MDMEmodem_CHECK_CGREG__:
  return val;
}


#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

int MdmEmodemSetCCLK(char *ptimeBuf)
{
	int written_bytes = -1, val = -1;
	char cclk_cmd[29] = "AT+CCLK=";
	bool need_recv_OK = false;
	struct timeval tv;

	static unsigned char count_times = 0;
	char pbuf[11];


	
	LOG("   ptimeBuf --> \"%s\"  \n", ptimeBuf);




	memset(pbuf,0,29);

	pbuf[0] = ptimeBuf[2];
	pbuf[1] = ptimeBuf[3];

	pbuf[2] = ptimeBuf[5];
	pbuf[3] = ptimeBuf[6];

	pbuf[4] = ptimeBuf[8];
	pbuf[5] = ptimeBuf[9];

	pbuf[6] = ptimeBuf[13];
	pbuf[7] = ptimeBuf[14];

	pbuf[8] = ptimeBuf[16];
	pbuf[9] = ptimeBuf[17];

	pbuf[10] = ptimeBuf[19];
	pbuf[11] = ptimeBuf[20];

	//memset(cclk_cmd,0,29);

	cclk_cmd[8] = '\"';
	cclk_cmd[9] = pbuf[0];
	cclk_cmd[10] = pbuf[1];
	cclk_cmd[11] = 0x2f; // /
	cclk_cmd[12] = pbuf[2];
	cclk_cmd[13] = pbuf[3];
	cclk_cmd[14] = 0x2f;
	cclk_cmd[15] = pbuf[4];
	cclk_cmd[16] = pbuf[5];
	cclk_cmd[17] = 0x2c;

	cclk_cmd[18] = pbuf[6];
	cclk_cmd[19] = pbuf[7];
	cclk_cmd[20] = 0x3a;
	
	cclk_cmd[21] = pbuf[8];
	cclk_cmd[22] = pbuf[9];
	cclk_cmd[23] = 0x3a;

	cclk_cmd[24] = pbuf[10];
	cclk_cmd[25] = pbuf[11];

	cclk_cmd[26] =  '\"';

	cclk_cmd[27] =  '\r';
	cclk_cmd[28] =  '\n';

	 LOG("  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&  \n");
 	 LOG("  111111111111111111111111111111111111111111111111111111111111111111111  \n");

	LOG("   cclk_cmd --> \"%s\"  \n", cclk_cmd);


	
	
	__MDMEmodem_CCLK_WRITE_AGAIN__:
	written_bytes = write(at_config.serial_fd, cclk_cmd, strlen(cclk_cmd));
	if(written_bytes < 0 || written_bytes == 0){
		if(written_bytes < 0 && errno != EINTR && errno != EAGAIN){
			LOG("ERROR: write() failed. errno=%d.\n", errno);
			return -1;
		}
		
		usleep(1000);
		goto __MDMEmodem_CCLK_WRITE_AGAIN__;
	}else{

		int read_bytes = -1, maxfdp1 = at_config.serial_fd+1, select_result = -1;
		char rsp_buf[256] = {0};
		fd_set rfd;
		
		FD_ZERO(&rfd);
		FD_SET(at_config.serial_fd, &rfd);

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		__MDMEmodem_CCLK_SELECT_AGAIN__:
		select_result = select(maxfdp1, &rfd, NULL, NULL, &tv);
		if(select_result < 0){
			if(errno == EINTR || errno == EAGAIN){
				goto __MDMEmodem_CCLK_SELECT_AGAIN__;
			}else{
				LOG("ERROR: select() failed. errno=%d.\n", errno);
				return -1;
			}
		}else if(select_result == 0){
			LOG("select() returns 0 timeout");

				goto __EXIT_OF_MDMEmodem_CCLK__;
			
		}


		__MDMEmodem_CCLK_READ_AGAIN__:
		memset(rsp_buf, 0, sizeof(rsp_buf));
		read_bytes = read(at_config.serial_fd, rsp_buf, sizeof(rsp_buf));
		if(read_bytes < 0 || read_bytes == 0){
			if(read_bytes < 0 && errno != EINTR && errno != EAGAIN){
				LOG("ERROR: read() failed. errno=%d.\n", errno);
				return -1;
			}
			
			usleep(1000);
			goto __MDMEmodem_CCLK_READ_AGAIN__;
		}else{
		
			char *cur = NULL;
			cur = strstr(rsp_buf, "+CCLK:");
			
			if(cur == NULL){
				//"OK"
				if(NULL != strstr(rsp_buf, "OK")){
					
						LOG("Rsp \"OK\" recved.\n");
						goto __EXIT_OF_MDMEmodem_CCLK__;
						
				}else{
						//"OK" not recved.
						count_times++;
						LOG("NOTICE: \"OK\" missed  Now write cmd again.count_times = %d \n", count_times);
						//ptv = NULL;
						if (count_times > 3){
							
							goto __EXIT_OF_MDMEmodem_CCLK__;
							
						}else{
						
							goto __MDMEmodem_CCLK_WRITE_AGAIN__;

						}
					
				}
				
			}
			
		}

	}


__EXIT_OF_MDMEmodem_CCLK__:

  LOG("Rsp \"OK\" return 0 \n");
	
  return 0;
	

}



#endif

void enqueue_and_resume_mdmEmodem(rsmp_transmit_buffer_s_type *rc, rsmp_state_s_type state)
{
  //isWaitToSusp is definitely false now.

  add_node(rc, state);

  if(!mdmEmodem_check_if_running()){
    LOG("Now, go to resume mdmEmodem.\n");
    w_resumeMdmHdlr();
  }else{
    LOG("ERROR: Wrong isMdmEmodemRunning (1) found!\n");
  }
}

#ifdef FEATURE_ENABLE_QUEUE_SYSTEM
//Precondition:
//Once you prepare to call this function, an available network link must be exsited and accessible.
//Return Values:
// 1 : First node has been dequeued and sent out.
// 0 : No node has been dequeued.
//  -1 : Internal error.
//Note:
//Not acquire acq_data_state_mutex inside (but outside). It is only called by r_acqDataHdlr().
static int dequeue_and_send_req(rsmp_transmit_buffer_s_type *rc)
{
  Queue_Node *p_node = NULL;
  int ret = -1;

  LOG6("~~~~~ dequeue_and_send_req ~~~~~\n");

  //Dequeue
  p_node = remove_first_node_from_queue();
  if(p_node == NULL){
    LOG("ERROR: No node acquired. remove_first_node_from_queue() return null.\n");
    return ret;
  }

  if(REQ_TYPE_STATUS == p_node->last_req_type && StatInfMsgSn > p_node->request.StatInfMsgSn)
  {
    //Dump out-of-date stat inf node
    LOG("Current sn: %d, node sn: %d. So dump this stat inf node!\n",
        StatInfMsgSn, p_node->request.StatInfMsgSn);

    remove_node(p_node);
    //Not need to decrease StatInfReqNum by 1. StatInfReqNum has been decreased in add_node().
    ret = 0;
  }else
  {
    reset_rsmp_transmit_buffer(rc); // Add by rjf at 2015-6-16-1656
    rc->size = p_node->request.size;
    rc->data = malloc(rc->size);
    memset(rc->data, 0, rc->size);
    memcpy(rc->data, p_node->request.data, rc->size);

    flow_info.flow_state = p_node->last_flow_state;
    flow_info.req_type = p_node->last_req_type;

    remove_node(p_node);

    acq_data_state = ACQ_DATA_STATE_CIPSEND_1;
    w_acqDataHdlr();

    ret = 1;
  }

  return ret;
}
#endif

//Function:
// It is convenient for locking and unlocking ChannelLock.
//Param:
// opt = X: Check if ChannelNum equals X.
int check_ChannelNum(const int opt)
{
  int ret = -1;

#ifdef FEATURE_ENABLE_CHANNEL_LOCK_LOG
  LOG6("~~~~ check_ChannelNum ~~~~\n");
#endif

  ACQ_CHANNEL_LOCK_RD;
  if(ChannelNum == opt){
    ret = 1;
  }else{
    ret = 0;
  }
  RLS_CHANNEL_LOCK;

  return ret;
}

//Param:
//opt = 0x00 : Req to acq mdm9215 inf.
 void notify_ListenerLoop_el(char opt)
{
	int n = -1;
	
	LOG6("~~~~ notify_ListenerLoop_el start ~~~~\n");
	
	do{
		if(n == 0)
			LOG("write() failed. n=%d.\n", n);
		n = write(EvtLoop_2_Listener_pipe[1], (void *)(&opt), 1);
	}while ((n<0 && errno==EINTR) || n==0);
	
	if(n<0){
		LOG("ERROR: write() error. n=%d, errno=%d.\n", 
				n, errno);
	}
	
	LOG6("~~~~ notify_ListenerLoop_el end ~~~~\n");
	return;
}

#define RESET_GLOBAL_VAR \
{ \
  LOG("Flow Ends Here!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); \
}

//ret=1 will go out of macro without a return
#define READER__CHK_NET(macro) \
{ \
  ret = reader(at_config.serial_fd, &ptr); \
  if(ret == 0){ \
    LOG("reader() error.\n"); \
    return; \
  }else if(ret == 2){ \
    if(*ATBufCur == '\0'){ \
      INIT_ATBUF; \
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL); \
      add_evt (&at_config.AT_event, 'r'); \
      return; \
    }else{ \
      LOG("There is still something unread in ATBuf.\n"); \
      goto macro; \
    } \
  }else if(ret == 4){ \
    if(*ATBufCur == '\0' && chk_net_state == CHK_NET_STATE_IDLE){ \
      LOG("\"ATE0\" rsp \"OK\" found.\n"); \
    } \
  }else if(ret == 10){ \
    INIT_ATBUF; \
    set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL); \
    add_evt (&at_config.AT_event, 'r'); \
    return; \
  } \
}

//Note:
//	1. When reader() ret 8, it doesn't need to send offline ind to other process.
//	2. Not care FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE any more in READER__ACQ_DATA
#define READER__ACQ_DATA(macro) \
{ \
	ret = reader(at_config.serial_fd, &ptr); \
	if(ret == 0){ \
		LOG("ERROR: reader() error.\n"); \
		RESET_GLOBAL_VAR; \
		break; \
	}else if(ret == 2){ \
		if(*ATBufCur == '\0'){ \
			INIT_ATBUF; \
			set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL); \
			add_evt (&at_config.AT_event, 'r'); \
			break; \
		}else{ \
			LOG("There is still something unread in ATBuf.\n"); \
			goto macro; \
		} \
	}else if(ret == 5){ \
		if(acq_data_state == ACQ_DATA_STATE_CIPSEND_2 || \
			acq_data_state == ACQ_DATA_STATE_CATR_1 || \
			acq_data_state == ACQ_DATA_STATE_CATR_2 \
		  ){ \
				ACQ_CHANNEL_LOCK_WR; \
		   		if(0 == ChannelNum){ \
					ChannelNum = -1; \
				}else if(3 == ChannelNum){ \
					ChannelNum = 4; \
				}else{ \
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", \
							ChannelNum, acq_data_state); \
					RESET_GLOBAL_VAR; \
					break; \
				} \
				RLS_CHANNEL_LOCK; \
		}else{ \
			LOG("ERROR: Wrong acq_data_state(%d) found! The macro should not appear at this acq_data_state.\n", \
					acq_data_state); \
			RESET_GLOBAL_VAR; \
			break; \
		} \
		if(acq_data_state == ACQ_DATA_STATE_CIPSEND_2) \
			add_node(&g_rsmp_transmit_buffer[THD_IDX_EVTLOOP], flow_info.flow_state); \
		GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr); \
	}else if(ret == 8){ \
		if(acq_data_state != ACQ_DATA_STATE_NETOPEN){ \
			LOG("ERROR: Wrong acq_data_state(%d) found! The val of ret(%d) dismatch with acq_data_state.\n", \
					ret, acq_data_state); \
			RESET_GLOBAL_VAR; \
			break; \
		} \
		isWaitRspError = true; \
		error_flag = 0x03; \
		if(*ATBufCur == '\0'){ \
			INIT_ATBUF; \
			set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL); \
			add_evt (&at_config.AT_event, 'r'); \
			break; \
		}else{ \
			LOG("There is still something unread in ATBuf.\n"); \
			goto macro; \
		} \
	}else if(ret == 10){ \
		INIT_ATBUF; \
		set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL); \
		add_evt (&at_config.AT_event, 'r'); \
		break; \
	}else if(ret == 11){ \
		if(ACQ_DATA_STATE_CIPSEND_1 != acq_data_state){ \
			LOG("ERROR: Wrong acq_data_state(%d) found! The val of ret(%d) dismatch with acq_data_state.\n", \
					ret, acq_data_state); \
			RESET_GLOBAL_VAR; \
			break; \
		} \
		if(*ATBufCur == '\0'){ \
			isWaitRspError = true; \
			error_flag = 0x01; \
			INIT_ATBUF; \
			set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL); \
			add_evt (&at_config.AT_event, 'r'); \
			break; \
		} \
		add_node(&g_rsmp_transmit_buffer[THD_IDX_EVTLOOP], flow_info.flow_state); \
		ACQ_CHANNEL_LOCK_WR; \
		if(0 == ChannelNum){ \
			ChannelNum = -1; \
			msq_send_offline_ind(); \
		}else if(3 == ChannelNum){ \
			ChannelNum = 4; \
			msq_send_offline_ind(); \
		}else{ \
			LOG("ERROR: ret=11, acq_data_state=%d. Wrong ChannelNum (%d) found!\n", acq_data_state, ChannelNum); \
			RESET_GLOBAL_VAR; \
			break; \
		} \
		RLS_CHANNEL_LOCK; \
		suspend_state = SUSPEND_STATE_CIPCLOSE; \
		w_suspendMdmHdlr(); \
		break; \
	}else if(ret == 12){ \
		if(ACQ_DATA_STATE_CIPSEND_1 != acq_data_state){ \
			LOG("ERROR: Wrong acq_data_state(%d) found! The val of ret(%d) dismatch with acq_data_state.\n", \
					ret, acq_data_state); \
			RESET_GLOBAL_VAR; \
			break; \
		} \
		if(*ATBufCur == '\0'){ \
			isWaitRspError = true; \
			error_flag = 0x02; \
			INIT_ATBUF; \
			set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL); \
			add_evt (&at_config.AT_event, 'r'); \
			break; \
		} \
		add_node(&g_rsmp_transmit_buffer[THD_IDX_EVTLOOP], flow_info.flow_state); \
		ACQ_CHANNEL_LOCK_WR; \
		if(0 == ChannelNum){ \
			ChannelNum = (-1); \
			msq_send_offline_ind(); \
		}else if(3 == ChannelNum){ \
			ChannelNum = (4); \
			msq_send_offline_ind(); \
		} \
		RLS_CHANNEL_LOCK; \
		GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr); \
	}else if(ret == 14){ \
		if(isWaitRspError){ \
			isWaitRspError = false; \
			if(error_flag == 0x01){ \
				error_flag = 0x00; \
				add_node(&g_rsmp_transmit_buffer[THD_IDX_EVTLOOP], flow_info.flow_state); \
				ACQ_CHANNEL_LOCK_WR; \
				if(0 == ChannelNum){ \
					ChannelNum = -1; \
					msq_send_offline_ind(); \
				}else if(3 == ChannelNum){ \
					ChannelNum = 4; \
					msq_send_offline_ind(); \
				}else{ \
					LOG("ERROR: ret=14, acq_data_state=%d. Wrong ChannelNum (%d) found!\n", acq_data_state, ChannelNum); \
					RESET_GLOBAL_VAR; \
					break; \
				} \
				RLS_CHANNEL_LOCK; \
				suspend_state = SUSPEND_STATE_CIPCLOSE; \
				w_suspendMdmHdlr(); \
				break; \
			}else if(error_flag == 0x02){ \
				error_flag = 0x00; \
				add_node(&g_rsmp_transmit_buffer[THD_IDX_EVTLOOP], flow_info.flow_state); \
				ACQ_CHANNEL_LOCK_WR; \
				if(0 == ChannelNum){ \
					ChannelNum = (-1); \
					msq_send_offline_ind(); \
				}else if(3 == ChannelNum){ \
					ChannelNum = (4); \
					msq_send_offline_ind(); \
				} \
				RLS_CHANNEL_LOCK; \
				GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr); \
			}else if(error_flag == 0x03){ \
				error_flag = 0x00; \
				GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr); \
			}else{ \
				LOG("ERROR: Wrong error_flag (0x%02x) found!\n", error_flag); \
				RESET_GLOBAL_VAR; \
				break; \
			} \
		}\
	} \
}


void power_up_virtual_usim(void)
{
  LOG6("~~~~ power_up_virtual_usim ~~~~\n");

  queue_clr_apdu_node(); // Add by rjf at 20150803

  isEnableManualApdu = false;
  
#ifdef FEATURE_NEW_APDU_COMM 
  APDU_setting.proc_step = 0; //reset APDU_setting.proc_step
#endif //FEATURE_NEW_APDU_COMM

  isRstingUsim = false; //Add by rjf at 201510281717
  //Add by rjf at 20150812 (Remove the mark of recving the URC report "+SIMCARD: NOT AVAILABLE".)
  URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_PWR_DWN] = 0x00;
  //Add by rjf at 20151026 (Remove the mark of recving the URC report "+SIMCARD: ILLEGAL SESSION".)
  URC_special_mark_array[SPEC_URC_ARR_IDX_VIRT_USIM_SESSION_ILLEGAL] = 0x00;


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
#endif 

  pthread_mutex_lock(&act_uim_mtx);
  proto_update_flow_state(FLOW_STATE_SET_OPER_MODE);
  notify_Sender(QMISENDER_SET_ONLINE);
  while(isUimActed == false){
    pthread_cond_wait(&act_uim_cond, &act_uim_mtx);
  }
  isUimActed = false; // reset
  pthread_mutex_unlock(&act_uim_mtx);

  isRegOffForPoorSig = false; // reset
}

//Description:
//Acquire the status info of mdm9215 and assembly the private protocol pack.
//Note:
//Check if proto_update_flow_state(FLOW_STATE_STAT_UPLOAD); is needed after calling this function.
static  void status_info_upload_preparatory_work(void)
{
  int length;
  Thread_Idx_E_Type cur_tid = get_thread_idx();
  rsmp_transmit_buffer_s_type *p_req_cfg = NULL;

  if(THD_IDX_MAX != cur_tid)
  {
    p_req_cfg = &g_rsmp_transmit_buffer[cur_tid];
  }else{
    LOG("ERROR: Illegal tid found! cur_tid=%u.\n", cur_tid);
    p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP]; //FIXME
  }

  if(THD_IDX_LISTENERLOOP != get_thread_idx())
  {
    pthread_mutex_lock(&el_req_mtx);
    notify_ListenerLoop_el(0x00);
    while(!el_reqed){
      pthread_cond_wait(&el_req_cnd, &el_req_mtx);
    }
    el_reqed = false; // reset 
    pthread_mutex_unlock(&el_req_mtx);
  }else{
#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
    StatInfReqNum ++;
#endif
    get_dev_and_net_info_for_status_report_session();
  }

  reset_rsmp_transmit_buffer(p_req_cfg);
  p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_STATUS, &sim5360_dev_info, &rsmp_dev_info);
  p_req_cfg->size = length;
  p_req_cfg->StatInfMsgSn = StatInfMsgSn; 
}

//Note:
//Please acquire ChannelLock before using this function and releasing it after.
//Modification Records:
//2015-8-18 : Add the examination if ChannelNum is valid.
//2015-8-19 : Remove the operation of acquiring and releasing ChannelLock.
//2015-10-14 : Add trigger_EventLoop() after w_acqDataHdlr().
//2015-10-29 : Send req config all by notify_EvtLoop(0x01).
void upload_status_info_and_power_up_virt_USIM_later(void)
{
  isNeedToNotifyServCondVar = false; // reset
  status_info_upload_preparatory_work();

  pthread_mutex_lock(&evtloop_mtx);
  push_flow_state(FLOW_STATE_STAT_UPLOAD);
  push_thread_idx(get_thread_idx());
  notify_EvtLoop(0x01);
  pthread_mutex_unlock(&evtloop_mtx);
}

static char * find_NextEOL(char *cur)
{
  //No need to worry about crossing border of ATBuf. It must has a EOF('\0') at the end.
  while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

  return *cur == '\0' ? NULL : cur;
}

char *skip_line_break(char *pos){
	char *cur = pos;
	
	for(; *cur=='\n'||*cur=='\r'; cur++);
	return cur;
}

//Note:
//	'\0' is regarded as valid char.
char *skip_valid_chars(char *pos){
	char *cur = pos;
	
	for(; *cur!='\n'&&*cur!='\r'; cur++);
	return cur;
}

//NOTE: It is only fit with string, in which '\0' is seen as an invalid char.
//Return value:
//	0: Error.
//	1: Success or expect to continue reading.
//	2: read() error. errno = Resource temporarily unavailable.
static int read_line(int fd)
{
	int count;
	//ptr_read pointed the position which read() should start from here.
    char *ptr_read = NULL;
	//ptr_eol pointed the first EOL char in the line.
    char *ptr_eol = NULL;


    //"*ATBufCur == '\0' " means "buffer consumed completely". 
    //If it points to a character, then the buffer continues until a \0.
    //ATBufCur is initialized as '\0'
    if (*ATBufCur == '\0'){
        memset(ATBuf, 0, sizeof(ATBuf));
		// *ATBufCur = '\0';
        ATBufCur = ATBuf;
		line_head = ATBuf;
        ptr_read = ATBuf;
    }else{
        //There's still some unread data in the buffer.
		
        while (*ATBufCur == '\r' || *ATBufCur == '\n')
            ATBufCur++;

		line_head = ATBufCur;
        ptr_eol = find_NextEOL(ATBufCur);
		ptr_read = ATBufCur + strlen(ATBufCur);
    }

	//NOTE: ptr_eol == NULL: line is not complete. 
	//		   ptr_eol == '\r'(or '\n'?): line is complete. But in the end *ptr_eol will be replaced by '\0'.
    while (ptr_eol == NULL) {
		
        if (0 == AT_BUF_LEN - (ptr_read - ATBuf)) {
            LOG("ERROR: Too much data recved. More buf needed.\n");
            ATBufCur = ATBuf;
            *ATBufCur = '\0';
            ptr_read = ATBuf;
        }

        do {
            count = read(fd, ptr_read, AT_BUF_LEN-(ptr_read-ATBuf));
			//NOTE: New data are stored into the memory pointed by ptr_read
        }while(count < 0 && errno == EINTR);

        if(count > 0){
            ptr_read[count] = '\0';

            while (*ATBufCur == '\r' || *ATBufCur == '\n')
                ATBufCur++;

			line_head = ATBufCur;
			ptr_eol = find_NextEOL(ATBufCur);
            ptr_read += count;
			//NOTE: It will exit from while loop if find_NextEOF() encounter a '\r' or '\n'.
			//		   If '\0' is encountered, while loop will continue.
        } else if (count <= 0) {
            if(count == 0) {
                LOG("EOF reached.\n");
				return 1;
            }else{
                LOG("ERROR: read() failed. errno=%d. \n", errno);
				if(errno == EAGAIN)
					return 2;
				else
					return 0;
            } // if(count<0)
        } // else if(count<=0)
    }

    *ptr_eol = '\0';
    ATBufCur = ptr_eol + 1;
		
	for(; (*ATBufCur=='\r'||*ATBufCur=='\n') && ATBufCur<ptr_read; ATBufCur++);
	//ATBufCur will point to first sign, which is nor an NL neither a CR behind *ptr_eol.

    return 1;
}

//function:
//	Simply parse the line and check if it is "+STIN: 21" or "+CIPEVENT:...".
//return value:
//	0: Unexpected error.
//	1: AT cmd response or unsolicited report found.
//	2: "+STIN: 25" or "+STIN: 21" received.
//	3: "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" received.
//	4: "OK" received. It belong to last command response.
//	5: "+CIPCLOSE: 0,1" received.
//	6: "+IP ERROR: Network is already opened" received.
//	7: "+CIPOPEN: 0,4" received.
//	8: Received "+CPIN: SIM PIN" or "+CPIN: SIM PUK" and etc. It means to unlock the UIM.
//	9: Startup messages received. Like "START", "OPL UPDATING", "PB DONE", etc.
//	10: "+CIPERROR: 2" received.
//	11: "+CIPERROR: 4" received.
//	12: "+SIMCARD: NOT AVAILABLE" received.
//	13: "ERROR" received.
//	14: "+CIPCLOSE: 0,4" received.
//	15: "+NETCLOSE: 2" received.


static int parse_line(void)
{
  char *head = line_head;

  if(*head == '\0'){
    LOG("ATBuf is empty.\n");
    return 0;
  }
  else if( *head == '\r' || *head == '\n' ){
    LOG("ATBuf is not processed properly.\n");
    return 0;
  }

  if (strcmp(head, "+STIN: 21") == 0 || strcmp(head, "+STIN: 25") == 0){
    timeoutCount++;
    LOG("\"+STIN: 21\" and \"+STIN: 25\" found %d time(s).\n", timeoutCount);
    return 2;
  } else if (strcmp(head, NetClosedUnsol) == 0){
    return 3;
  } else if (strcmp(head, "OK") == 0){
    return 4;
  } else if (strcmp(head, PeerClosedUnsol) == 0) {
    return 5;
  } else if (strcmp(head, "+IP ERROR: Network is already opened") == 0) {
    return 6;
  } else if (strcmp(head, "+CIPOPEN: 0,4") == 0) {
    return 7;
  } else if (strncmp(head, "+CIPERROR: 2", sizeof("+CIPERROR: 2")-1) == 0) {
    return 10;
  } else if (strncmp(head, "+CIPERROR: 4", sizeof("+CIPERROR: 4")-1) == 0) {
    return 11;
  } else if (strncmp(head, "+SIMCARD: NOT AVAILABLE", sizeof("+SIMCARD: NOT AVAILABLE")-1) == 0){
    return 12;
  } else if (strncmp(head, "ERROR", sizeof("ERROR")-1) == 0){
    return 13;
  } else if (strncmp(head, "+CIPCLOSE: 0,4", sizeof("+CIPCLOSE: 0,4")-1) == 0) {
    return 14;
  } else if (strncmp(head, "+NETCLOSE: 2", sizeof("+NETCLOSE: 2")-1) == 0) {
    return 15;
  }
  else if (strcmp(head, "+CPIN: ") == 0 && strcmp(head, "+CPIN: READY") != 0) {
    LOG("%s found.\n", head);
    return 8;
  } else {
    int cnt = 0;
    for(; cnt < 7; cnt++)
      if(strcmp(head, InitUnsols[cnt]) == 0)
        break;
    if(cnt < 7){
      LOG("Startup message received. %s found.\n", InitUnsols[cnt]);
      return 9;
    }
  //Else, see it as a valid response
  } // else

  timeoutCount = 0;
  return 1;
}

//return value:
// 0: Initialization failed, or initializing msg not received.
// 1: Initializing msg received completely.
// 2: Initializing msg received partly.
// 3: USIM card in mdmEmodem is pined.
// 4: Unexpected incoming message received.
// 5: Startup messages missed. //Not used!!!
static int parse_initial_lines(void)
{
	char *cur = line_head;
	char *head = cur;
	int count;

	if(*cur == '\0'){
		LOG("ERROR: No data found in ATBuf.\n");
		return 0;
	}else if( *cur == '\r' || *cur == '\n' ){
		//The head of string can't be '\r' or '\n'
		LOG("ERROR: ATBuf is not processed properly.\n");
		return 0;
	}

	for(count=0;  count<=6;  count++)
		if(strcmp(head, InitUnsols[count]) == 0){
			isInitInfoRecved = true;
			break;
		} // if

	if(count == 7){
		if (strcmp(head, "+STIN: 21") == 0 || strcmp(head, "+STIN: 25") == 0){
			timeoutCount++;
			LOG("\"+STIN: 21\" and \"+STIN: 25\" found %d time(s).\n", timeoutCount);
			return 2;
		}
		else if (strcmp(head, InitUnsols[1]) == 0){
			timeoutCount = 0;
			return 3;
		}
		else{
			LOG("Unexpected incoming message received. Message: \"%s\"\n", head);
			timeoutCount = 0;
			return 4;
		}
	}
	else{
		timeoutCount = 0;
		return 2;
	}
}

//fd for supporting multi port
//data for parsed report or response
//return value:
// 0: Unexpected error.
// 1: Incoming messages received completely. 
// 2: Incoming messages received partly. Or "+STIN: 25""+STIN: 21" received. Or
//    startup messages of mdmEmodem.
// 3: "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" received. (parse_line().)
// 4: "OK" received. (parse_line().)
//	5: "+CIPCLOSE: 0,1" received. (parse_line().)
//	6: Initializing messages missed. (parse_initial_line().)
//	7: "+CPIN: SIM PIN" received. (parse_initial_line(), parse_line().)
//	8: "+IP ERROR: Network is already opened" received. (parse_line().)
//	9: "+CIPOPEN: 0,4" received. (parse_line().)
//	10: read() error. errno = Resource Temporarily Unavailable.
//	11: "+CIPERROR: 2" received. CIPSEND but not NETOPEN.
//	12: "+CIPERROR: 4" received. CIPSEND but not CIPOPEN.
//	13: "+SIMCARD: NOT AVAILABLE" received.
//	14: "ERROR" received.
//	15: "+CIPCLOSE: 0,4" received.
// 16: "+NETCLOSE: 2" received.
//NOTE1: In case of reading unprocessed data after first call of read_line(),
//       we can't initialize ATBuf in reader()!!!
//NOTE2: If there is still something unread in the ATBuf, reader will not return but keep running
//       to read remaining content when isProcOfRecvInitInfo is true.(This is for initUnsolHdlr.) And when isInitProcess
//       is false, these actions will leave to some functions outside reader().
static int reader(int fd, char **data_ptr)
{
  int result = 0, ret_val = -1;

  if( fd<0 || data_ptr==NULL ){
    LOG("ERROR: Wrong param(s) input.\n");
    return 0;
  }

  *data_ptr = NULL;

__READ_LINE__:
  result = read_line(fd);
  LOG("Debug:result read_line =.%d )...\n", result);
  if(result == 0){
    LOG("ERROR: read_line() error.\n");
    return 0;
  }else if (result == 2){
    return 10;
  }

   LOG("Debug:isProcOfRecvInitInfo =.%d time)...\n", isProcOfRecvInitInfo);

  if (isProcOfRecvInitInfo == false)
  {
    LOG("     <-- \"%s\"  \n", line_head);
    result = parse_line();
    LOG("Debug:result parse_line =.%d )...\n", parse_line);

    switch(result){
    case 1:
    case 4:
    {
      memset(RespArray, 0, sizeof(RespArray));
      memcpy(RespArray, line_head, strlen(line_head)+1);
      *data_ptr = RespArray;
    }
    case 2:
    case 3:
    case 5:
    {
      ret_val = result;
      break;
    }
    case 6:
    {
      ret_val =  8;
      break;
    }
    case 7:
    {
      memset(RespArray, 0, sizeof(RespArray));
      memcpy(RespArray, line_head, strlen(line_head)+1);
      *data_ptr = RespArray;
	  LOG("   Debug:RespArray  <-- \"%s\"  \n", RespArray);
      ret_val =  9;
      break;
    }
    case 8:
    {
      LOG("NOTICE: PIN code needed!\n");
      ret_val =  7;
      break;
    }
    case 9:
    {
      ret_val =  2;
      break;// Need to receive more.
    }

    case 10:
    case 11:
    case 12:
    {
#ifdef FEATURE_MDMEmodem_RST_FOR_SIMCARD_NOT_AVAIL
      if(isSIM5360CpinReady )
      {
        LOG("NOTICE: USIM card of mdmEmodem becomes unavailable unexpectedly! Now, restart the whole device.\n");
       //system("reboot");
       yy_popen_call("reboot -f");
      }
#endif // FEATURE_MDMEmodem_RST_FOR_SIMCARD_NOT_AVAIL
    }
    case 13:
    case 14:
    case 15:
    {
      //ret_val=result+1 is a coincidence. No logic here.
      ret_val = result+1;

      if(result == 13 || result == 14 || result == 15){
        memset(RespArray, 0, sizeof(RespArray));
        memcpy(RespArray, line_head, strlen(line_head)+1);
        *data_ptr = RespArray;
		 LOG("   Debug:RespArray  <-- \"%s\"  \n", RespArray);
      }

      break;
    }
    default:
    {
      LOG("parse_line() error. Unknown message received.\n");
      ret_val = 0;
      break;
    }
    } // switch(result)
  }else
  { // isProcOfRecvInitInfo == true
    LOG("     <-- \"%s\"  \n", line_head);

    result = parse_initial_lines();

    if((result==2 && *ATBufCur!='\0') || (result==4 && *ATBufCur!='\0')){
      LOG("There is still something unread in the ATBuf. Now, go to call read_line() again.\n");
      goto __READ_LINE__;
    }

    switch(result)
    {
    case 0:
    case 1:
    case 2:
    {
      ret_val = result;
      break;
    }
    case 3:
    {
      LOG("NOTICE: Pin code needed.\n");
      ret_val = 7;
      break;
    }
    case 4:
    {
      LOG("Unexpected incoming message received.\n");
      ret_val = 2;
      break;
    }
    case 5:
    {
      LOG("Initial msg missed. Now, go to check net status.\n");
      ret_val = 6;
      break;
    }
    default:
    {
      //No way to reach here
      ret_val = 0;
      break;
    } // default
    } // switch(result)
  } // if(isProcOfRecvInitInfo == true)

  return ret_val;
}

//return value:
// 0: Unexpected error
// 1: Received '>'
// 2: "+STIN: 21" received
// 3: "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" received.
// 4: "+CIPCLOSE: 0,1" received.
// 5: Startup messages of mdmEmodem received.
// 6: Unexpected message received.
// 7: read() failed. errno=11.
// 8: "+IPCLOSE: 0,1" received. Wait for later URC "+CIPERROR: 4".
// 9: "+IPCLOSE: 0,2" received. Wait for later URC "+CIPERROR: 2"?
// 10: "+CIPERROR: 4" received. "ERROR" will be followed.(CIPSEND but not CIPOPEN before)
// 11: "+CIPERROR: 2" received. "ERROR" will be followed.(CIPSEND but not NETOPEN before)
// 12: "ERROR" received.
static int reader_ext(int fd)
{
  int count;
  int ret = -1;

  static unsigned char recordCloseCount1 = 0;
  static unsigned char recordCloseCount2 = 0;

  if(fd < 0){
    LOG("ERROR: Wrong param(s) input.\n");
    ret = 0;
    goto __EXIT_OF_READER_EXT__;
  }

  do{
    count = read(fd, ATBuf, DEFAULT_BYTES_RECVED_FOR_ONCE);
  }while ((count<0 && errno==EINTR) || count==0);

   LOG("Debug:ChannelNum = %d.\n", ChannelNum);
   LOG("Debug:suspend_state = %d.\n", suspend_state);
   LOG("Debug:acq_data_state = %d.\n", acq_data_state);
   LOG("Debug:rsmp_net_info.basic_val = %d.\n", rsmp_net_info.basic_val);
   LOG("Debug isOnlineMode=%d, isUimPwrDwn=%d.\n", isOnlineMode, isUimPwrDwn);

  if(count > 0)
  {
    while (*ATBufCur== '\r' || *ATBufCur == '\n') ATBufCur++;
    
    line_head = ATBufCur;

    if(*line_head != '>' && *line_head != '+')
    {
      int cnt = 0;
      char *ptr = NULL;

      for(; cnt<7; cnt++)
        if(0 == strncmp(line_head, InitUnsols[cnt], InitUnsolLen[cnt]-1))
          break;
        
      if(cnt < 7){
        ptr = find_NextEOL(line_head);
        if(ptr != NULL)  *ptr = '\0';

        LOG("Initial message \"%s\" found.\n", InitUnsols[cnt]);
        
        for( ptr++; *ptr=='\r' || *ptr=='\n'; ptr++);
        
        ATBufCur = ptr;
        ret = 5;
        goto __EXIT_OF_READER_EXT__;
      }

      if(0 == strncmp(line_head, "ERROR", sizeof("ERROR")-1)){
        LOG("Message \"ERROR\" found.\n");
        ret = 12;
        goto __EXIT_OF_READER_EXT__;
      }

      LOG("Unexpected response found. Message: %s.\n", line_head);
      ret = 6;
      goto __EXIT_OF_READER_EXT__;
    }else if(*line_head=='+')
    {
      char *ptr = NULL;

      ptr = find_NextEOL(line_head);
      if(ptr != NULL)  *ptr = '\0';
    //NOTE: NULL char is regarded as valid.

      if(0 == strcmp(line_head, "+STIN: 21")
          || 0 == strcmp(line_head, "+STIN: 25"))
      {
        LOG("Message \"%s\" found.\n", line_head);
        // Skip the following '\r' and '\n'
        for( ptr++; *ptr=='\r' || *ptr=='\n'; ptr++);
        ATBufCur = ptr;
        ret = 2;
        goto __EXIT_OF_READER_EXT__;
      }

      if(0 == strncmp(line_head, NetClosedUnsol, sizeof(NetClosedUnsol)-1)){
        print_cur_gm_time("   MDMEmodem PDP Deactivated   ");
        LOG("Message \"+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY\" found.\n");
        for( ptr++; *ptr=='\r' || *ptr=='\n'; ptr++);
        ATBufCur = ptr;
        ret = 3;
        goto __EXIT_OF_READER_EXT__;
      }

      if(0 == strncmp(line_head, "+IPCLOSE: 0,1", sizeof("+IPCLOSE: 0,1")-1)){
        print_cur_gm_time("   Server Disconnected   ");
        LOG("Message \"+IPCLOSE: 0,1\" found.\n");
#ifdef FEATURE_ENABLE_REBOOT_DEBUG
	recordCloseCount1++;
       LOG("ERROR: +IPCLOSE: 0,1 reboot when > 10 times recordCloseCount1 = %d \n", recordCloseCount1);
	LOG("Debug: g_is7100modemstartfail = %d.\n", g_is7100modemstartfail);

	if ((recordCloseCount1 >= 10) || (g_is7100modemstartfail == 1)){
		recordCloseCount1 = 0;
		LOG("ERROR: +IPCLOSE: 0,1 reboot.\n");
	      //system("reboot");
		  yy_popen_call("reboot -f");
	}
	
 #endif
	 g_isEmodemipclose_report = true;
	 LOG("Debug: g_isEmodemipclose_report = %d\n",g_isEmodemipclose_report);
        ret = 8;
        goto __EXIT_OF_READER_EXT__;
      }

      if(0 == strncmp(line_head, "+IPCLOSE: 0,2", sizeof("+IPCLOSE: 0,2")-1)){
        print_cur_gm_time("   Server Disconnected   ");
        LOG("Message \"+IPCLOSE: 0,2\" found.\n");
        //				for( ptr++; *ptr=='\r' || *ptr=='\n'; ptr++);
        //				ATBufCur = ptr;
#ifdef FEATURE_ENABLE_REBOOT_DEBUG
	recordCloseCount2++;
       LOG("ERROR: +IPCLOSE: 0,2 reboot when > 10 times recordCloseCount2 = %d \n", recordCloseCount2);
	LOG("Debug: g_is7100modemstartfail = %d.\n", g_is7100modemstartfail);

	if ((recordCloseCount2 >= 10) || (g_is7100modemstartfail == 1)){
		recordCloseCount2 = 0;
		LOG("ERROR: +IPCLOSE: 0,2 reboot.\n");
	      //system("reboot");
	      yy_popen_call("reboot -f");
	}
	
 #endif
        g_isEmodemipclose_report = true;
        LOG("Debug: g_isEmodemipclose_report = %d\n",g_isEmodemipclose_report);
        ret = 9; // Now, re-connect to server.
        goto __EXIT_OF_READER_EXT__;
      }

      if(0 == strncmp(line_head, "+CIPERROR: 4", sizeof("+CIPERROR: 4")-1)){
        LOG("Message \"+CIPERROR: 4\" found.\n");
        ret = 10;
        goto __EXIT_OF_READER_EXT__;
      }

      if(0 == strncmp(line_head, "+CIPERROR: 2", sizeof("+CIPERROR: 2")-1)){
        LOG("Message \"+CIPERROR: 2\" found.\n");
        ret = 11;
        goto __EXIT_OF_READER_EXT__;
      }

      LOG("Unexpected response found. Message: \"%s\".\n", line_head);
      ret = 6; // Don't care unexpected msg.
      goto __EXIT_OF_READER_EXT__;
    }else if(*line_head == '>')
    {
      LOG("\'>\' received.\n");
      ret = 1;
      goto __EXIT_OF_READER_EXT__;
    }
  }else //count <= 0
  {
    LOG("read() error. errno=%s(%d).\n", strerror(errno), errno);
    ret = 7;
    goto __EXIT_OF_READER_EXT__;
  } // if(count <= 0)

__EXIT_OF_READER_EXT__:
  return ret;
}

//Parameters:
//	*recv_len: The memory, which it pointed, should be 0x00 when *recv_len transmitted into function.
//Return Values:
//  -10: "+IPCLOSE: 0,2" received.
//  -9: "+IPCLOSE: 0,1" received.
//  -8: Receive partial message of line 2 from serial file descriptor.
//  -7: Receive partial message of line 1 from serial file descriptor.
//  -6: "+CIPCLOSE: 0,1" received.
//  -5: Unexpected message from unexpected source
//  -4: "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" received
//  -3: "+STIN: 21" received
//  -2: unexpected response received
//  -1: irredeemable error
//  >0: length of already received data
//Note:
//	URC_header_len need to be 0 when the function is being called at first time.
static int reader_recv_start(int fd, int *recv_len, int *total_len_of_this_time, int retry_times)
{
	int read_bytes;
	char *cur = NULL;
	char *p_plus = NULL, *p_1st_colon = NULL, *p_2nd_colon = NULL, *p_NL = NULL, *p_last_NL = NULL;
	int retry = retry_times;
	int len_bytes = 0;
	int read_bytes2 = 0;
	fd_set rfd;
	int ret = -1;

	FD_ZERO(&rfd);
       FD_SET(fd, &rfd);


	LOG6("~~~~ reader_recv_start ~~~~.\n");
	
	if(fd < 0){
		LOG("ERROR: Wrong param input. fd is unavailable.\n");
		return -1;
	}
		
    do{
		read_bytes = read(fd, ATBuf, DEFAULT_BYTES_RECVED_FOR_ONCE);
	}while((read_bytes<0 && errno==EINTR) || read_bytes==0);

	LOG("Debug: read_bytes = %d.\n", read_bytes);
	LOG("Debug: retry = %d.\n", retry);
	LOG("Debug: flow_info.flow_state  2 =FLOW_STATE_ID_AUTH =  %d.\n", flow_info.flow_state);


// add by jackli fix receive "IPCLOSE"
	if (read_bytes > 0){
		
		if (retry == 0){
			
		if(strncmp(&ATBufCur[2], "+IPCLOSE: 0,1", sizeof("+IPCLOSE: 0,1")-1) == 0 ){

			print_cur_gm_time("   Server Disconnected   ");
			//Server connection closed passively
			LOG("\"+IPCLOSE: 0,1\" found.\n");
			return -9;
		}

		else if(strncmp(&ATBufCur[2], "+IPCLOSE: 0,2", sizeof("+IPCLOSE: 0,2")-1) == 0 ){
				print_cur_gm_time("   Server Disconnected  ");
				//Data transmission timeout
				LOG("\"+IPCLOSE: 0,2\" found.\n");
				return -10;
		}else if(strncmp(&ATBufCur[2], NetClosedUnsol, sizeof(NetClosedUnsol)-1) == 0){
				print_cur_gm_time("   MDMEmodem PDP Deactivated   ");
				LOG("\"+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY\" found.\n");
				return -4;
		}else if(strncmp(&ATBufCur[2], "+STIN: 21", sizeof("+STIN: 21")-1) == 0){
				timeoutCount++;
				LOG("\"+STIN: 21\" found %d time(s).\n", timeoutCount);
				return -3;
		}
		
	}
		
}
// add by jackli fix receive "IPCLOSE"
#ifdef FEATURE_ENABLE_AT_UART_RECEIVE_DATA
        msq_send_qlinkdatanode_run_stat_ind(27);
#endif 

	if ((flow_info.flow_state == FLOW_STATE_ID_AUTH) || (flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ)){
	
		if ((retry == 0) && (read_bytes < 41)){

		__READ_MORE_APDU_DATA_IN_PROCESS_RECVDE__:
					ret = select(fd+1, &rfd, NULL, NULL, NULL);
					if(ret != 1){
						LOG("ERROR: select() error. errno=%d.\n", errno);
						goto __READ_MORE_APDU_DATA_IN_PROCESS_RECVDE__;
					}

				 do{
					read_bytes2 = read(fd, ATBuf+read_bytes, DEFAULT_BYTES_RECVED_FOR_ONCE-read_bytes);
				 }while((read_bytes2<0 && errno==EINTR) || read_bytes2==0);

				
				 read_bytes += read_bytes2;

				 LOG("Debug: read_bytes = %d.\n", read_bytes);
				 LOG("Debug: read_bytes2 = %d.\n", read_bytes2);
			}

      }
	
#ifdef FEATURE_ENABLE_AT_UART_RECEIVE_DATA
        msq_send_qlinkdatanode_run_stat_ind(28);
#endif 

	RECV_LOG_PRINT_ATBUFCUR(1, read_bytes);

	if(read_bytes > 0){
		if(retry == 1)
			LOG("retry = %d.\n", retry);
		
		for( ; (*ATBufCur=='\r') || (*ATBufCur=='\n'); ATBufCur++);
		
		if(retry == 0){
			if(strncmp(ATBufCur, "+STIN: 21", sizeof("+STIN: 21")-1) == 0){
				timeoutCount++;
				LOG("\"+STIN: 21\" found %d time(s).\n", timeoutCount);
				return -3;
			}else if(strncmp(ATBufCur, NetClosedUnsol, sizeof(NetClosedUnsol)-1) == 0){
				print_cur_gm_time("   MDMEmodem PDP Deactivated   ");
				LOG("\"+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY\" found.\n");
				return -4;
			}else if(strncmp(ATBufCur, PeerClosedUnsol, sizeof(PeerClosedUnsol)-1) == 0){
				print_cur_gm_time("   Server Disconnected   ");
				LOG("\"+CIPCLOSE: 0,1\" found.\n");
				return -6;
			}else if(strncmp(ATBufCur, "+IPCLOSE: 0,1", sizeof("+IPCLOSE: 0,1")-1) == 0 ){
				print_cur_gm_time("   Server Disconnected   ");
				//Server connection closed passively
				LOG("\"+IPCLOSE: 0,1\" found.\n");
				return -9;
			}else if(strncmp(ATBufCur, "+IPCLOSE: 0,2", sizeof("+IPCLOSE: 0,2")-1) == 0 ){
				print_cur_gm_time("   Server Disconnected  ");
				//Data transmission timeout
				LOG("\"+IPCLOSE: 0,2\" found.\n");
				return -10;
			}else if(strncmp(ATBufCur, "RECV FROM", 9) != 0){
				int cnt = 0;
			
				for(; cnt < 7; cnt++)
					if(strncmp(ATBufCur, InitUnsols[cnt], InitUnsolLen[cnt]-1) == 0)
						break;
				if(cnt < 7){
					LOG("\"%s\" found.\n", InitUnsols[cnt]);
					//Also return -2
				}else{
					print_cur_gm_time("   Unexpected Msg   ");
					LOG("ERROR: Unexpected response found. Message: %s\n", ATBuf);
				}
				return -2;
			}

			print_cur_gm_time("   server->App   ");
			p_1st_colon = strchr(ATBufCur, ':');
			p_2nd_colon = strchr(p_1st_colon+1, ':');
			p_NL = strchr(p_2nd_colon+1, '\n');
			cur = p_1st_colon+1; // Move cur to first byte of IP address.

			if(strncmp(cur, at_config.net_config.server_IP, p_2nd_colon-p_1st_colon-1) != 0){
				LOG("NOTICE: Incoming messages came from unverified source!\n");
				return -5;
			}
			
			//FIXME: Check port?
//			LOG("Incoming message came from verified source.\n");
		}

		if(retry == 0)
			p_plus = p_NL+1;
		else if(retry == 1)
			p_plus = ATBufCur;
		else // retry == 2
			goto __COPY_VALID_DATA_TO_LINE_HEAD__;
		if(*p_plus != '+' && retry == 0){
			memcpy(recv_tmp_buf, ATBuf, read_bytes);
			URC_header_len += read_bytes;
			*recv_len = read_bytes;
			LOG("Partial msg (line 1) recved! Until now, recved bytes(URC_header_len)=%d.\n", URC_header_len);
			return -7;
		}else if(*p_plus != '+' && retry == 1){
			LOG("ERROR: 2nd line of URC is not started with \'+\'! Recved msg: %s.\n",
					ATBufCur);
			return -1;
		}
		
		for( cur=p_plus+4; *cur!='\r' && *cur!='\n'; cur++, len_bytes++){
			*total_len_of_this_time *= 10;
			*total_len_of_this_time += *cur-'0';
		}
		p_last_NL = cur+1;
		if(*p_last_NL != '\n'){
			LOG("ERROR: \'\\n\' not found in the end of URC!\n");
			return -1;
		}
		URC_header_len += p_last_NL-ATBuf+1;
		LOG("URC_header_len=%d.\n", URC_header_len);
		
		
		if(p_last_NL-ATBuf+1 == read_bytes){
			memcpy(recv_tmp_buf+last_recv_len, ATBuf, read_bytes);
			*recv_len = read_bytes;
			LOG("Partial msg (line 2) recved! Until now, recved bytes(URC_header_len)=%d.\n", URC_header_len);
			return -8;
		}else if(p_last_NL-ATBuf+1 > read_bytes){
			LOG("ERROR: Unexpected error happened!More data parsed than the amount of read data.\n");
			return -1;
		}

		
		//p_last_NL-ATBuf+1 < read_bytes
		__COPY_VALID_DATA_TO_LINE_HEAD__:
		memcpy(recv_tmp_buf+last_recv_len, ATBuf, read_bytes);
		*recv_len = read_bytes;

		at_config.net_config.header_pre_len = URC_header_len-(len_bytes+2);
		if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ){
			URC_header_len_of_remote_UIM_data = URC_header_len;
		}
		memset(URC_data, 0, sizeof(URC_data));
		memcpy(URC_data, recv_tmp_buf, URC_header_len);
		URC_header_len = 0;
		
		if(len_bytes+2 != p_last_NL-p_plus-3){
			LOG("ERROR: len_bytes=%d, p_last_NL-p_plus-3=%d.\n", len_bytes, p_last_NL-p_plus-3);
			return -1;
		}
		LOG("AT rsp header pre len (TCP URC rpt) =%d.\n", at_config.net_config.header_pre_len);
		return *recv_len;
	}else{
		LOG("ERROR: read() failed. errno=%d.\n", errno);
		return -1;
	}
	
}

//Parameter:
//	offset and var are all positive value.
#define FILL_NEW_DATA_INTO_BUF(offset, var) \
{ \
	p_buffer->data = realloc(p_buffer->data, buf_len+read_bytes-(var)); \
	memset(p_buffer->data+buf_len, 0, read_bytes-(var)); \
	memcpy(p_buffer->data+buf_len, (void *)(ATBuf+(offset)), read_bytes-(var)); \
	p_buffer->size += read_bytes-(var); \
}

#ifndef FEATURE_DEBUG_QXDM
	#ifndef FEATURE_DEBUG_LOG_FILE
		#define PRINT_WHAT_READ_GET \
		{ \
			int i = 0; \
			LOG("read() recved: "); \
			for(; i<read_bytes; i++){ \
				LOG2("%02x ", ATBuf[i]); \
			} \
			LOG2("\n"); \
		}
	#else
		#define PRINT_WHAT_READ_GET \
		{ \
			int i = 1; \
			pthread_mutex_lock(&log_file_mtx); \
			LOG2("read() recved: \n"); \
			for(; i<=read_bytes; i++){ \
				if(i%20 == 1) \
					LOG4(" %02x", ATBuf[i-1]); \
				else \
					LOG3(" %02x", ATBuf[i-1]); \
				if(i%20 == 0) \
					LOG3("\n"); \
			} \
			if((i-1)%20 != 0) \
				LOG3("\n"); \
			pthread_mutex_unlock(&log_file_mtx); \
		}
	#endif /*FEATURE_DEBUG_LOG_FILE*/
#else
	#ifndef FEATURE_ENABLE_FMT_PRINT_DATA_LOG
		#define PRINT_WHAT_READ_GET \
		{ \
			int data_len = read_bytes; \
			int buf_len = 3*data_len+1; \
			char buf[buf_len] = {0}; \
			for(; i < data_len; i++){ \
				sprintf(buf+i*3, "%02x ", ATBuf[i]); \
			} \
			LOG("read() recved: "); \
			LOG2("  %s", buf); \
		}
	#else
		#define PRINT_WHAT_READ_GET \
		{ \
			int data_len = read_bytes; \
			int buf_len = 3*data_len+1; \
			char buf[buf_len] = {0}; \
			for(; i < data_len; i++){ \
				sprintf(buf+i*3, "%02x ", ATBuf[i]); \
			} \
			LOG("read() recved: "); \
			print_fmt_data(buf); \
		}
	#endif /*FEATURE_ENABLE_FMT_PRINT_DATA_LOG*/
#endif /*FEATURE_DEBUG_QXDM*/

//Received data will be ended with a '\0' rather than a '\r' or a '\n'.
//return value:
//	0: read() error or select() error or other unexpected error
//	1: success

#define VALID_DATA_LEN_OF_EACH_TCP_URC (1394)

//Function:
//	Return the digits of input val.
//Note:
//	Input parameter must be unsigned integer.
int get_digits_of_uint_val(const int val)
{
	int digit_num = 0;
	int div = 10;

	for(; val/div!=0; digit_num++, div*=10);
	digit_num ++;

	return digit_num;
}

//Param:
//recv_len_param : Total recved TCP URC len.
//data_len_param : TCP URC data len (of this first recved URC). It is marked after str "+IPD".
//Return Values:
// 1: Success.
// 0: Wrong data recved. (e.x. size error, content error, etc.)
//  -1: Internal error. (Not acceptable)
static int process_RECV(int fd, int recv_len_param, int data_len_param)
{
  rsmp_recv_buffer_s_type *p_buffer = NULL;
  int ret;
  fd_set rfd;
  bool isFirstURCOfUIMData = true;

  LOG6("~~~~ process_RECV ~~~~\n");

  if(fd < 0 || recv_len_param <= 0 || data_len_param <= 0){
    LOG("ERROR: Wrong param input. fd=%d, recv_len_param=%d, data_len_param=%d.\n", 
                  fd, recv_len_param, data_len_param);
    return -1;
  }

  FD_ZERO(&rfd);
  FD_SET(fd, &rfd);
  
  reset_rsmp_recv_buffer();

  p_buffer = &g_rsmp_recv_buffer;

  LOG("Until now, total recved len=%d, valid data len of this URC=%d.) \n", recv_len_param, data_len_param);
  p_buffer->data = malloc(recv_len_param);
  memset(p_buffer->data, 0, recv_len_param);
  memcpy(p_buffer->data, recv_tmp_buf, recv_len_param);
  p_buffer->size = recv_len_param;
  memset(recv_tmp_buf, 0, sizeof(recv_tmp_buf));

  if(flow_info.flow_state != FLOW_STATE_REMOTE_UIM_DATA_REQ)
  {
		int URC_header_len = at_config.net_config.header_pre_len+get_digits_of_uint_val(data_len_param)+2, 
			recv_len = recv_len_param, 
			total_len = data_len_param+URC_header_len;
		//recv_len : Actually recved len.
		//total_len : Expectedly recvd len.
		
		if(recv_len_param < data_len_param+URC_header_len){
			int read_bytes = -1;
			
			p_buffer->size = total_len;
			p_buffer->data = realloc(p_buffer->data, p_buffer->size);
			memset((unsigned char *)p_buffer->data+recv_len_param, 
							0, 
							p_buffer->size-recv_len_param
						 );
			
__READ_MORE_APDU_DATA_IN_PROCESS_RECV__:
			ret = select(fd+1, &rfd, NULL, NULL, NULL);
			if(ret != 1){
				LOG("ERROR: select() error. errno=%d.\n", errno);
				goto __READ_MORE_APDU_DATA_IN_PROCESS_RECV__;
			}
			read_bytes = read(fd, 
											(unsigned char *)p_buffer->data+recv_len, 
											total_len-recv_len
										 );
			if(read_bytes <= 0){
				if(read_bytes<0 && errno!=EINTR && errno!=EAGAIN){
					LOG("ERROR: read() failed. errno=%d.\n", errno);
					return -1;
				}
				
				usleep(1000);
				goto __READ_MORE_APDU_DATA_IN_PROCESS_RECV__;
			}else{
				if(read_bytes+recv_len < total_len){

					LOG("ERROR: Get data len is vaild,send again!\n");
					
					recv_len += read_bytes;
					goto __READ_MORE_APDU_DATA_IN_PROCESS_RECV__;

					
				}else if(read_bytes+recv_len > total_len){
					LOG("ERROR: Oversize data recved!\n");
					return 0;
				}
			}
		}
		
		//Check if right URC header recved in buffer
		ret = memcmp(p_buffer->data, URC_data, URC_header_len);
		if(ret != 0){
			LOG("ERROR: TCP URC header not found in buffer!\n");
			return 0;
		}
		
		//Remove useless TCP URC header
		memmove(p_buffer->data, 
							(unsigned char *)p_buffer->data+URC_header_len, 
							p_buffer->size-URC_header_len
						 );
		p_buffer->size -= URC_header_len; // Update p_buffer->size
		memset((unsigned char *)p_buffer->data+p_buffer->size, 0, URC_header_len); // Not necessary
		
  //Print recved data without TCP URC header
	#if 1
		#ifndef FEATURE_DEBUG_QXDM
			#ifndef FEATURE_DEBUG_LOG_FILE
				int i;
				LOG("Msg without TCP URC header: (p_buffer->size=%d) ",
						p_buffer->size);
				for( i=0; i<p_buffer->size; i++){
					LOG2("%02x ", ((unsigned char *)p_buffer->data)[i]);
				}
				LOG2("\n");
			#else
				int i = 1;
				pthread_mutex_lock(&log_file_mtx);
				LOG2("Msg without TCP URC header: (p_buffer->size=%d) \n",
							p_buffer->size);
				for(; i<=p_buffer->size; i++){
					if(i%20 == 1)
						LOG4(" %02x", ((unsigned char *)p_buffer->data)[i-1]);
					else
						LOG3(" %02x", ((unsigned char *)p_buffer->data)[i-1]);
					
					if(i%20 == 0) //Default val of i is 1
						LOG3("\n");
				}
				if((i-1)%20 != 0) // Avoid print redundant line break
					LOG3("\n");
				pthread_mutex_unlock(&log_file_mtx);
			#endif /*FEATURE_DEBUG_LOG_FILE*/
		#else
			int i = 0;
			int data_len = p_buffer->size;
			int buf_len = 3*data_len+1;
			char buf[buf_len];
				
			memset(buf, 0, buf_len);
			for(; i < data_len; i++){
				sprintf(buf+i*3, "%02x ", ((unsigned char *)p_buffer->data)[i]);
			}
			LOG("Msg without TCP URC header: (p_buffer->size=%d) ", p_buffer->size);
			#ifdef FEATURE_ENABLE_FMT_PRINT_DATA_LOG
				print_fmt_data(buf);
			#else
				LOG2("  %s", buf);
			#endif
		#endif
	#endif
	
	}else{
		char *msg_len_str = NULL;
		int read_bytes = -1, 
			recv_len = recv_len_param, 
			total_len = 0,
			parsed_len = 0, /*Len of raw data which has been parsed.*/
			valid_len = 0; /*Len of valid data which is parsed out of raw data*/
		char *cur = NULL;
		
		//Not recv all bytes of len field of priv proto msg
		if(isFirstURCOfUIMData == true && 
			recv_len_param-URC_header_len_of_remote_UIM_data < FIELD_LEN_OF_MSG_HEAD_AND_MSG_LEN
		){
			__PROCESS_RECV_SELECT_AGAIN__:
			ret = select(fd+1, &rfd, NULL, NULL, NULL);
			if(ret <= 0){
				if(errno == EINTR || errno == EAGAIN)
					goto __PROCESS_RECV_SELECT_AGAIN__;
				
            	LOG("ERROR: select() returns %d. errno=%d.\n", ret, errno);
				return -1;
        	}else{
				char rsp_buf[48] = {0};
				
				__READ_MSG_LEN_IN_PROCESS_RECV__:
				read_bytes = read(fd, rsp_buf, 48);
				if(read_bytes <= 0){
					if(read_bytes<0 && errno!=EINTR && errno!=EAGAIN){
						LOG("ERROR: read() failed. errno=%d.\n", errno);
						return -1;
					}
					
					usleep(1000);
					goto __READ_MSG_LEN_IN_PROCESS_RECV__;
				}
				p_buffer->data = realloc(p_buffer->data, p_buffer->size+read_bytes);
				memset((unsigned char *)p_buffer->data+p_buffer->size, 0, read_bytes);
				memcpy((unsigned char *)p_buffer->data+p_buffer->size, rsp_buf, read_bytes);
				p_buffer->size += read_bytes;
				recv_len += read_bytes;
			}
		}
		
		//Acq priv proto msg len
		msg_len_str = (char *)p_buffer->data + URC_header_len_of_remote_UIM_data + FIELD_LEN_OF_MSG_HEAD;
		LOG("The len field of priv proto msg: [No.0]=%02x, [No.1]=%02x, [No.2]=%02x, [No.3]=%02x.\n",
				msg_len_str[0], msg_len_str[1], msg_len_str[2], msg_len_str[3]);
		total_len = (msg_len_str[0]<<24) + (msg_len_str[1]<<16) + (msg_len_str[2]<<8) + msg_len_str[3];
		LOG("Without TCP URC headers, the priv proto msg len (not count msg head and len field)=%d. \n", total_len);
//		LOG("And %d bytes of proto header data not count in total_len.\n", FIELD_LEN_OF_MSG_HEAD_AND_MSG_LEN);
		isFirstURCOfUIMData = false;
		
		//Tips:
		//		We can't assume remote USIM data pack will be seperated into each URC with valid data length of
		//	1394 bytes.
		
		//Create a temp buffer.
		//Description for code below:
		//		Temporary buffer for remote USIM data and it is insufficient for not counting len of all TCP URC headers. But this 
		//	operation is for reducing the calls of realloc().
		if(total_len > p_buffer->size){ // Fixed by rjf at 20150915
			p_buffer->data = realloc(p_buffer->data, total_len);
			memset((unsigned char *)p_buffer->data+p_buffer->size, 0, total_len-p_buffer->size);
			//No update for p_buffer->size.
		}
		
		//Recv remaining unrecved data
		while(1){
			char tmp_buf[DEFAULT_BYTES_RECVED_FOR_ONCE];
			struct timeval tv;

			memset(tmp_buf, 0, DEFAULT_BYTES_RECVED_FOR_ONCE);
			tv.tv_sec = 5; // Changed by rjf at 20151102 (It may consume more than 1s to recv subsequent data blocks.)
			tv.tv_usec = 0;
			
			ret = select(fd+1, &rfd, NULL, NULL, &tv);
			if(ret == 1){
				int saved_len = recv_len;
				
				read_bytes = read(fd, tmp_buf, DEFAULT_BYTES_RECVED_FOR_ONCE);
				if(read_bytes == 0 ||
					(read_bytes < 0 && (errno == EINTR || errno == EAGAIN))
				){
					continue;
				}else if(read_bytes < 0){
					LOG("ERROR: read() failed. saved_len=%d, errno=%d.\n", saved_len, errno);
					return -1;
				}
				
				recv_len += read_bytes;
				if(recv_len > total_len){
					p_buffer->data = realloc(p_buffer->data, recv_len);
					memset((unsigned char *)p_buffer->data+saved_len, 0, read_bytes);
				}
				memcpy((unsigned char *)p_buffer->data+saved_len, tmp_buf, read_bytes);
			}else if(ret == 0){
				LOG("Recving process ends. recv_len=%d (Total recved len incl URC headers).\n", recv_len);
				break;
			}
		}

    //Print raw data of buffer
#ifdef FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE
		if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ){
			int i = 0;
			
			fstream = fopen(RECVED_DATA_LOG_FILE_PATH, "a+");
			if(fstream == NULL){
				LOG("ERROR: fopen(\"%s\") failed. errno=%d.\n", RECVED_DATA_LOG_FILE_PATH, errno);
				goto __PROCESS_RECV_PARSE_RECVED_DATA__;
	  	}
			truncate(RECVED_DATA_LOG_FILE_PATH, 0);
			
			for(; i<recv_len; i++){
				fprintf(fstream, "%02x%c", ((unsigned char *)p_buffer->data)[i], 0x20);
				if( i%30 == 0 && i != 0 )
					fprintf(fstream, "\n");
			}
			fclose(fstream);
		}
#endif // FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE

    //Parse recved raw data to rm TCP URC headers
#ifdef FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE
__PROCESS_RECV_PARSE_RECVED_DATA__:
#endif
		//cur: the start pos for parsing raw data each time
		cur = p_buffer->data;
		while(parsed_len < recv_len){
			char *p_tgt = NULL; // Ptr to URC header
			int rm_len = 0;
			
			p_tgt = find_sub_content(cur, URC_data, recv_len-parsed_len, URC_header_len_of_remote_UIM_data);
			if(p_tgt == NULL){
				//Unexpected URC header found (This URC header is diff from the one stored in URC_data)
				
				//Search for header pre
				p_tgt = find_sub_content(cur, URC_data, recv_len-parsed_len, at_config.net_config.header_pre_len);
				if(p_tgt != NULL){
					char *URC_len_str = NULL;
					int cnt = 0, URC_header_len_of_this_time = 0;
					
					URC_len_str = p_tgt+at_config.net_config.header_pre_len;
					for( ; URC_len_str[cnt]!='\r'&&URC_len_str[cnt]!='\n'; cnt++){
						LOG("+++++ In this unexp URC header, URC_len_str[%d]: %c. \n", cnt, URC_len_str[cnt]);
					}
					URC_header_len_of_this_time = at_config.net_config.header_pre_len+cnt+2;
					LOG("+++++ In this unexp URC header, URC_header_len_of_this_time: %d \n", URC_header_len_of_this_time);
					
					parsed_len += p_tgt-cur+URC_header_len_of_this_time;
					memcpy(p_tgt, p_tgt+URC_header_len_of_this_time, recv_len-parsed_len);
					rm_len += URC_header_len_of_this_time;
					memset(cur+recv_len-rm_len, 0, rm_len); //Clear invalid mem
					valid_len += p_tgt-cur; //Now, *p_tgt is invalid byte.
					continue;
				}else{
					LOG("No more TCP URC headers found in buffer!\n");
				}

				//No URC header found
				valid_len += recv_len-parsed_len;
				parsed_len = recv_len;
				LOG("Parsing process ends. valid_len=%d, parsed_len=%d.\n", valid_len, parsed_len);
				break;
			}
			
			//Find same URC header as the one in URC_data
			parsed_len += p_tgt-cur+URC_header_len_of_remote_UIM_data;
			memcpy(p_tgt, p_tgt+URC_header_len_of_remote_UIM_data, recv_len-parsed_len);
			rm_len += URC_header_len_of_remote_UIM_data;
			memset(cur+recv_len-rm_len, 0, rm_len); //Clear invalid mem
			valid_len += p_tgt-cur; //NOTE: p_tgt is invalid byte.
			cur =  p_tgt;
		}
		p_buffer->size = valid_len;

    //Print abnormal remote USIM data whose data len is not more than 200B.
#ifdef FEATURE_ENABLE_ABNORMAL_REMOTE_USIM_DATA_PRINT_LOG
		if(p_buffer->size <= 200){
			#ifndef FEATURE_DEBUG_QXDM
				#ifndef FEATURE_DEBUG_LOG_FILE
					int i = 0;
					LOG("Abnormal remote USIM data: (p_buffer->size=%d) \n",
							p_buffer->size);
					for(; i<p_buffer->size; i++)
						LOG2("%02x ", get_byte_of_void_mem(p_buffer->data, i) );
					LOG2("\n");
				#else
					int i = 1;
					
					pthread_mutex_lock(&log_file_mtx);
					LOG2("Abnormal remote USIM data: (p_buffer->size=%d) \n",
							  p_buffer->size);
					for(; i<=p_buffer->size; i++){
						if(i%20 == 1)
							LOG4(" %02x", get_byte_of_void_mem(p_buffer->data, i-1) );
						else
							LOG3(" %02x", get_byte_of_void_mem(p_buffer->data, i-1) );
						
						if(i%20 == 0) //Default val of i is 1
							LOG3("\n");
					}
					if((i-1)%20 != 0) // Avoid print redundant line break
						LOG3("\n");
					pthread_mutex_unlock(&log_file_mtx);
				#endif /*FEATURE_DEBUG_LOG_FILE*/
			#else
				int i = 0;
				int data_len =p_buffer->size;
				int buf_len = 3*data_len+1;
				char buf[buf_len];
				
				memset(buf, 0, buf_len);
				for(; i<data_len; i++){
					sprintf(buf+i*3, "%02x ", get_byte_of_void_mem(p_buffer->data, i) );
				}
				LOG("Abnormal remote USIM data: \n");
				#ifdef FEATURE_ENABLE_FMT_PRINT_DATA_LOG
					print_fmt_data(buf);
				#else
					LOG2("  %s", buf);
				#endif
			#endif /*FEATURE_DEBUG_QXDM*/
		}
#endif

    //Print recved remote USIM data pack to /usr/bin/remote_usim_data_log
#ifdef FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE_AT_ONCE

#if  0
		if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ){
			FILE *fstream2;
			int i = 1;
			struct tm *tm;
			time_t now;
			char time_mark_buf[32] = {0};
			
			fstream2 = fopen(REMOTE_USIM_DATA_LOG_FILE_PATH, "w+");
			if(fstream2 == NULL){
				LOG("ERROR: fopen(\"%s\") failed. errno=%d.\n", REMOTE_USIM_DATA_LOG_FILE_PATH, errno);
				goto __EXIT_OF_PROCESS_RECV__;
		  }
			
			time(&now);
			tm = localtime(&now);
		  sprintf(time_mark_buf, "%02d/%02d %02d:%02d:%02d ", 
						tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
			fprintf(fstream2, "***** Create Time: %s *****\n", time_mark_buf);
			
			for(; i<=p_buffer->size; i++){
				fprintf(fstream2, "%02x%c", ((unsigned char *)p_buffer->data)[i-1], 0x20);
				if( i%30 == 0 && i != 0/*i!=0 is for that default i is 0*/ )
					fprintf(fstream2, "\n");
			} // for
			
			fclose(fstream2);
		}

#endif
	
#endif // FEATURE_ENABLE_PROCESS_RECV_WRITE_REMOTE_UIM_DATA_TO_LOG_FILE_AT_ONCE
		
		if(valid_len != total_len+5){ // total_len not count in protocol header, but valid_len does.
			LOG("ERROR: valid_len (%d) isn't equal to total_len+5 (%d).\n", valid_len, total_len+5);
			return 0;
		}
	}
	
__EXIT_OF_PROCESS_RECV__:
  LOG("All data(sent from server) received!\n");

#ifndef FEATURE_NEW_APDU_COMM
  APDU_setting.proc_step = 0;
#endif // !FEATURE_NEW_APDU_COMM

  return 1;
}

//Function:
//	Read the ouput "OK"
//Return Values:
//	2: Unexpected response received.
//	1: "OK" received.
//	0: Unexpected error.
static int get_OK(void)
{
	int ret;
	
	if(ATBufCur == NULL){
		LOG("ERROR: ATBufCur = null!\n");
		return 0;
	}
	
	while(*ATBufCur == '\r' || *ATBufCur == '\n') ATBufCur++;
	line_head = ATBufCur;
	ATBufCur = find_NextEOL(ATBufCur);
	if(ATBufCur != NULL){
		*ATBufCur = '\0';
	}else{
		LOG("Not found \\r or \\n before char EOF('\\0'). Recved data is a fragment.\n");
		
		if(NULL == strstr(line_head, "OK")){
			return 2;
		}
	}

	ret = strcmp(line_head, "OK");
	if(ret != 0){
		LOG("Unexpected response found. Message: %s.\n", line_head);
		return 2;
	}
	
	LOG("     <-- \"%s\"  \n", line_head);
	
	return 1;
}

//return value:
// 0: error
// 1: success
static int write_line(const int fd, const void *data, const unsigned int len_param)
{
	int count;
	int written_bytes = 0;
	int len = len_param;

//	LOG6("~~~~ write_line start ~~~~\n");

	if( fd<0 || data==NULL || len == 0){
		LOG("ERROR: Wrong param(s) input. fd=%d, len=%u.\n", fd, len_param);
		return 0;
	}
	
	while(written_bytes < len){
//		LOG("+++++++++++++++ len: %d, written_bytes: %d\n", len, written_bytes);
//		LOG("+++++++++++++++ cmd: %s\n", (unsigned char *)data+written_bytes);
		count = write(fd, 
							  (unsigned char *)data+written_bytes,  // Fixed by rjf at 20150913
							  len-written_bytes);
		if(count > 0){
			written_bytes += count;
			if(written_bytes < len){
				LOG("NOTICE: write() returns %d. len=%d. Continue to write...\n", count, len);
			}
		}else if(count < 0 && errno != EINTR && errno != EAGAIN){
			LOG("ERROR: write() failed. errno=%d.\n", errno);
			return 0;
		}else{
			LOG("write() returns %d. Try again.\n", count);
			sleep(1);
		}
	}
	
	//Fixed by rjf at 20150914
	//	Even if AT+CIPSEND, \r and \n will be added at the end of pure data. But it is fine to do this.
	count = -1;
	do{
		if(count == 0)
			LOG("ERROR: write() returns %d (\"\\r\\n\"). Try again.\n", count);
		count = write(fd, "\r\n" , 2);
	}while((count < 0 && (errno == EINTR || errno == EAGAIN)) || (count == 0));
	if(count < 0){
		LOG("ERROR: write() failed (\"\\r\\n\"). errno=%d.\n", errno);
		return 0;
	}
	//Assume it completes once. Not possible that count>0 but count<2.
	
	return 1;
}

//Function:
//		When send AT cmd by writer(), no need to add \r or \n at end of cmd string. And
//	argument len should be strlen(data).
//Return Values:
//	0: error
//	1: succeed
static int writer(const int fd, const void *data, const unsigned int len)
{
  int result;

  if(fd < 0 || data == NULL || len == 0){
    LOG("ERROR: Wrong param(s) input. fd=%d, len=%u.\n", fd, len);
    return 0;
  }

  result = write_line(fd, data, len);
  if(result != 1){
    LOG("ERROR: write_line() error. result=%d.\n", result);
    return 0;
  }

  return 1;
}

#ifdef FEATURE_ENABLE_ATE0_RSP_TIMER
static void ate0_rsp_timeout_cb(void *userdata)
{
  LOG6("~~~~ ate0_rsp_timeout_cb ~~~~\n");
  openDev(2);
}

void add_timer_ate0_rsp_timeout(void)
{
  struct timeval tv;

  memset(&tv, 0, sizeof(tv));
  tv.tv_sec = DEFAULT_ATE0_RSP_TIMEOUT_SEC;
  tv.tv_usec = DEFAULT_ATE0_RSP_TIMEOUT_USEC;
  set_evt(&ate0_rsp_timeout_evt, -1, false, ate0_rsp_timeout_cb, NULL);
  add_timer(&ate0_rsp_timeout_evt, &tv);
  LOG("NOTICE: Add ate0 rsp timeout timer. ate0_rsp_timeout_evt=%x.\n", (unsigned int)(&ate0_rsp_timeout_evt));
  return;
}
#endif

#ifdef FEATURE_CONFIG_SIM5360
#include "qlinkdatanode_atfwd_share.h"

static struct ATCMD5360_TYPE sim5360_config_data;

//Description:
//		Skip the all chars of customed line separator ("\r\n*****\r\n"), and return the next char right
//	after the last char of customed line separator.
//Param:
//	pos: It must point to the first char of customed line separator ("\r\n*****\r\n").
char *skip_customed_line_separator(char *pos){
	char *cur = pos;
	
	cur += 2; // Jump to the '*' of separator
	while('*' == *cur)	cur++;
	cur += 2; // Skip the '\r' and '\n' of separator
	return cur;
}

//Return Values:
//	1 : Redundant rsp "OK" recved.
//	0 : Redundant rsp "OK" not recved.
int check_if_redundant_OK_recved(char *start_pos)
{
	char *p_1st_valid_char = NULL;
	int ret = 0;
	
	p_1st_valid_char = skip_line_break(start_pos);
	if(p_1st_valid_char[0] == 'O' && p_1st_valid_char[1] == 'K'){
		ret = 1;
	}
	
	return ret;
}

void write_atcmd5360_file(void)
{
  int fd = -1;

  if((fd = creat(ATCMD5360_FILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
  {
    LOG("ERROR: open() failed. errno=%d.\n", errno);
    return;
  }

  if(flock(fd, LOCK_EX) == 0)
  {
    if(write(fd, &sim5360_config_data, sizeof(sim5360_config_data)) < 0)
    {
      LOG("ERROR: write() failed. errno=%d.\n", errno);
      return;
    }
  }
  flock(fd, LOCK_UN);

  close(fd);
}

int read_atcmd5360_file(struct ATCMD5360_TYPE *pstAtcmd5360 )
{
  FILE *fp;
  int nFileLen =0, ret, result = 0;//0 :successfull ;-1: failed

  if(access(ATCMD5360_FILE, R_OK) != 0){
    return -1;
  }

  if((fp = fopen(ATCMD5360_FILE, "rb"))==NULL){
    return -1;
  }

  {
    fseek(fp, 0, SEEK_END);
    nFileLen = ftell(fp);

    if(nFileLen == sizeof(struct ATCMD5360_TYPE))
    {
      fseek(fp, 0, SEEK_SET);
      if((ret = fread(pstAtcmd5360, sizeof(struct ATCMD5360_TYPE), 1, fp))!=1)
      {
        result = -1;
      LOG("ERROR: fread() failed. errno=%d.\n", errno);
    }
    }else{
      result = -1;
      LOG("ERROR: nFileLen=%d, but sizeof(struct ATCMD5360_TYPE)=%d.\n",
                  nFileLen, sizeof(struct ATCMD5360_TYPE));
    }
  }

  fclose(fp);
  return result;
}

void YyReadConfig(const char *file_path, const char *item_name, char *item_buff)
{
  char line_buf[256], line_name[40], sign = '=', *p_sign = NULL;
  FILE *fstream = NULL;
  int len;

  fstream = fopen(file_path, "r");
  if(fstream == NULL){
    LOG("ERROR: fopen(%s) failed. errno=%d.\n", file_path, errno);
    return;
  }

  fseek(fstream, 0, SEEK_SET); 
  while(fgets(line_buf, 256, fstream) != NULL)
  {
    int line_len = strlen(line_buf);

    //Skip the empty line (Just 2 line break chars.)
    if(line_len < 3){
      continue;
    }
    
    //Skip the line ended with '\n' and '\r'
    if(line_buf[strlen(line_buf)-1] == 0x0a){
    line_buf[strlen(line_buf)-1] = 0x00;
    }
    if(line_buf[strlen(line_buf)-1] == 0x0d){ //Don't change "strlen(line_buf)"
        line_buf[strlen(line_buf)-1] = 0x00;
    }

    memset(line_name, 0, sizeof(line_name));
    p_sign = strchr(line_buf, sign);
    if(p_sign == NULL){
      continue;
    }else{
      len = p_sign -line_buf;
      strncpy(line_name, line_buf, len);
      if(strcmp(line_name, item_name) == 0){
        strncpy(item_buff, line_buf+(len+1), strlen(line_buf)-len-1);
        break;
      }
      if(fgetc(fstream) == EOF){
        LOG("EOF found!\n");
        break;  
      }
      fseek(fstream, -1, SEEK_CUR);
      memset(line_buf, 0, sizeof(line_buf));
    }
  }
  fclose(fstream);

  return;
}

#define MAX_SET_PDP_CONTEXT_AT_CMD_STR_LEN (512)

void r_checkPdpCtxHdlr(void *userdata);
void r_setPdpCtxHdlr(void *userdata);

static bool isSIM5360PrebuiltAtcmdFileRead = false;

void pdp_ctx_inf_conv_buf_into_struct(void)
{
	char *front = NULL, *end = NULL;
	int idx = 1;
	
	front = pdp_ctx_inf_buf;
	for(; idx<CHECK_PDP_CTX_STATE_MAX; idx++){
		end = strstr(front, "*****");
		end -= 2; // Jump back to the beginning '\r' of separator
		
		switch(idx){
			case CHECK_PDP_CTX_STATE_CGDCONT:{
				memset(sim5360_config_data.cgdcont5360read, 0, sizeof(sim5360_config_data.cgdcont5360read));
				memcpy(sim5360_config_data.cgdcont5360read, front, end-front);
				break;
			}
			case CHECK_PDP_CTX_STATE_CGAUTH:{
				memset(sim5360_config_data.cgauth5360read, 0, sizeof(sim5360_config_data.cgauth5360read));
				memcpy(sim5360_config_data.cgauth5360read, front, end-front);
				break;
			}
			default:
				break;
		} // switch end
		
		if(idx == CHECK_PDP_CTX_STATE_CGAUTH){
			sim5360_config_data.isupdate = 1;
			continue; // Exit
		}
		front = skip_customed_line_separator(end);
	}
	
	return;
}

//Description:
//Write checking result of "AT+CGDCONT?" and "AT+CGAUTH?" into the struct member "cgdcont5360read" and 
//"cgauth5360read" of corresponding struct of config file.
void pdp_ctx_inf_write_cfg_file(void)
{
  pdp_ctx_inf_conv_buf_into_struct();

  write_atcmd5360_file();

  free(pdp_ctx_inf_buf);
  pdp_ctx_inf_buf_offset = 0;
}

void w_checkPdpCtxHdlr(void)
{
  const char *check_ctx_at_cmd[] = {
    "AT",
    "AT+CGDCONT?",
    "AT+CGAUTH?",
    "AT"
  };

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(check_pdp_ctx_state){
  case CHECK_PDP_CTX_STATE_CGDCONT:
  case CHECK_PDP_CTX_STATE_CGAUTH:
  {
    LOG("    --> \"%s\"  \n", check_ctx_at_cmd[(int)check_pdp_ctx_state]);

    writer(at_config.serial_fd, 
            check_ctx_at_cmd[(int)check_pdp_ctx_state], 
            strlen(check_ctx_at_cmd[(int)check_pdp_ctx_state])
          );
    INIT_ATBUF;
    set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkPdpCtxHdlr, NULL);
    add_evt (&at_config.AT_event, 'r');
    break;
  }

  default:
    LOG("ERROR: Wrong check_pdp_ctx_state (%d) found!\n", check_pdp_ctx_state);
    break;
  } // switch end
}

void r_checkPdpCtxHdlr(void *userdata)
{
  char rsp_buf[DEFAULT_PDP_CTX_INF_BUF_SIZE] = {0}, separator[] = "\r\n*****\r\n";
  int read_bytes = -1;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(check_pdp_ctx_state)
  {
  case CHECK_PDP_CTX_STATE_CGDCONT:
  case CHECK_PDP_CTX_STATE_CGAUTH:
  {
    char *p_OK = NULL, check_result = -1;
    
    if(CHECK_PDP_CTX_STATE_CGDCONT == check_pdp_ctx_state)
    {
      pdp_ctx_inf_buf = (char *)malloc(DEFAULT_PDP_CTX_INF_BUF_SIZE);
      memset(pdp_ctx_inf_buf, 0, DEFAULT_PDP_CTX_INF_BUF_SIZE);
      pdp_ctx_inf_buf_offset = 0;
    }

_R_CHECKPDPCTXHDLR_READ_AGAIN__:
    read_bytes = read(at_config.serial_fd, rsp_buf, DEFAULT_PDP_CTX_INF_BUF_SIZE);
    if(read_bytes < 0 || read_bytes == 0)
    {
      if(errno == EAGAIN || errno == EINTR || read_bytes == 0)
      {
        goto _R_CHECKPDPCTXHDLR_READ_AGAIN__;
    }

      LOG("ERROR: read() failed. errno=%d.\n", errno);
      goto _R_CHECKPDPCTXHDLR_READ_AGAIN__;
    }

    //read_bytes > 0
    //It may include "+STIN: 21" or "+STIN: 25" inside rsp_buf.
    if(read_bytes + pdp_ctx_inf_buf_offset > DEFAULT_PDP_CTX_INF_BUF_SIZE)
    {
      LOG("ERROR: Unexpected length of data recved. Truncate the recved data and end the process!\n");
      memcpy(pdp_ctx_inf_buf+pdp_ctx_inf_buf_offset, rsp_buf, DEFAULT_PDP_CTX_INF_BUF_SIZE-pdp_ctx_inf_buf_offset);
      pdp_ctx_inf_buf_offset = DEFAULT_PDP_CTX_INF_BUF_SIZE;
      pdp_ctx_inf_write_cfg_file();
      uartconfig_state = CONF_STATE_IPR;
      w_uartConfigHdlr();
      break;
    }

    check_result = check_if_redundant_OK_recved(rsp_buf);
    if(1 == check_result)
    {
      char *p = NULL;

      LOG("Redundant rsp string \"OK\" recved.\n");

      p = skip_line_break(rsp_buf);
      p = skip_valid_chars(p);
      p = skip_line_break(p);

      if('\0' == *p){
        LOG("No more rsp string left. Continue to recv...\n");
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkPdpCtxHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        int rm_len = p-rsp_buf;
        memmove(rsp_buf, p, read_bytes-rm_len);
        memset(rsp_buf+read_bytes-rm_len, 0, rm_len);
        read_bytes -= rm_len; // For updating pdp_ctx_inf_buf_offset below correctly
      }
    }

    memcpy(pdp_ctx_inf_buf+pdp_ctx_inf_buf_offset, rsp_buf, read_bytes);
    pdp_ctx_inf_buf_offset += read_bytes;

    p_OK = strstr(rsp_buf, "OK");
    if(NULL == p_OK){
      //Assume there must be rsp str "OK"
      LOG("The recved rsp is incomplete. Continue to recv more.\n");
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkPdpCtxHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    //"OK" recved
    //Add line separator
    memcpy(pdp_ctx_inf_buf+pdp_ctx_inf_buf_offset, separator, strlen(separator));
    pdp_ctx_inf_buf_offset += strlen(separator);
    //Jump into next state
    check_pdp_ctx_state ++;
    if(CHECK_PDP_CTX_STATE_MAX != check_pdp_ctx_state){
      w_checkPdpCtxHdlr();
    }else{
    //Help to print string
    if(pdp_ctx_inf_buf_offset >= DEFAULT_PDP_CTX_INF_BUF_SIZE)
    {
      //Basically, pdp_ctx_inf_buf_offset == DEFAULT_PDP_CTX_INF_BUF_SIZE
      pdp_ctx_inf_buf = (char *)realloc((void *)pdp_ctx_inf_buf, pdp_ctx_inf_buf_offset+1);
      pdp_ctx_inf_buf[pdp_ctx_inf_buf_offset] = '\0'; // Last byte
    }

#ifdef FEATURE_ENABLE_PRINT_PDP_CTX_INF_BUF
				{
					#ifdef FEATURE_DEBUG_QXDM
						int i = 0, prt_buf_len = 0;
						char *front = NULL, *end = NULL, *prt_buf = NULL;
						
						LOG2("************************************************************\n");
						LOG("The content of pdp_ctx_inf_buf: \n");
						front = pdp_ctx_inf_buf;
						while(i < pdp_ctx_inf_buf_offset){
							for(; i<pdp_ctx_inf_buf_offset&&pdp_ctx_inf_buf[i]!='\r'&&pdp_ctx_inf_buf[i]!='\n'; i++);
							end = pdp_ctx_inf_buf+i;
							
							if(0 == prt_buf_len){
								prt_buf = (char *)malloc(end-front+1);
								prt_buf_len = end-front+1;
							}else if(end-front+1 > prt_buf_len){
								prt_buf = (char *)realloc(prt_buf, end-front+1);
								prt_buf_len = end-front+1; // Can't be moved outside!
							}
							memset(prt_buf, 0, prt_buf_len);
							memcpy(prt_buf, front, end-front);
							LOG2("  %s\n", prt_buf);
							
							for(; i<pdp_ctx_inf_buf_offset&&(pdp_ctx_inf_buf[i]=='\r'||pdp_ctx_inf_buf[i]=='\n'); i++);
							if(i == pdp_ctx_inf_buf_offset)
								break;
							front = pdp_ctx_inf_buf+i;
						}
						free(prt_buf);
						LOG2("\n************************************************************\n");
					#else
						#ifndef FEATURE_DEBUG_LOG_FILE
							int i = 0;
							LOG2("************************************************************\n");
							LOG("The content of pdp_ctx_inf_buf: \n");
							for(; i<pdp_ctx_inf_buf_offset&&pdp_ctx_inf_buf[i]!='\0'; i++){
								LOG2("%c", pdp_ctx_inf_buf[i]);
							}
							LOG2("\n************************************************************\n");
						#else
							pthread_mutex_lock(&log_file_mtx);
							LOG5("\n************************************************************\n");
							LOG3("%s(%d)-%s: The content of pdp_ctx_inf_buf: \n", __FILE__, __LINE__, __FUNCTION__);
							LOG3("%s", pdp_ctx_inf_buf);
							LOG3("\n************************************************************\n");
							pthread_mutex_unlock(&log_file_mtx);
						#endif /*FEATURE_DEBUG_LOG_FILE*/
					#endif /*FEATURE_DEBUG_QXDM*/
				}
#endif
      isSIM5360PdpContAcqed = true; // Never reset
      pdp_ctx_inf_write_cfg_file();
      uartconfig_state = CONF_STATE_IPR;
      w_uartConfigHdlr();
    }

    break;
  }

  default:
    LOG("ERROR: Wrong check_pdp_ctx_state (%d) found!\n", check_pdp_ctx_state);
    break;
  } // switch end

  return;
}

//Description:
//SET_PDP_CTX_STATE_CGDCONT is only for setting APN.
void w_setPdpCtxHdlr(void)
{
  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(set_pdp_ctx_state)
  {
  case SET_PDP_CTX_STATE_CGDCONT:
  case SET_PDP_CTX_STATE_CGAUTH:
  {
    char cmd_str[MAX_SET_PDP_CONTEXT_AT_CMD_STR_LEN] = {0};

    sprintf(cmd_str, "%s",
              (SET_PDP_CTX_STATE_CGDCONT == set_pdp_ctx_state)?(sim5360_config_data.cgdcont5360write):(sim5360_config_data.cgauth5360write)
            );

    if('\0' == cmd_str[0] && SET_PDP_CTX_STATE_CGAUTH != set_pdp_ctx_state){
				LOG("NOTICE: set_pdp_ctx_state=%d. No pdp context setting request found.\n", set_pdp_ctx_state);
				set_pdp_ctx_state ++;
				w_setPdpCtxHdlr();
				break;
			}else if('\0' == cmd_str[0] && SET_PDP_CTX_STATE_CGAUTH == set_pdp_ctx_state){
				LOG("NOTICE: set_pdp_ctx_state=%d. No pdp context setting request found.\n", set_pdp_ctx_state);
				pthread_mutex_lock(&i1r_mtx);
				if(is1stRunOnMdmEmodem == true){
					uartconfig_state = CONF_STATE_IPR;
					w_uartConfigHdlr();
				}else{
					//Restart process
					ACQ_ACQ_DATA_STATE_MTX;
					acq_data_state = ACQ_DATA_STATE_COPS_1;
					w_acqDataHdlr();
					RLS_ACQ_DATA_STATE_MTX;
				}
				pthread_mutex_unlock(&i1r_mtx);
				isSIM5360PdpContextSet = true; // Never reset anymore
				break;
			}
			
    LOG("    --> \"%s\"  \n", cmd_str);
    writer(at_config.serial_fd, 
            cmd_str, 
            strlen(cmd_str)
          );
    INIT_ATBUF;
    set_evt(&at_config.AT_event, at_config.serial_fd, false, r_setPdpCtxHdlr, NULL);
    add_evt (&at_config.AT_event, 'r');
    break;
  }

  default:
    LOG("ERROR: Wrong set_pdp_ctx_state (%d) found!\n", set_pdp_ctx_state);
    break;
  }

  return;
}

void r_setPdpCtxHdlr(void *userdata)
{
	#define DEFAULT_SET_PDP_CTX_RSP_BUF_SIZE (64)
	
	char rsp_buf[DEFAULT_SET_PDP_CTX_RSP_BUF_SIZE] = {0};
	int read_bytes = -1;
	
	if(at_config.serial_fd < 0){
		LOG("ERROR: at_config.serial_fd < 0!\n");
		RESET_GLOBAL_VAR;
		return;
	}
	
  switch(set_pdp_ctx_state)
  {
  case SET_PDP_CTX_STATE_CGDCONT:
  case SET_PDP_CTX_STATE_CGAUTH:
  {
			char *p_OK = NULL;
			
			
			_R_SETPDPCTXHDLR_READ_AGAIN__:
			read_bytes = read(at_config.serial_fd, rsp_buf, DEFAULT_SET_PDP_CTX_RSP_BUF_SIZE);
			if(read_bytes < 0 || read_bytes == 0){
				if(errno == EAGAIN || errno == EINTR || read_bytes == 0){
					goto _R_SETPDPCTXHDLR_READ_AGAIN__;
				}
				
				LOG("ERROR: read() failed. errno=%d.\n", errno);
				goto _R_SETPDPCTXHDLR_READ_AGAIN__;
			}
			
			//read_bytes > 0
			p_OK = strstr(rsp_buf, "OK");
			if(NULL == p_OK){
				LOG("The rsp \"OK\" not found. Msg: \"%s\". Continue to read...\n", rsp_buf);
				goto _R_SETPDPCTXHDLR_READ_AGAIN__;
			}
			
			//"OK" recved
			if(SET_PDP_CTX_STATE_CGAUTH == set_pdp_ctx_state){
				pthread_mutex_lock(&i1r_mtx);
				if(is1stRunOnMdmEmodem == true){
					uartconfig_state = CONF_STATE_IPR;
					w_uartConfigHdlr();
				}else{
					//Restart process
					ACQ_ACQ_DATA_STATE_MTX;
					acq_data_state = ACQ_DATA_STATE_COPS_1;
					w_acqDataHdlr();
					RLS_ACQ_DATA_STATE_MTX;
				}
				pthread_mutex_unlock(&i1r_mtx);
				isSIM5360PdpContextSet = true; // Never reset anymore
			}else{
				set_pdp_ctx_state ++;
				w_setPdpCtxHdlr();
			}
			
			break;
		}
		
		default:
			LOG("ERROR: Wrong set_pdp_ctx_state (%d) found!\n", set_pdp_ctx_state);
			break;
	} // switch end
	
	return;
}
#endif // FEATURE_SET_PDP_CONTEXT

#ifdef FEATURE_CONFIG_SIM5360
void r_acqMfgInfHdlr(void *userdata);

void mfg_inf_conv_buf_into_struct(void)
{
	char *front = NULL, *end = NULL;
	int idx = 1;
	
	front = mfg_inf_buf;
	for(; idx<ACQ_MFG_INF_STATE_MAX; idx++){
		end = strstr(front, "*****");
		end -= 2; // Jump back to the '\r' of separator
		
		switch(idx){
			case ACQ_MFG_INF_STATE_SIMCOMATI:{
				memset(sim5360_config_data.simcomati5360, 0, sizeof(sim5360_config_data.simcomati5360));
				memcpy(sim5360_config_data.simcomati5360, front, end-front);
				break;
			}
			case ACQ_MFG_INF_STATE_CSUB:{
				memset(sim5360_config_data.csub5360, 0, sizeof(sim5360_config_data.csub5360));
				memcpy(sim5360_config_data.csub5360, front, end-front);
				break;
			}
			case ACQ_MFG_INF_STATE_SIMEI:{
				memset(sim5360_config_data.simei5360, 0, sizeof(sim5360_config_data.simei5360));
				memcpy(sim5360_config_data.simei5360, front, end-front);
				break;
			}
			case ACQ_MFG_INF_STATE_CICCID:{
				memset(sim5360_config_data.ciccid5360, 0, sizeof(sim5360_config_data.ciccid5360));
				memcpy(sim5360_config_data.ciccid5360, front, end-front);
				break;
			}
			case ACQ_MFG_INF_STATE_CNVR_1:{
				memset(sim5360_config_data.cnvr5360, 0, sizeof(sim5360_config_data.cnvr5360));
				memcpy(sim5360_config_data.cnvr5360, front, end-front);
				break;
			}
			case ACQ_MFG_INF_STATE_CPIN:{
				memset(sim5360_config_data.cpin5360, 0, sizeof(sim5360_config_data.cpin5360));
				memcpy(sim5360_config_data.cpin5360, front, end-front);
				break;
			}
			default:
				break;
		} // switch end
		
		if(idx == ACQ_MFG_INF_STATE_CPIN)
			continue; // Exit
		front = skip_customed_line_separator(end);
	}
}

void mfg_inf_write_cfg_file(void)
{
  mfg_inf_conv_buf_into_struct();

  write_atcmd5360_file();

  free(mfg_inf_buf);
  mfg_inf_buf_offset = 0;
}

void w_acqMfgInfHdlr(void)
{
  const char *acq_inf_at_cmd[] = {
    "AT",
    "AT+SIMCOMATI",
    "AT+CSUB",
    "AT+SIMEI?",
    "AT+CICCID",
    "AT+CNVR=2499,0",
    "AT+CPIN?",
    "AT"
  };
  const int acq_inf_at_cmd_len[] = {
    sizeof("AT"),
    sizeof("AT+SIMCOMATI"),
    sizeof("AT+CSUB"),
    sizeof("AT+SIMEI?"),
    sizeof("AT+CICCID"),
    sizeof("AT+CNVR=2499,0"),
    sizeof("AT+CPIN?"),
    sizeof("AT")
  };

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(acq_mfg_inf_state)
  {
  case ACQ_MFG_INF_STATE_SIMCOMATI:
  case ACQ_MFG_INF_STATE_CSUB:
  case ACQ_MFG_INF_STATE_SIMEI:
  case ACQ_MFG_INF_STATE_CICCID:
  case ACQ_MFG_INF_STATE_CPIN:
  case ACQ_MFG_INF_STATE_CNVR_1:{
    LOG("    --> \"%s\"  \n", acq_inf_at_cmd[(int)acq_mfg_inf_state]);

    writer(at_config.serial_fd, 
            acq_inf_at_cmd[(int)acq_mfg_inf_state], 
            acq_inf_at_cmd_len[(int)acq_mfg_inf_state]-1
          );
    INIT_ATBUF;
    set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqMfgInfHdlr, NULL);
    add_evt (&at_config.AT_event, 'r');
    break;
  }
  default:
    LOG("ERROR: Wrong acq_mfg_inf_state (%d) found!\n", acq_mfg_inf_state);
    break;
  }
}

void r_acqMfgInfHdlr(void *userdata)
{
  char rsp_buf[DEFAULT_MFG_INF_BUF_SIZE] = {0}, separator[] = "\r\n*****\r\n";
  int read_bytes = -1;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(acq_mfg_inf_state)
  {
  case ACQ_MFG_INF_STATE_SIMCOMATI:
  case ACQ_MFG_INF_STATE_CSUB:
  case ACQ_MFG_INF_STATE_SIMEI:
  case ACQ_MFG_INF_STATE_CICCID:
  case ACQ_MFG_INF_STATE_CNVR_1:
  case ACQ_MFG_INF_STATE_CPIN:
  {
    char *p_OK = NULL, *p_ERROR = NULL, check_result = -1;

    if(ACQ_MFG_INF_STATE_SIMCOMATI == acq_mfg_inf_state)
    {
      mfg_inf_buf = (char *)malloc(DEFAULT_MFG_INF_BUF_SIZE);
      memset(mfg_inf_buf, 0, DEFAULT_MFG_INF_BUF_SIZE);
      mfg_inf_buf_offset = 0;
    }

_R_ACQMFGINFHDLR_READ_AGAIN__:
    read_bytes = read(at_config.serial_fd, rsp_buf, DEFAULT_MFG_INF_BUF_SIZE);
    if(read_bytes < 0 || read_bytes == 0){
    if(errno == EAGAIN || errno == EINTR || read_bytes == 0){
					goto _R_ACQMFGINFHDLR_READ_AGAIN__;
				}
				
				LOG("ERROR: read() failed. errno=%d.\n", errno);
				goto _R_ACQMFGINFHDLR_READ_AGAIN__;
			}
			
			//read_bytes > 0
			//It may include "+STIN: 21" or "+STIN: 25" inside rsp_buf.
			if(read_bytes + mfg_inf_buf_offset > DEFAULT_MFG_INF_BUF_SIZE){
				LOG("ERROR: Unexpected length of data recved. Truncate the recved data and end the process!\n");
				memcpy(mfg_inf_buf+mfg_inf_buf_offset, rsp_buf, DEFAULT_MFG_INF_BUF_SIZE-mfg_inf_buf_offset);
				mfg_inf_buf_offset = DEFAULT_MFG_INF_BUF_SIZE;
				mfg_inf_write_cfg_file();
				uartconfig_state = CONF_STATE_IPR;
				w_uartConfigHdlr();
				break;
			}
			
			check_result = check_if_redundant_OK_recved(rsp_buf);
			if(1 == check_result){
				char *p = NULL;
				
				LOG("Redundant rsp string \"OK\" recved.\n");
				
				p = skip_line_break(rsp_buf);
				p = skip_valid_chars(p);
				p = skip_line_break(p);
				
				if('\0' == *p){
					LOG("No more rsp string left. Continue to recv.\n");
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqMfgInfHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					int rm_len = p-rsp_buf;
					memmove(rsp_buf, p, read_bytes-rm_len); // Fixed by rjf at 20150826
					memset(rsp_buf+read_bytes-rm_len, 0, rm_len);
					read_bytes -= rm_len; // For updating mfg_inf_buf_offset below correctly
				}
			}
			
			memcpy(mfg_inf_buf+mfg_inf_buf_offset, rsp_buf, read_bytes);
			mfg_inf_buf_offset += read_bytes;
			
			p_OK = strstr(rsp_buf, "OK");
			if(NULL == p_OK){
				if((p_ERROR = strstr(rsp_buf, "ERROR")) == NULL 
					&& ACQ_MFG_INF_STATE_CPIN != acq_mfg_inf_state){
					LOG("The recved rsp is incomplete. Continue to recv more.\n");
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqMfgInfHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}
			}
			
      //"OK" recved
      //Add line separator
      memcpy(mfg_inf_buf+mfg_inf_buf_offset, separator, strlen(separator));
      mfg_inf_buf_offset += strlen(separator);
      acq_mfg_inf_state ++; //Jump into next state
      if(ACQ_MFG_INF_STATE_MAX != acq_mfg_inf_state)
      {
        w_acqMfgInfHdlr();
      }else{
        if(mfg_inf_buf_offset >= DEFAULT_MFG_INF_BUF_SIZE)
        {
          mfg_inf_buf = (char *)realloc((void *)mfg_inf_buf, mfg_inf_buf_offset+1);
          mfg_inf_buf[mfg_inf_buf_offset] = '\0'; // Last byte
        }

#ifdef FEATURE_ENABLE_PRINT_MFG_INF_BUF
				{
					#ifdef FEATURE_DEBUG_QXDM
						int i = 0, prt_buf_len = 0;
						char *front = NULL, *end = NULL, *prt_buf = NULL;
						
						LOG2("************************************************************\n");
						LOG("The content of mfg_inf_buf: \n");
						front = mfg_inf_buf;
						while(i < mfg_inf_buf_offset){
							for(; i<mfg_inf_buf_offset&&mfg_inf_buf[i]!='\r'&&mfg_inf_buf[i]!='\n'; i++);
							end = mfg_inf_buf+i;
							
							if(0 == prt_buf_len){
								prt_buf = (char *)malloc(end-front+1);
								prt_buf_len = end-front+1;
							}else if(end-front+1 > prt_buf_len){
								prt_buf = (char *)realloc(prt_buf, end-front+1);
								prt_buf_len = end-front+1; // Can't be moved outside!
							}
							memset(prt_buf, 0, prt_buf_len);
							memcpy(prt_buf, front, end-front);
							LOG2("  %s\n", prt_buf);
							
							for(; i<mfg_inf_buf_offset&&(mfg_inf_buf[i]=='\r'||mfg_inf_buf[i]=='\n'); i++);
							if(i == mfg_inf_buf_offset)
								break;
							front = mfg_inf_buf+i;
						}
						free(prt_buf);
						LOG2("\n************************************************************\n");
					#else
						#ifndef FEATURE_DEBUG_LOG_FILE
							int i = 0;
							LOG2("************************************************************\n  ");
							LOG("The content of mfg_inf_buf: \n");
							for(; i<mfg_inf_buf_offset&&mfg_inf_buf[i]!='\0'; i++){
								LOG2("%c", mfg_inf_buf[i]);
							}
							LOG2("\n************************************************************\n");
						#else
							pthread_mutex_lock(&log_file_mtx);
							LOG5("\n************************************************************\n  ");
							LOG3("%s(%d)-%s: The content of mfg_inf_buf: \n", __FILE__, __LINE__, __FUNCTION__);
							LOG3("%s", mfg_inf_buf);
							LOG3("\n************************************************************\n");
							pthread_mutex_unlock(&log_file_mtx);
						#endif /*FEATURE_DEBUG_LOG_FILE*/
					#endif /*FEATURE_DEBUG_QXDM*/
      }
#endif

      isSIM5360BasicInfoAcqed = true; // Never reset
      mfg_inf_write_cfg_file();
#if 1
      if(!isSIM5360PdpContAcqed ) //xiaobin
      {
        LOG(" get SIM5360 pdp...\n");
        check_pdp_ctx_state = CHECK_PDP_CTX_STATE_CGDCONT;
        w_checkPdpCtxHdlr();
        break;
      }
#endif
      uartconfig_state = CONF_STATE_IPR;
      w_uartConfigHdlr();
    }

    break;
  }

  default:
    LOG("ERROR: Wrong acq_mfg_inf_state (%d) found!\n", acq_mfg_inf_state);
    break;
  }
}
#endif // FEATURE_MDMEmodem_MFG_TEST_INF

void AtCmd_cclk_hdl(void *rsp_ptr)
{
  #define RSP_BUF_SIZE (512)

  char rsp_buf[RSP_BUF_SIZE] = {0}, *cur = NULL;
  int read_bytes = -1;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    return;
  }

  _R_ATCMDCCLKHDL_READ_AGAIN__:
  memset(rsp_buf, 0, sizeof(rsp_buf));
  read_bytes = read(at_config.serial_fd, rsp_buf, RSP_BUF_SIZE);
  if(read_bytes < 0 || read_bytes == 0)
  {
    if(read_bytes == 0 || errno == EINTR || errno == EAGAIN)
      goto _R_ATCMDCCLKHDL_READ_AGAIN__;

    LOG("read() failed. errno=%d.\n", errno);
    goto _R_ATCMDCCLKHDL_READ_AGAIN__;
  }

  cur = skip_line_break(rsp_buf);
  if(0 == strncmp(cur, "+STIN: 21", sizeof("+STIN: 21")-1) ||
      0 == strncmp(cur, "+STIN: 25", sizeof("+STIN: 25")-1)
  ){
    LOG("\"+STIN: 21\" or \"+STIN: 25\" is recved. Try again...\n");
    goto _R_ATCMDCCLKHDL_READ_AGAIN__;
  }

  LOG("     <-- \"%s\"\n", rsp_buf);
  memcpy(rsp_ptr, rsp_buf, (read_bytes>RSP_BUF_SIZE)?RSP_BUF_SIZE:read_bytes);

  
}

//Description:
//Not parse the rsp of AT cmd, but just copy the rsp data into the pointer userdata. (Tip: userdata can't be local var.)
//Param:
//No need to add '\r' and '\n' in cmd_str.
//The userdata must be a pointer which is pointed to a 512-byte buffer.
void customedAtCmdHdlr(char *cmd_str, void *rsp_ptr)
{
  #define RSP_BUF_SIZE (512)

  char rsp_buf[RSP_BUF_SIZE] = {0}, *cur = NULL;
  int read_bytes = -1;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }
  
  if(cmd_str == NULL){
    LOG("ERROR: Wrong param input. cmd_str=null.\n");
    RESET_GLOBAL_VAR;
    return;
  }
  if((cmd_str[0]!='a' && cmd_str[0]!='A') ||
    (cmd_str[1]!='t' && cmd_str[1]!='T')
  ){
    //Not possible to reach here
    LOG("ERROR: Wrong cmd_str format found. cmd_str: %s.\n", cmd_str);
    RESET_GLOBAL_VAR;
    return;
  }

  LOG("     --> \"%s\"\n", cmd_str);
  writer(at_config.serial_fd, 
          cmd_str, 
          strlen(cmd_str)
        );

__R_CUSTOMEDATCMDHDLR_READ_AGAIN__:
  memset(rsp_buf, 0, sizeof(rsp_buf));
  read_bytes = read(at_config.serial_fd, rsp_buf, RSP_BUF_SIZE);
  if(read_bytes < 0 || read_bytes == 0)
  {
    if(read_bytes == 0 || errno == EINTR || errno == EAGAIN)
      goto __R_CUSTOMEDATCMDHDLR_READ_AGAIN__;

    LOG("read() failed. errno=%d.\n", errno);
    goto __R_CUSTOMEDATCMDHDLR_READ_AGAIN__;
  }

  cur = skip_line_break(rsp_buf);
  if(0 == strncmp(cur, "+STIN: 21", sizeof("+STIN: 21")-1) ||
      0 == strncmp(cur, "+STIN: 25", sizeof("+STIN: 25")-1)
  ){
    LOG("\"+STIN: 21\" or \"+STIN: 25\" is recved. Try again...\n");
    goto __R_CUSTOMEDATCMDHDLR_READ_AGAIN__;
  }

  LOG("     <-- \"%s\"\n", rsp_buf);
  memcpy(rsp_ptr, rsp_buf, (read_bytes>RSP_BUF_SIZE)?RSP_BUF_SIZE:read_bytes);
}

#ifdef FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED
//Param:
// opt: (For opt == REATTACH_MDMEmodem_STATE_DEATTACH)
// 1: Exec "AT+CFUN=0"
// 2: Exec "AT+CFUN=1"
void reattachMdmEmodemHdlr(char opt, char param)
{
#define REATTACH_MDMEmodem_HDLR_RSP_BUF_SIZE (64)
#define REATTACH_MDMEmodem_STATE_MIN (0)
#define REATTACH_MDMEmodem_STATE_DEATTACH (1)
#define REATTACH_MDMEmodem_STATE_ATTACH (2)
#define REATTACH_MDMEmodem_STATE_MAX (3)
#define AT_CMD_LEN (sizeof("AT+CFUN=0"))
#define REATTACH_FLOW_FINAL_RSP_STR "PNN UPDATING"

  char cmd_str_1[AT_CMD_LEN], cmd_str_2[] = "AT+CFUN=1", rsp_buf[REATTACH_MDMEmodem_HDLR_RSP_BUF_SIZE] = {0};
  int read_bytes = -1;
  char *cur = NULL, *p_cmd_str = NULL;
  bool isOkRecved = false, isFinalRspRecved = false;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }
  if(opt >= REATTACH_MDMEmodem_STATE_MAX ||
    opt <= REATTACH_MDMEmodem_STATE_MIN)
  {
    LOG("ERROR: Wrong param input. opt=%d.\n", opt);
    RESET_GLOBAL_VAR;
    return;
  }

  if(REATTACH_MDMEmodem_STATE_DEATTACH == opt){
    memset(cmd_str_1, 0, sizeof(cmd_str_1));
    switch(param){
    case 0:
      memcpy(cmd_str_1, "AT+CFUN=0", sizeof("AT+CFUN=0")-1);
      break;
    case 1:
      memcpy(cmd_str_1, "AT+CFUN=4", sizeof("AT+CFUN=4")-1);
      break;
    default:
      break;
    }
  }
  p_cmd_str = (opt == REATTACH_MDMEmodem_STATE_DEATTACH)?cmd_str_1:cmd_str_2;
  LOG("     --> \"%s\"\n", p_cmd_str);
  writer(at_config.serial_fd, 
          p_cmd_str, 
          strlen(p_cmd_str)
        );

__R_REATTACHMDMEmodemHDLR_READ_AGAIN__:
  memset(rsp_buf, 0, sizeof(rsp_buf));
  read_bytes = read(at_config.serial_fd, rsp_buf, REATTACH_MDMEmodem_HDLR_RSP_BUF_SIZE);
  if(read_bytes < 0 || read_bytes == 0)
  {
    if(read_bytes == 0 || errno == EINTR || errno == EAGAIN)
      goto __R_REATTACHMDMEmodemHDLR_READ_AGAIN__;

    LOG("read() failed. errno=%d.\n", errno);
    goto __R_REATTACHMDMEmodemHDLR_READ_AGAIN__;
  }

  //read_bytes > 0
  LOG("     <-- \"%s\"\n", rsp_buf);
  cur = skip_line_break(rsp_buf);
  if(0 == strncmp(cur, "+STIN: 21", sizeof("+STIN: 21")-1) ||
      0 == strncmp(cur, "+STIN: 25", sizeof("+STIN: 25")-1))
  {
    LOG("\"+STIN: 21\" or \"+STIN: 25\" is recved. Try again...\n");
    goto __R_REATTACHMDMEmodemHDLR_READ_AGAIN__;
  }

  if(!isOkRecved){
    if(0 == strncmp(cur, "OK", 2)){
      isOkRecved = true;

      if(REATTACH_MDMEmodem_STATE_ATTACH == opt){
				do{
					cur = skip_line_break(skip_valid_chars(cur));
					if(*cur == '\0'){
						LOG("\"OK\" is recved. Need to recv more...\n");
						break;
					}else{
						//Something unparsed
						if(0 != strncmp(cur, REATTACH_FLOW_FINAL_RSP_STR, sizeof(REATTACH_FLOW_FINAL_RSP_STR)-1)){
							LOG("Unparsed msg found. Msg: %s. Need to recv more...\n",
									cur);
						}else{
							isFinalRspRecved = true;
							break;
						}
					}
				}while(!isFinalRspRecved);
				
				if(!isFinalRspRecved){
					LOG("\"%s\" not found. Need to recv more...\n", REATTACH_FLOW_FINAL_RSP_STR);
					goto __R_REATTACHMDMEmodemHDLR_READ_AGAIN__;
				}else{
					LOG("\"%s\" found.\n", REATTACH_FLOW_FINAL_RSP_STR);
				}
			}
		}else{
			LOG("Unexpected msg recved. Msg: %s. Continue to recv more...\n",
					cur);
			goto __R_REATTACHMDMEmodemHDLR_READ_AGAIN__;
		}
	}
	else{
		if(REATTACH_MDMEmodem_STATE_ATTACH == opt){
			if(0 == strncmp(cur, REATTACH_FLOW_FINAL_RSP_STR, sizeof(REATTACH_FLOW_FINAL_RSP_STR)-1)){
				LOG("\"%s\" found.\n",
						REATTACH_FLOW_FINAL_RSP_STR);
				//Final rsp found
				goto __EXIT_OF_REATTACHMDMEmodemHDLR__;
			}
			
			//Final rsp not found
			do{
				cur = skip_line_break(skip_valid_chars(cur));
				if(*cur == '\0'){
					LOG("\"OK\" is recved. Need to recv more...\n");
					break;
				}else{
					//Something unparsed
					if(0 != strncmp(cur, REATTACH_FLOW_FINAL_RSP_STR, sizeof(REATTACH_FLOW_FINAL_RSP_STR)-1)){
						LOG("Unparsed msg found. Msg: %s. Need to recv more...\n",
								cur);
					}else{
						isFinalRspRecved = true;
						break;
					}
				}
			}while(!isFinalRspRecved);
			
			if(!isFinalRspRecved){
				LOG("\"%s\" not found. Need to recv more...\n", REATTACH_FLOW_FINAL_RSP_STR);
				goto __R_REATTACHMDMEmodemHDLR_READ_AGAIN__;
			}else{
				LOG("\"%s\" found.\n", REATTACH_FLOW_FINAL_RSP_STR);
			}
		}else{
			LOG("ERROR: \"OK\" is recved, but request to recv more rsp unexpectedly!opt=%d, param=%d.\n",
					opt, param);
		}
	}
	
__EXIT_OF_REATTACHMDMEmodemHDLR__:
  return;
}
#endif // FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED

void w_checkNetHdlr(void)
{
  int ret;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(chk_net_state)
  {
  case CHK_NET_STATE_IDLE:
    LOG("NOTICE: AT event=0x%x.\n", (unsigned int)(&at_config.AT_event));

    case CHK_NET_STATE_CSQ:
    case CHK_NET_STATE_CGREG:
    {
#ifdef FEATURE_ENABLE_ATE0_RSP_TIMER
      if(CHK_NET_STATE_IDLE == chk_net_state)
      {
        add_timer_ate0_rsp_timeout();
      }
#endif

      if(!isSIM5360PrebuiltAtcmdFileRead)
      {
        read_atcmd5360_file(&sim5360_config_data);
        isSIM5360PrebuiltAtcmdFileRead = true; // Never reset unless reboot
      }

      LOG("    --> \"%s\"  \n", ChkNetAT[(int)chk_net_state]);
      ret = writer(at_config.serial_fd, ChkNetAT[(int)chk_net_state], ChkNetATLen[(int)chk_net_state]-1);
      if(ret != 1){
        LOG("ERROR: writer() error. chk_net_state=%d.\n", (int)chk_net_state);
        RESET_GLOBAL_VAR;
        break;
      }
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }
    default:
    {
      LOG("ERROR: Wrong chk_net_state found. chk_net_state=%d.\n", chk_net_state);
      RESET_GLOBAL_VAR;
      break;
    }
  }

  return;
}

//NOTE:
//	If AT+CSQ or At+CGREG? returns unsatisfied result,
//	flow will end here rather than keep inquiring over and over again.
//	When user found this, they should fix the problem and restart the device.
void r_checkNetHdlr(void *userdata)
{
  int ret;
  char *ptr = NULL;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    goto _EXIT_OF_R_CHECKNETHDLR_;
  }

  switch(chk_net_state)
  {
  case CHK_NET_STATE_IDLE:
  {
    LOG("chk_net_state == CHK_NET_STATE_IDLE.\n");

    //NOTE:
    //It might have set ATE0 before(i.e. Application is restarted and hardware is still on.), so
    //it may lose the response of ATE0.
__CHK_NET_STATE_IDLE_READER_AGAIN__:
    READER__CHK_NET(__CHK_NET_STATE_IDLE_READER_AGAIN__);

    if(*ATBufCur != '\0' && ret == 1)
    {
      int r = -1;

      r = get_OK();//r == 2 (Not recv "OK")
      if(0 == r){
        //Not possible to reach here.
        LOG("ERROR: get_OK() error. r=%d.\n", r);
        RESET_GLOBAL_VAR;
        break;
      }
    }else if(*ATBufCur == '\0' && ret == 1)
    {
      //It is fine to ignore the "OK".
      LOG("Message \"OK\" missed.\n");
    }else
    {
    }

    //*****Clear ate0 rsp timer*****
    //Recv ate0 rsp, and clear expired ate0 rsp timer in Timeouts_list.
    //Add "ret == 3" by rjf at 20151105 (For macro FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION.)
    if(ret == 1 || ret == 3 || ret == 4)
    {
      if(isAte0RspTimerExpired)
      {
        isAte0RspTimerExpired = false;

        LOG("NOTICE: Clear ate0 rsp timer in Timeouts_list.\n");
        clr_timeout_evt(&ate0_rsp_timeout_evt);
      }
      isATEvtAct = false;
      clr_ate0_rsp_timer();
    }

    //ret == 1 or 4
    //Situation Presentation:
    //It hasn't sent ID auth msg to server, and we assume the local virt card is released for other devices.
    //So app has no need to send power down msg to server.
    if(isWaitToPwrDwn)
    {
      LOG("chk_net_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
                                                chk_net_state, isWaitToPwrDwn);
      isWaitToPwrDwn = false;

      if(isPwrDwnNotificationTimerExpired){
        isPwrDwnNotificationTimerExpired = false; // reset

        LOG("NOTICE: Clear power down notification timer in Timeouts_list.\n");
        clr_timeout_evt(&power_down_notification_timeout_evt);
      }
      clr_power_down_notification_timer();

      if(!check_ChannelNum(-1)){
        LOG("ERROR: Wrong ChannelNum(%d) found! chk_net_state(%d) dismatch with ChannelNum.\n",
                                      get_ChannelNum(), chk_net_state);
      }

      ACQ_ACQ_DATA_STATE_MTX;
      set_at_evt_waiting();
      RLS_ACQ_DATA_STATE_MTX;

      isMdmEmodemRunning = false;
      
#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
      mdmEmodem_shutdown();
#endif

      LOG("Send msg to notify other processes.\n");
      msq_send_pd_rsp();

      return;
    }

    chk_net_state = CHK_NET_STATE_CSQ;
    w_checkNetHdlr();
    break;
  } //CHK_NET_STATE_IDLE

  case CHK_NET_STATE_CSQ:
  {
    LOG("chk_net_state == CHK_NET_STATE_CSQ.\n");
    qlinkdatanode_run_stat &= 0x800 ;
    LOG("qlinkdatanode_run_stat == qlinkdatanode_run_stat = %d .\n",qlinkdatanode_run_stat);
 

__CHK_NET_STATE_CSQ_READER_AGAIN__:
    READER__CHK_NET(__CHK_NET_STATE_CSQ_READER_AGAIN__);

    if(ret == 4)
    {
      LOG("Redundant rsp \"OK\" recved. Ignore it and continue to recv more.\n");
      if(*ATBufCur != '\0'){
        LOG("There is still something unread in ATBuf.\n");
        goto __CHK_NET_STATE_CSQ_READER_AGAIN__;
      }else{
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
        add_evt(&at_config.AT_event, 'r');
        return;
      }
    }else if(ret == 1)
    {
      if(*ATBufCur != '\0')
      {
        int r = -1;

        r = get_OK();
        if(r == 0){
          LOG("ERROR: get_OK() error. r=%d.\n", r);
          break;
        }

        //Not care if "OK" is not recved.
        }else{
          LOG("Message \"OK\" missed.\n");
        }
    }
    
    //For macro FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION. If the macro is not defined,
    //this procedure is useful for strengthening the fault tolerance.
    else if(ret == 3)
    {
      LOG("URC rpt \"+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY\" recved. Continue...\n");
      if(*ATBufCur != '\0')
      {
        LOG("There is still something unread in ATBuf. Continue to read...\n");
        goto __CHK_NET_STATE_CGREG_READER_AGAIN__;
      }else{
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        return;
      }
    }

    if(isWaitToPwrDwn)
    {
				LOG("chk_net_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n", chk_net_state, isWaitToPwrDwn);
				isWaitToPwrDwn = false;
				
				if(isTermRcvrNow){
					isTermRcvrNow = false;
				}
				
				ACQ_CHANNEL_LOCK_WR;
				if(isProcOfInitApp){
	 				if(ChannelNum == -1){
						if(isPwrDwnNotificationTimerExpired){
							isPwrDwnNotificationTimerExpired = false; // reset
							
							LOG("NOTICE: Clear power down notification timer in Timeouts_list.\n");
							clr_timeout_evt(&power_down_notification_timeout_evt);
						}
						clr_power_down_notification_timer();
						
						//Not update ChannelNum
						
						ACQ_ACQ_DATA_STATE_MTX;
						set_at_evt_waiting();
						RLS_ACQ_DATA_STATE_MTX;
						
						isMdmEmodemRunning = false;
						#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
							mdmEmodem_shutdown();
						#endif
						
						LOG("Send msg to notify other processes.\n");
						msq_send_pd_rsp();
						return;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! chk_net_state(%d) dismatch with ChannelNum.\n",
								ChannelNum, chk_net_state);
						RESET_GLOBAL_VAR;
						return;
					}
				}else{
					if(4 == ChannelNum || -1 == ChannelNum){
						if(ChannelNum == 4){
							isMifiConnected = false;
							ChannelNum = -1;
						}
						
						isNeedToSendOutReq0x65 = true;
						//Keep working on the old "recovering" process
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! chk_net_state(%d) dismatch with ChannelNum.\n",
								ChannelNum, chk_net_state);
						RESET_GLOBAL_VAR;
						return;
					}
				}
				RLS_CHANNEL_LOCK;
				
    }// isWaitToPwrDwn

    if(isTermRcvrNow)
    {
      LOG("isTermRcvrNow=1. Because chk_net_state=1, nothing need to be done.\n");
      isTermRcvrNow = false;
      //Keep working on the old "recovering" process
    } // isTermRcvrNow

    ret = process_CSQ(&sim5360_dev_info, ptr);
    if(ret == -1)
    {
      LOG("ERROR: process_CSQ() error. ret=-1.\n");
      RESET_GLOBAL_VAR;
      break;
    }else if(ret <= 3 || ret == 99)
    {
				#ifdef FEATURE_ENABLE_MSGQ
					#ifndef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						if(!isPoorSigNotified){
							if(!isMdm9215CpinReady)
								msq_send_qlinkdatanode_run_stat_ind(6);
							else
								msq_send_qlinkdatanode_run_stat_ind(8);
						}
					#else
						if(!isPoorSigNotified){
							msq_send_qlinkdatanode_run_stat_ind(8);
						}
					#endif
					isPoorSigNotified = true;
				#endif
				
#ifndef FEATURE_KEEP_CHECKING_CSQ_UNTIL_RSSI_ISNT_0
					if(CSQCount < CSQ_REPETITION_TIMES){
						sleep(1);
						
						CSQCount++;
						LOG("Continue to check signal strength...\n");
						w_checkNetHdlr();
						break;
					}else{
						LOG("NOTICE: Signal strength is weak. rssi=0 or 99.\n");
					}
#else
					usleep(DEFAULT_RETRY_EXEC_AT_CMD_INTERVAL_TIME);
					

              // factory need
#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD 
          if(isFactoryWriteIMEI5360){
            isFactoryWriteIMEI5360 = true;
            proto_update_flow_state(FLOW_STATE_MDMEmodem_AT_CMD_REQ);
            ACQ_ACQ_DATA_STATE_MTX;
            acq_data_state = (ACQ_DATA_STATE_CUSTOM_AT_CMD);
            w_acqDataHdlr();
            RLS_ACQ_DATA_STATE_MTX;
            return;
          }
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD      // factory need

          LOG("rssi=%d. Continue to check signal strength...\n", ret);

          w_checkNetHdlr();
          break;
#endif
			}else if(ret == -2)
			{ // Unexpected msg found
				if(*ATBufCur == '\0'){
					LOG("Unexpected msg found. Need to recv more...\n");
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf.\n");
					goto __CHK_NET_STATE_CSQ_READER_AGAIN__;
				}
			}
			
#ifdef FEATURE_ENABLE_qlinkdatanode_RUN_STAT_IND
				#ifndef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
					if(!isMdm9215CpinReady)
						msq_send_qlinkdatanode_run_stat_ind(7);
					else
						msq_send_qlinkdatanode_run_stat_ind(9);
				#else
					msq_send_qlinkdatanode_run_stat_ind(9);
				#endif
				isPoorSigNotified = false;
#endif // FEATURE_ENABLE_qlinkdatanode_RUN_STAT_IND

    chk_net_state = CHK_NET_STATE_CGREG;
    w_checkNetHdlr();
    break;
  } //case CHK_NET_STATE_CSQ

  case CHK_NET_STATE_CGREG:
  {
    LOG("chk_net_state == CHK_NET_STATE_CGREG.\n");

__CHK_NET_STATE_CGREG_READER_AGAIN__:
    READER__CHK_NET(__CHK_NET_STATE_CGREG_READER_AGAIN__);


    LOG("Debug:chk_net_state == CHK_NET_STATE_CGREG  ret = %d.\n",ret);


    if(ret == 4)
    {
				LOG("Redundant rsp \"OK\" recved. Continue...\n");
				if(*ATBufCur != '\0'){
					LOG("There is still something unread in ATBuf. Continue to read...\n");
					goto __CHK_NET_STATE_CGREG_READER_AGAIN__;
				}else{
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}
		}else if(ret == 1){
				if(*ATBufCur != '\0'){
					int r = -1;
					
					r = get_OK();
					if(r == 0){
						LOG("ERROR: get_OK() error. r=%d.\n", r);
						break;
					}
					
					//Not care if "OK" is not recved.
				}else{
					LOG("Message \"OK\" missed.\n");
				}
		}
		else if(ret == 3){
				LOG("URC rpt \"+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY\" recved. Continue...\n");
				if(*ATBufCur != '\0'){
					LOG("There is still something unread in ATBuf. Continue to read...\n");
					goto __CHK_NET_STATE_CGREG_READER_AGAIN__;
				}else{
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
      }
    }

    if(isWaitToPwrDwn)
    {
      LOG("chk_net_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
                                                  chk_net_state, isWaitToPwrDwn);
      isWaitToPwrDwn = false;

      if(isTermRcvrNow)
      {
        isTermRcvrNow = false;
      }

      ACQ_CHANNEL_LOCK_WR;
				if(isProcOfInitApp){
	 				if(ChannelNum == -1){
						if(isPwrDwnNotificationTimerExpired){
							isPwrDwnNotificationTimerExpired = false; // reset
							
							LOG("NOTICE: Clear power down notification timer in Timeouts_list.\n");
							clr_timeout_evt(&power_down_notification_timeout_evt);
						}
						clr_power_down_notification_timer();
						
						//Not update ChannelNum
						
						ACQ_ACQ_DATA_STATE_MTX;
						set_at_evt_waiting();
						RLS_ACQ_DATA_STATE_MTX;
						
						isMdmEmodemRunning = false;
						#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
							mdmEmodem_shutdown();
						#endif
						
						LOG("Send msg to notify other processes.\n");
						msq_send_pd_rsp();
						return;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! chk_net_state(%d) dismatch with ChannelNum.\n", ChannelNum, chk_net_state);
						RESET_GLOBAL_VAR;
						return;
					}
				}else{
					if(4 == ChannelNum || -1 == ChannelNum){
						if(ChannelNum == 4){
							isMifiConnected = false;
							ChannelNum = -1;
						}
						
						isNeedToSendOutReq0x65 = true;
						//Keep working on the old "recovering" process
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! chk_net_state(%d) dismatch with ChannelNum.\n", ChannelNum, chk_net_state);
						RESET_GLOBAL_VAR;
						return;
					}
				}
				RLS_CHANNEL_LOCK;
    }

    if(isTermRcvrNow)
    {
      LOG("isTermRcvrNow=1. Because chk_net_state=2, nothing need to be done.\n");
      isTermRcvrNow = false;
      //Keep working on the old "recovering" process
    }

    ret = process_CGREG(ptr);

    LOG("Debug:  ret = process_CGREG(ptr)ret =.%d time)...\n", ret);
    if(ret == -1)
    {
      LOG("ERROR: process_CGREG() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
    else if(ret == 0 || ret == 2 || ret == 3 || ret == 4)
    {
#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      if(!isNotRegNotified)
      {
        msq_send_qlinkdatanode_run_stat_ind(10);
        isNotRegNotified = true;
      }
#endif //FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      //Check USIM card status
      if(false == isSIM5360CpinReady || (0 == ret || 2 == ret))
      {
        char rsp_buf[512] = {0};

        customedAtCmdHdlr("AT+CPIN?", (void *)rsp_buf);
        if(rsp_buf[0] == '\0')
        {
          LOG("ERROR: Fail to check \"AT+CPIN?\".\n");
          RESET_GLOBAL_VAR;
          break;
        }

#ifdef FEATURE_MDMEmodem_RST_FOR_SIMCARD_NOT_AVAIL
        if(strstr(rsp_buf, "+SIMCARD: NOT AVAILABLE") != NULL && isSIM5360CpinReady)
        {
          LOG("USIM card hasn't been detected! Now, restart the whole device.\n");
        //  system("reboot");
		  yy_popen_call("reboot -f");
          break;
        }
#endif /*FEATURE_MDMEmodem_RST_FOR_SIMCARD_NOT_AVAIL*/

        if(strstr(rsp_buf, "+CME ERROR") != NULL)
        {
          msq_send_qlinkdatanode_run_stat_ind(23); // Add by jack li 20160303
          LOG("USIM card is unavailable!\n");
          LOG("Now, set AT event to waiting state.\n");
          isNotNeedIdleTimer = true; //Not add idle timer. This is for manufacturing test. Not reset anymore.
          chk_net_state = CHK_NET_STATE_MAX; // For mdmEmodem_check_busy()
          uartconfig_state = CONF_STATE_MAX; // For mdmEmodem_check_busy()
          set_at_evt_waiting();
          break;
        }

        isSIM5360CpinReady = true; // Not reset any more
      }

	

	  LOG("Debug:isSIM5360CpinReady =.%d time)...\n", isSIM5360CpinReady);
	  LOG("Debug:isSIM5360BasicInfoAcqed =.%d time)...\n", isSIM5360BasicInfoAcqed);

      if(isSIM5360CpinReady && !isSIM5360BasicInfoAcqed) 
      {
        LOG(" SIM5360CpinReady ~isSIM5360BasicInfoAcqed  go to get sim5360 info\n");
        acq_mfg_inf_state = ACQ_MFG_INF_STATE_SIMCOMATI;
        w_acqMfgInfHdlr();
        break;
      }
      
    //Reattach once
#ifdef FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED

      LOG("Debug:isMdmEmodemReattachedWhenRegFailed =.%d time)...\n", isMdmEmodemReattachedWhenRegFailed);

      if(true == isSIM5360CpinReady && false == isMdmEmodemReattachedWhenRegFailed)
      {
        LOG("USIM is ready, but registration failed. Try to reattach mdmEmodem once.\n");
        isMdmEmodemReattachedWhenRegFailed = true; // Never reset it

        reattachMdmEmodemHdlr(1, 0);
        sleep(3); // Wait for "AT+CFUN=0" to take effect
        reattachMdmEmodemHdlr(2, 255);

        chk_net_state = CHK_NET_STATE_CSQ;
        w_checkNetHdlr();
        break;
      }
#endif // FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED

#if 0 //!FEATURE_KEEP_CHECKING_CGREG_UNTIL_REG_SUCCEEDED
      if(CGREGCount < CGREG_REPETITION_TIMES)
      { 
        sleep(1);
        CGREGCount++;
        LOG("Searching net...\n");
        w_checkNetHdlr();
      }else if(CGREGCount >= CGREG_REPETITION_TIMES)
      {
        int time = 1;
        int i = 0;

        for(; i<CGREGCount-CGREG_REPETITION_TIMES+1; i++)
          time *= 2;
        if(time > 128) time = 128;
        sleep(time);
        CGREGCount++;
        LOG("Searching net(No.%d time)...\n", CGREGCount+1);
        w_checkNetHdlr();
      }
#else
      usleep(DEFAULT_RETRY_EXEC_AT_CMD_INTERVAL_TIME);
      CGREGCount++;
      LOG("Searching net (No.%d time)...\n", CGREGCount+1);

      // factory need
#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD 
      if(isFactoryWriteIMEI5360){
        isFactoryWriteIMEI5360 = true;
        proto_update_flow_state(FLOW_STATE_MDMEmodem_AT_CMD_REQ);
        ACQ_ACQ_DATA_STATE_MTX;
        acq_data_state = (ACQ_DATA_STATE_CUSTOM_AT_CMD);
        w_acqDataHdlr();
        RLS_ACQ_DATA_STATE_MTX;
        return;
      }
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD      // factory need
      
      w_checkNetHdlr();
#endif // FEATURE_KEEP_CHECKING_CGREG_UNTIL_REG_SUCCEEDED

      break;
    }else if(ret == -2)
    {
      if(*ATBufCur != '\0'){
        LOG("There is still something unread in ATBuf. Continue to read...\n");
        goto __CHK_NET_STATE_CGREG_READER_AGAIN__;
      }else{
        LOG("Unexpected msg found. Need to recv more...\n");
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_checkNetHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }
    }

  msq_send_qlinkdatanode_run_stat_ind(24);// Add by jack li 20160303

CHK_NET_STATE_CGREG_1:
  
#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
    msq_send_qlinkdatanode_run_stat_ind(11);
    isNotRegNotified = false; // Reset it
#endif

    isSIM5360CpinReady = true; // Not reset any more

#ifdef FEATURE_ENABLE_MDMEmodem_REATTACH_ONCE_WHEN_REGISTRATION_FAILED
    isMdmEmodemReattachedWhenRegFailed = false;
#endif

    CGREGCount = 0;

    LOG("chk net pass, set CHK_NET_STATE_MAX\n");
    chk_net_state = CHK_NET_STATE_MAX;

#ifdef FEATURE_CONFIG_SIM5360
    if(!isSIM5360PdpContextSet)
    {
      set_pdp_ctx_state = SET_PDP_CTX_STATE_CGDCONT;
      w_setPdpCtxHdlr();
      break;
    }
#endif //FEATURE_CONFIG_SIM5360

    pthread_mutex_lock(&i1r_mtx);
    if(is1stRunOnMdmEmodem == true)
    {
      uartconfig_state = CONF_STATE_IPR;
      w_uartConfigHdlr();
    }else{
      //Restart process
      ACQ_ACQ_DATA_STATE_MTX;
      acq_data_state = ACQ_DATA_STATE_COPS_1;
      w_acqDataHdlr();
      RLS_ACQ_DATA_STATE_MTX;
    }
    pthread_mutex_unlock(&i1r_mtx);

    break;
  } // CHK_NET_STATE_CGREG

  default:
  {
    LOG("ERROR: Wrong chk_net_state found. chk_net_state=%d.\n", chk_net_state);
    RESET_GLOBAL_VAR;
    break;
  }
  }//switch chk_net_state end

_EXIT_OF_R_CHECKNETHDLR_:

  return;
}

void w_uartConfigHdlr(void)
{
  int ret;

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch(uartconfig_state)
  {
  case CONF_STATE_IFC:
  case CONF_STATE_IPR:
  {
#ifdef FEATURE_CONFIG_SIM5360
    if(CONF_STATE_IPR == uartconfig_state && false == isSIM5360BasicInfoAcqed)
    {
      LOG("isSIM5360BasicInfoAcqed=0. Now, go to acq mfg inf.\n");
      acq_mfg_inf_state = ACQ_MFG_INF_STATE_SIMCOMATI;
      w_acqMfgInfHdlr();
      break;
    }
    
    if(CONF_STATE_IPR == uartconfig_state && false == isSIM5360PdpContAcqed)
    {
      LOG("isSIM5360PdpContAcqed=0. Now, go to acq pdp ctx inf.\n");
      check_pdp_ctx_state = CHECK_PDP_CTX_STATE_CGDCONT;
      w_checkPdpCtxHdlr();
      break;
    }
#endif //FEATURE_CONFIG_SIM5360

    LOG("    --> \"%s\"  \n", ConfAT[(int)uartconfig_state]);
    ret = writer(at_config.serial_fd, ConfAT[(int)uartconfig_state], ConfATLen[(int)uartconfig_state]-1);
    if(ret != 1)
    {
      LOG("ERROR: writer() error. uartconfig_state=%d.\n", (int)uartconfig_state);
      RESET_GLOBAL_VAR;
      break;
    }
    INIT_ATBUF;
    set_evt(&at_config.AT_event, at_config.serial_fd, false, r_uartConfigHdlr, NULL);
    add_evt(&at_config.AT_event, 'r');
    break;
  }

  default:{
    LOG("ERROR: Wrong uartconfig_state found. uartconfig_state=%d.\n", uartconfig_state);
    RESET_GLOBAL_VAR;
    break;
  }
  }
}

void r_uartConfigHdlr(void *userdata)
{
  char *ptr = NULL;
  int ret;

  switch(uartconfig_state)
  {
  case CONF_STATE_IPR:
  case CONF_STATE_IFC:
  {
    LOG("uartconfig_state=%d.\n", uartconfig_state);

__R_CONFHDLR_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
    if(ret == 0){
				LOG("ERROR: reader() error.\n");
				return;
			}else if(ret == 2){
				if(*ATBufCur == '\0'){
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_uartConfigHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}else{
					LOG("There is still something unread in ATBuf.\n"); \
					goto __R_CONFHDLR_READER_AGAIN__;
				}
			}else if(ret == 10){
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_uartConfigHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				return;
			}
			
    if(NULL == strstr(ptr, "OK")){
      LOG("Rsp \"OK\" not found. Msg: %s.\n", ptr);
      sleep(1);
      //Maintain uartconfig_state
      w_uartConfigHdlr();
      return;
    }

    if(uartconfig_state == CONF_STATE_IPR)
    {
      struct termios  old_termios;
      struct termios  new_termios;

      if(tcgetattr(at_config.serial_fd, &old_termios) != 0)
      {
        LOG("ERROR: tcgetattr() error. errno=%s(%d).\n", strerror(errno), errno);
        return;
      }

      LOG("Set new baud rate 460800Hz.\n");
      new_termios = old_termios;
      cfsetispeed(&new_termios, B460800);
      cfsetospeed(&new_termios, B460800);

      tcsetattr(at_config.serial_fd, TCSANOW, &new_termios);

      usleep(1500*1000);
    }

    //Situation Presentation:
    //It hasn't sent ID auth msg to server, and we assume the local virt card is released for other devices.
    //So app has no need to send power down msg to server.
    if(isWaitToPwrDwn)
    {
      LOG("uartconfig_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n", uartconfig_state, isWaitToPwrDwn);
      isWaitToPwrDwn = false;

      if(!check_ChannelNum(-1)){
        LOG("ERROR: Wrong ChannelNum(%d) found! uartconfig_state(%d) dismatch with ChannelNum.\n",
                                    get_ChannelNum(), uartconfig_state);
      }

      if(isPwrDwnNotificationTimerExpired){
        isPwrDwnNotificationTimerExpired = false; // reset

        LOG("NOTICE: Clear power down notification timer in Timeouts_list.\n");
        clr_timeout_evt(&power_down_notification_timeout_evt);
      }
      clr_power_down_notification_timer();

      ACQ_ACQ_DATA_STATE_MTX;
      set_at_evt_waiting();
      RLS_ACQ_DATA_STATE_MTX;

      isMdmEmodemRunning = false;
        
#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
      mdmEmodem_shutdown();
#endif // FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN

      LOG("Send msg to notify other processes.\n");
      msq_send_pd_rsp();

      return;
    }

    if(uartconfig_state == CONF_STATE_IPR)
    {
      uartconfig_state = CONF_STATE_MAX;

      ACQ_ACQ_DATA_STATE_MTX;
      
#ifdef FEATURE_USE_MDMEmodem_IMEI
      //Note: Not use is1stRunOnMdmEmodem any more after processing below.
      if(is1stRunOnMdmEmodem == true){
        acq_data_state = ACQ_DATA_STATE_CGSN;
      }else{
        acq_data_state = ACQ_DATA_STATE_COPS_1;
      }
#else
      //Acquire IMEI of mdmImodem before this.
      acq_data_state = ACQ_DATA_STATE_COPS_1;
#endif // FEATURE_USE_MDMEmodem_IMEI

      w_acqDataHdlr();
      RLS_ACQ_DATA_STATE_MTX;
      break;
    }else{
      uartconfig_state++;
      w_uartConfigHdlr();
      break;
    }
  }

  default:
  {
    LOG("ERROR: Wrong uartconfig_state found. uartconfig_state=%d.\n", uartconfig_state);
    RESET_GLOBAL_VAR;
    break;
  }
  } // switch
}

//Note:
//Not acquire any lock or mutex inside. (Like acq_data_state_mtx.) 
void w_acqDataHdlr(void)
{
  int ret,itemp,iret=0;
  char whole_cmd[45];
  unsigned int cmd_len = 0;
  char *cmd = NULL;
  
#ifdef  FEATURE_ENABLE_CPOL_CONFIG
  char * p1_temp = NULL;
  char * p2_temp= NULL;
  char * p3_temp = NULL;
  char * p4_temp= NULL;
#endif

 #ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
	 char whole_cmd_CIP[45];
 #endif

  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    RESET_GLOBAL_VAR;
    return;
  }

  switch((int)acq_data_state)
  {

  
  case ACQ_DATA_STATE_NETOPEN:

 #ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
  
   case ACQ_DATA_STATE_CIPOPEN_REQ:
   case ACQ_DATA_STATE_CIPCLOSE:
  
  #endif
	
  case ACQ_DATA_STATE_CIPOPEN:
  case ACQ_DATA_STATE_CIPSEND_1:
    cmd = AcqDataAT[acq_data_state];
    break;

  case ACQ_DATA_STATE_CGSN:
  case ACQ_DATA_STATE_COPS_1:
  case ACQ_DATA_STATE_COPS_2:

#ifdef  FEATURE_ENABLE_CPOL_CONFIG
  case ACQ_DATA_STATE_COPS_3:
  case ACQ_DATA_STATE_COPS_4:
 #endif

 #ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG
   case ACQ_DATA_STATE_CCLK:
 #endif
 
  case ACQ_DATA_STATE_CREG_1:
  case ACQ_DATA_STATE_CREG_2:
  	

 
  case ACQ_DATA_STATE_CREG_3:
#ifdef FEATURE_ENABLE_CATR
  case ACQ_DATA_STATE_CATR_1:
  case ACQ_DATA_STATE_CATR_2:
#endif
    cmd = AcqDataAT[acq_data_state-3];
    break;
  default:
    break;
  }

  LOG("acq_data_state=%d.\n", acq_data_state);

  //NOTE: No need to add '\r' or '\n'.
  //It's done by write_line().
  switch(acq_data_state)
  {
  case ACQ_DATA_STATE_IDLE:
  {
    LOG("ERROR: Wrong acq_data_state found. acq_data_state=%d.\n", acq_data_state);
    RESET_GLOBAL_VAR;
    return;
  }

  case ACQ_DATA_STATE_CIPOPEN:
  {
    char format[] = "%s=%d,\"%s\",\"%s\",%d";
    char format_udp_cipopen[] = "%s=%d,\"%s\",,,%d";

     
// 20160928 by jackli	 
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

	LOG("Debug: g_IdcheckRspis_F3 = %d .\n",g_IdcheckRspis_F3);
	LOG("Debug: g_EmodemUdp_socket_link = %d .\n",g_EmodemUdp_socket_link);
	LOG("Debug: g_EmodemTcp_socket_link = %d .\n",g_EmodemTcp_socket_link);
	LOG("Debug: g_enable_EmodemUdp_link = %d .\n",g_enable_EmodemUdp_link);
       LOG("Debug: g_enable_EmodemUdp_linkcount = %d .\n",g_enable_EmodemUdp_linkcount);
	LOG("Debug: g_EmodemUdp_link_server_state = %d .\n",g_EmodemUdp_link_server_state);

	LOG("Debug: Udp_IP_addr --> \"%s\"  \n", Udp_IP_addr);
	LOG("Debug: udp_server_port = %d.\n",udp_server_port);
	
	LOG("Debug: UDP_default1_addr --> \"%s\"  \n", Udp_IP_addr_default1);
	LOG("Debug: udp_server_port_default1 = %d.\n",udp_server_port_default1);

	LOG("Debug: UDP_default2_addr --> \"%s\"  \n", Udp_IP_addr_default2);
	LOG("Debug: udp_server_port_default2 = %d.\n",udp_server_port_default2);

	LOG("Debug: UDP_default3_addr --> \"%s\"  \n", Udp_IP_addr_default3);
	LOG("Debug: udp_server_port_default3 = %d.\n",udp_server_port_default3);

	LOG("Debug: tcp_IP_addr --> \"%s\"  \n", tcp_IP_addr);
	LOG("Debug: tcp_server_port = %d.\n",tcp_server_port);
	LOG("Debug: tcp_IP_addr_2 --> \"%s\"  \n", tcp_IP_addr_2);
	LOG("Debug: tcp_server_port_2 = %d.\n",tcp_server_port_2);
	LOG("Debug: g_enable_EmodemTCP_link = %d.\n",g_enable_EmodemTCP_link);
	LOG("Debug: g_enable_TCP_IP01_linkcount = %d.\n",g_enable_TCP_IP01_linkcount);
	LOG("Debug: g_enable_TCP_IP02_linkcount = %d.\n",g_enable_TCP_IP02_linkcount);

	if (((g_enable_EmodemUdp_link == true) && (g_enable_EmodemUdp_linkcount < 12)) || (g_IdcheckRspis_F3 == true)){

		at_config.net_config.link_num = 1;
		at_config.net_config.protocol = "UDP";
		at_config.net_config.server_port = 5000;

		
		
		 cmd_len = sprintf(whole_cmd, format_udp_cipopen, cmd, 
                      at_config.net_config.link_num,
                      at_config.net_config.protocol,
                      at_config.net_config.server_port);

		 g_EmodemUdp_socket_link = true;
		 g_IdcheckRspis_F3 = false;
	}
	else{

		g_enable_EmodemUdp_linkcount = 0;
		g_enable_EmodemUdp_link = false;

	       g_EmodemTcp_socket_link = true;
		
		at_config.net_config.link_num = 0;
	   	at_config.net_config.protocol = "TCP";
		
	    if(IP_addr[0] == '\0'){
	      LOG("ERROR: IP_addr is empty.\n");
	      RESET_GLOBAL_VAR;
	      return;
	    }

	 
	   if ((tcp_server_port > 0) && (tcp_server_port <= 9999) && (g_enable_TCP_IP01_linkcount < 3) && (g_enable_TCP_IP02_linkcount == 0)) {

		//tcp_IP_addr[0] = 0x39;
		strncpy(IP_addr, tcp_IP_addr, 15);
	       server_port = tcp_server_port;
		g_enable_EmodemTCP_link = true;
		LOG("Debug: Link server via IP01. \n");

		   
	   }else if((tcp_server_port_2 > 0) && (tcp_server_port_2 <= 9999) && (g_enable_TCP_IP01_linkcount >= 3) && (g_enable_TCP_IP02_linkcount < 3) && (g_EmodemUdp_link_server_state != true)){

	   	//tcp_IP_addr_2[0] = 0x39;
		strncpy(IP_addr, tcp_IP_addr_2, 15);
	       server_port = tcp_server_port_2;
		g_enable_EmodemTCP_link = true;
		
		LOG("Debug: Link server via IP02. \n");

	   }
	   else{

		g_enable_EmodemTCP_link = true;
		g_EmodemUdp_link_server_state = false;
		g_enable_TCP_IP01_linkcount = 0;
		g_enable_TCP_IP02_linkcount = 0;

		g_enable_EmodemUdp_linkcount = 0;
	       g_enable_EmodemUdp_link = true;
		   
		at_config.net_config.link_num = 1;
		at_config.net_config.protocol = "UDP";
		at_config.net_config.server_port = 5000;

		g_EmodemUdp_socket_link = true;

		 cmd_len = sprintf(whole_cmd, format_udp_cipopen, cmd, 
                      at_config.net_config.link_num,
                      at_config.net_config.protocol,
                      at_config.net_config.server_port);

	
		LOG("Debug: Restart UDP connect server after TCP link fail  \n");

		break;
		 

	   }

	   LOG("Debug: IP_addr --> \"%s\"  \n", IP_addr);
	   LOG("Debug: server_port = %d.\n",server_port);
	    
	    
	    strncpy(at_config.net_config.server_IP, IP_addr, 15);
	    at_config.net_config.server_IP[15] = '\0';
	    if(server_port > 65535 || server_port < 0){
	      LOG("ERROR: server_port is incorrect.\n");
	      RESET_GLOBAL_VAR;
	      return;
	    }
	    at_config.net_config.server_port = server_port;

	    cmd_len = sprintf(whole_cmd, format, cmd, 
	                      at_config.net_config.link_num,
	                      at_config.net_config.protocol,
	                      at_config.net_config.server_IP,
	                      at_config.net_config.server_port);

	}


#else

    at_config.net_config.link_num = 0;
    at_config.net_config.protocol = "TCP";
    if(IP_addr[0] == '\0'){
      LOG("ERROR: IP_addr is empty.\n");
      RESET_GLOBAL_VAR;
      return;
    }
	
    strncpy(at_config.net_config.server_IP, IP_addr, 15);
    at_config.net_config.server_IP[15] = '\0';
    if(server_port > 65535 || server_port < 0){
      LOG("ERROR: server_port is incorrect.\n");
      RESET_GLOBAL_VAR;
      return;
    }
    at_config.net_config.server_port = server_port;

    cmd_len = sprintf(whole_cmd, format, cmd, 
                      at_config.net_config.link_num,
                      at_config.net_config.protocol,
                      at_config.net_config.server_IP,
                      at_config.net_config.server_port);
#endif
    break;
  } // case ACQ_DATA_STATE_CIPOPEN

  case ACQ_DATA_STATE_CIPSEND_1:
  {
    char format[] = "%s=%d,%d";
    char format_udp_cipsend1[] = "%s=%d,%d,\"%s\",%d";

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

 LOG("Debug: b_enable_softwarecard_auth = %d .\n",b_enable_softwarecard_auth);
 LOG("Debug: b_enable_softwarecard_auth = %d .\n",b_enable_softwarecard_send);

	if (b_enable_softwarecard_auth == true){

		rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];
		whole_cmd[0] = 'A';
		whole_cmd[1] = 'T';
		cmd_len = 2;

		break;
		
	}


      


#endif


// 20160928 by jackli	 
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

       LOG("Debug: g_enable_EmodemUdp_link = %d .\n",g_enable_EmodemUdp_link);
       LOG("Debug: g_enable_EmodemUdp_linkcount = %d .\n",g_enable_EmodemUdp_linkcount);

rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

	if ((g_enable_EmodemUdp_link == true) && (g_enable_EmodemUdp_linkcount < 12)){
		
		if ((g_enable_EmodemUdp_linkcount > 2) && (g_enable_EmodemUdp_linkcount < 6)){

			LOG("Debug: UDP default addr 1.\n");
			strncpy(Udp_IP_addr, Udp_IP_addr_default1, 15);
			udp_server_port = udp_server_port_default1;
			
		}else if ((g_enable_EmodemUdp_linkcount >= 6) && (g_enable_EmodemUdp_linkcount < 9)){

			LOG("Debug: UDP default addr 2.\n");
			strncpy(Udp_IP_addr, Udp_IP_addr_default2, 15);
			udp_server_port = udp_server_port_default2;
		}else if ((g_enable_EmodemUdp_linkcount >= 9) && (g_enable_EmodemUdp_linkcount < 12)){

			LOG("Debug: UDP default addr 3.\n");
			strncpy(Udp_IP_addr, Udp_IP_addr_default3, 15);
			udp_server_port = udp_server_port_default3;
		}
		
		at_config.net_config.link_num = 1;
		at_config.net_config.server_port = udp_server_port;
	       strncpy(at_config.net_config.server_IP, Udp_IP_addr, 15);
	   	at_config.net_config.server_IP[15] = '\0';

		p_req_cfg->size = 29;
		
		cmd_len = sprintf(whole_cmd, format_udp_cipsend1, cmd, 
              	 at_config.net_config.link_num,
             		p_req_cfg->size,
             		 at_config.net_config.server_IP,
              	 at_config.net_config.server_port);

		 LOG("Debug: CIPSEND cmd_len = 0x%02x.\n",cmd_len);
		// LOG("Debug:   whole_cmd --> \"%s\"  \n", whole_cmd);

	}else{

	g_enable_EmodemUdp_linkcount = 0;
	g_enable_EmodemUdp_link = false;
	
	 rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

		#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
		    if(REQ_TYPE_STATUS == proto_get_req_cfg_data_type(p_req_cfg))
		    {
		      StatInfReqNum--;
		      if(StatInfReqNum > 0){
		        LOG("NOTICE: This Status info is out of date (StatInfReqNum: %d). Now, dump this status info req.\n",
		                  StatInfReqNum);

#ifdef FEATURE_ENABLE_4G_OK_F0		

			 LOG("Debug: need to suspend mdmEmodem.\n");
			
			 pthread_mutex_lock(&evtloop_mtx);
      			 notify_EvtLoop(0x03);
      			 pthread_mutex_unlock(&evtloop_mtx);
#endif			 
		        set_at_evt_waiting();
				
		        return;

			

				
		      }
		    }
		#endif

		 cmd_len = sprintf(whole_cmd, format, cmd, at_config.net_config.link_num, p_req_cfg->size);

	}

#else
	
    		rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

	#ifdef FEATURE_ENHANCED_STATUS_INFO_UPLOAD
		    if(REQ_TYPE_STATUS == proto_get_req_cfg_data_type(p_req_cfg))
		    {
		      StatInfReqNum--;
		      if(StatInfReqNum > 0){
		        LOG("NOTICE: This Status info is out of date (StatInfReqNum: %d). Now, dump this status info req.\n",
		                  StatInfReqNum);
		        set_at_evt_waiting();
		        return;
		      }
		    }
	#endif

 		cmd_len = sprintf(whole_cmd, format, cmd, at_config.net_config.link_num, p_req_cfg->size);

#endif

   
LOG("Debug:   whole_cmd --> \"%s\"  \n", whole_cmd);

	break;
  } // case ACQ_DATA_STATE_CIPSEND_1

  case ACQ_DATA_STATE_CIPSEND_2:
  {
    rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

    if(p_req_cfg->data == NULL){
      LOG("ERROR: The member \"data\" of req config struct is empty!\n");
      RESET_GLOBAL_VAR;
      return;
    }

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

 LOG("Debug: b_enable_softwarecard_auth = %d .\n",b_enable_softwarecard_auth);
 LOG("Debug: b_enable_softwarecard_send = %d .\n",b_enable_softwarecard_send);


	if (b_enable_softwarecard_auth == true){

		whole_cmd[0] = 'A';
		whole_cmd[1] = 'T';
		cmd_len = 2;

		ret = writer(at_config.serial_fd, whole_cmd, cmd_len);
		  if(ret != 1){
		    LOG("ERROR: writer() error. acq_data_state=%d.\n", (int)acq_data_state);
		    RESET_GLOBAL_VAR;
		    return;
		  }

	 	INIT_ATBUF;
      		set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      		add_evt (&at_config.AT_event, 'r');

#ifdef FEATURE_ENABLE_TIMER_SYSTEM

#ifdef FEATURE_ENABLE_ID_AUTH_RSP_TIMER
      if(flow_info.flow_state == FLOW_STATE_ID_AUTH){
        clr_idle_timer();
        clr_pdp_check_act_routine_timer();
        add_timer_ID_auth_rsp_timeout();
      }
#endif //FEATURE_ENABLE_ID_AUTH_RSP_TIMER

#ifdef FEATURE_ENABLE_REMOTE_USIM_DATA_RSP_TIMER
	      if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ){
	        clr_idle_timer();
	        clr_pdp_check_act_routine_timer();
	        add_timer_remote_USIM_data_rsp_timeout();
	      }
#endif //FEATURE_ENABLE_REMOTE_USIM_DATA_RSP_TIMER

#endif // FEATURE_ENABLE_TIMER_SYSTEM

		return;
		
	}


      


#endif


#ifdef FEATURE_ENABLE_W_ACQDATAHDLR_DISPATCHED_DATA_LOG
    {
				#ifndef FEATURE_DEBUG_QXDM
					#ifndef FEATURE_DEBUG_LOG_FILE
						int i = 0;
						LOG("App -> server: ");
						for(; i<p_req_cfg->size; i++)
							LOG2("%02x ", get_byte_of_void_mem(p_req_cfg->data, i) );
						LOG2("\n");
					#else
						int i = 1;
						
						pthread_mutex_lock(&log_file_mtx);
						LOG2("App -> server: \n");
						for(; i<=p_req_cfg->size; i++){
							if(i%20 == 1)
								LOG4(" %02x", get_byte_of_void_mem(p_req_cfg->data, i-1) );
							else
								LOG3(" %02x", get_byte_of_void_mem(p_req_cfg->data, i-1) );
							
							if(i%20 == 0) //Default val of i is 1
								LOG3("\n");
						}
						if((i-1)%20 != 0) // Avoid print redundant line break
							LOG3("\n");
						pthread_mutex_unlock(&log_file_mtx);
					#endif /*FEATURE_DEBUG_LOG_FILE*/
				#else
					int i = 0;
					int data_len = p_req_cfg->size;
					int buf_len = 3*data_len+1;
					char buf[buf_len];
				
					memset(buf, 0, buf_len);
					for(; i<data_len; i++){
						sprintf(buf+i*3, "%02x ", get_byte_of_void_mem(p_req_cfg->data, i) );
					}
					LOG("App -> server: ");
					#ifdef FEATURE_ENABLE_FMT_PRINT_DATA_LOG
						print_fmt_data(buf);
					#else
						LOG2("  %s", buf);
					#endif
				#endif
			}
#endif // FEATURE_ENABLE_W_ACQDATAHDLR_DISPATCHED_DATA_LOG

      ret = writer(at_config.serial_fd, p_req_cfg->data, p_req_cfg->size);
      if(ret != 1){
        LOG("ERROR: writer() error. acq_data_state=%d.\n", (int)acq_data_state);
        RESET_GLOBAL_VAR;
        return;
      }

      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');

#ifdef FEATURE_ENABLE_TIMER_SYSTEM

#ifdef FEATURE_ENABLE_ID_AUTH_RSP_TIMER
      if(flow_info.flow_state == FLOW_STATE_ID_AUTH){
        clr_idle_timer();
        clr_pdp_check_act_routine_timer();
        add_timer_ID_auth_rsp_timeout();
      }
#endif //FEATURE_ENABLE_ID_AUTH_RSP_TIMER

#ifdef FEATURE_ENABLE_REMOTE_USIM_DATA_RSP_TIMER
      if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ){
        clr_idle_timer();
        clr_pdp_check_act_routine_timer();
        add_timer_remote_USIM_data_rsp_timeout();
      }
#endif //FEATURE_ENABLE_REMOTE_USIM_DATA_RSP_TIMER

#endif // FEATURE_ENABLE_TIMER_SYSTEM

    return;
  } // case ACQ_DATA_STATE_CIPSEND_2

  case ACQ_DATA_STATE_NETOPEN:
  case ACQ_DATA_STATE_CGSN:
  case ACQ_DATA_STATE_COPS_1:
  case ACQ_DATA_STATE_COPS_2:
  case ACQ_DATA_STATE_CREG_1:
  case ACQ_DATA_STATE_CREG_2:
  case ACQ_DATA_STATE_CREG_3:
#ifdef FEATURE_ENABLE_CATR
  case ACQ_DATA_STATE_CATR_1:
  case ACQ_DATA_STATE_CATR_2:
#endif
  {
    cmd_len = sprintf(whole_cmd, "%s", cmd);
    break;
  }
#ifdef  FEATURE_ENABLE_CPOL_CONFIG
  case ACQ_DATA_STATE_COPS_3:

	  if (bcpolatIsSenddefault == false)
	   {
		bcpolatIsSenddefault = true;

		if (Atcommand_CoplByMCCMNC(MdmEmodemMCCMNC,atCmdCpolbuf,&cmd_len) != -1)
		{
	
			memcpy(whole_cmd,&atCmdCpolbuf[0],cmd_len);
		}
		else
		{
			
			whole_cmd[0] = 'A';
			whole_cmd[1] = 'T';
			cmd_len = 2;
		}
	   }
	  else
	  {
		whole_cmd[0] = 'A';
		whole_cmd[1] = 'T';
		cmd_len = 2;
	  }

	  break;


	  case ACQ_DATA_STATE_COPS_4:

		 LOG("Debug:COPS4 setting\n");
		 if ((bgetAtCmdH == false) || (getcpolAtCommandNumber != 0))	
		 {
		 	bgetAtCmdH = true;
			if (ConfCPOLByCountryType(MdmEmodemCountryType,atCmdCpolbuf,&cmd_len,&getcpolAtnumber) != -1)
			{
				LOG("Debug: getcpolAtCommandNumber = %d\n",getcpolAtCommandNumber);
				LOG("Debug: getcpolAtnumber = %d\n",getcpolAtnumber);
				memcpy(whole_cmd,&atCmdCpolbuf[0],cmd_len);
				if(getcpolAtCommandNumber > 0)
				{
					getcpolAtnumber++;
				}
				
			}
			else
			{
				whole_cmd[0] = 'A';
				whole_cmd[1] = 'T';
				cmd_len = 2;
			}
		 }
		 else
		{
				whole_cmd[0] = 'A';
				whole_cmd[1] = 'T';
				cmd_len = 2;
		}
			
			
		break;
#endif

#ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG
   case ACQ_DATA_STATE_CCLK:
	 cmd_len = sprintf(whole_cmd, "%s", cmd);
   break;
   
 #endif


 #ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
  
   case ACQ_DATA_STATE_CIPOPEN_REQ:



		cmd_len = sprintf(whole_cmd, "%s", cmd);
		  break;
   case ACQ_DATA_STATE_CIPCLOSE:

	
		memset(whole_cmd, 0, sizeof(whole_cmd));

		
              LOG("Debug: g_isEmodemcipopen :  %d\" \n", g_isEmodemcipopen_vaid);
		LOG("Debug:g_EmodemUdpTcp_socket_linkCount : %d.\n", g_EmodemUdpTcp_socket_linkCount);
		LOG("Debug:g_EmodemTcp_socket_link : %d.\n", g_EmodemTcp_socket_link);
		LOG("Debug:g_EmodemUdp_socket_link : %d.\n", g_EmodemUdp_socket_link);
		LOG("Debug: g_isEmodemipclose_report = %d\n",g_isEmodemipclose_report);
		
		 if( (g_EmodemTcp_socket_link == true) && (g_isEmodemcipopen_vaid == 1)){
			g_EmodemTcp_socket_link = false;

			if (g_isEmodemipclose_report){

				g_isEmodemipclose_report = false;
				whole_cmd[0] = 'A';
				whole_cmd[1] = 'T';
				cmd_len = 2;
				g_isat_command_AT = true;
			}else{

					whole_cmd[0] = 'A';
					whole_cmd[1] = 'T';
					whole_cmd[2] = '+';
					whole_cmd[3] = 'C';
					whole_cmd[4] = 'I';
					whole_cmd[5] = 'P';
					whole_cmd[6] = 'C';
					whole_cmd[7] = 'L';
					whole_cmd[8] = 'O';
					whole_cmd[9] = 'S';
					whole_cmd[10] = 'E';
					whole_cmd[11] = '=';
					whole_cmd[12] = '0';

					cmd_len = 13;
					g_isat_command_AT = false;

			}
			
			
			
		 }
		 else if ((g_EmodemUdp_socket_link == true) && (g_isEmodemcipopen_vaid == 2)){
		 
			g_EmodemUdp_socket_link = false;
			
			whole_cmd[0] = 'A';
			whole_cmd[1] = 'T';
			whole_cmd[2] = '+';
			whole_cmd[3] = 'C';
			whole_cmd[4] = 'I';
			whole_cmd[5] = 'P';
			whole_cmd[6] = 'C';
			whole_cmd[7] = 'L';
			whole_cmd[8] = 'O';
			whole_cmd[9] = 'S';
			whole_cmd[10] = 'E';
			whole_cmd[11] = '=';
			whole_cmd[12] = '1';

			cmd_len = 13;
			g_isat_command_AT = false;
			

		 }
		 else{
		 	
			whole_cmd[0] = 'A';
			whole_cmd[1] = 'T';
			cmd_len = 2;
			g_isat_command_AT = true;

		 }
		
  		break;
  #endif

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
  case ACQ_DATA_STATE_CUSTOM_AT_CMD:
  {
    if(proc_msg_at_config.cmd_str[0] == '\0'){
      LOG("ERROR: Nothing found in proc_msg_at_config.cmd_str.\n");
      RESET_GLOBAL_VAR;
      return;
    }

    cmd_len = sprintf(whole_cmd, "%s", proc_msg_at_config.cmd_str);

    break;
  }
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

  default:
  {
    LOG("ERROR: Wrong acq_data_state found in w_acqDataHdlr(). acq_data_state=%d.\n", acq_data_state);
    RESET_GLOBAL_VAR;
    return;
  }
  }//switch end
  
  LOG("   whole_cmd --> \"%s\"  \n", whole_cmd);
  ret = writer(at_config.serial_fd, whole_cmd, cmd_len);
  if(ret != 1){
    LOG("ERROR: writer() error. acq_data_state=%d.\n", (int)acq_data_state);
    RESET_GLOBAL_VAR;
    return;
  }
  
  INIT_ATBUF;
  set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
  add_evt (&at_config.AT_event, 'r');
  return;
}

void r_acqDataHdlr(void *userdata)
{
  int ret;
  char *ptr = NULL;
  char *ptr_cip = NULL;
  rsmp_transmit_buffer_s_type *p_req_cfg = &g_rsmp_transmit_buffer[THD_IDX_EVTLOOP];

#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

  char *P_cclk1 = NULL;
  char cclkbuf[20];
  unsigned char cclk_i = 0;
  unsigned char cclk_m = 0;
  char exp[50] = "";



#endif
  
  if(at_config.serial_fd < 0){
    LOG("ERROR: at_config.serial_fd < 0!\n");
    return;
  }

  ACQ_ACQ_DATA_STATE_MTX;

  if(ACQ_DATA_STATE_RECVING != acq_data_state)
  {
    isIdAuthRspTimerExpired = false;
    isRemoteUsimDataRspTimerExpired = false;
    isPwrDwnNotificationTimerExpired = false; // Actually, not need to reset
  }
  
  if(ACQ_DATA_STATE_RECVING != acq_data_state &&
      ACQ_DATA_STATE_CIPSEND_2 != acq_data_state)
  {
    isApduRspExtTimerExpired = false;
  }

  switch(acq_data_state)
  {
  case ACQ_DATA_STATE_IDLE:
  case ACQ_DATA_STATE_NETOPEN:
  {
    if(ACQ_DATA_STATE_IDLE == acq_data_state)
    {
      LOG("acq_data_state == ACQ_DATA_STATE_IDLE.\n");
    }else{
      LOG("acq_data_state == ACQ_DATA_STATE_NETOPEN.\n");
    }
   
__ACQ_DATA_STATE_NETOPEN_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
    if(ret == 0){
				LOG("ERROR: reader() error.\n");
				RESET_GLOBAL_VAR;
				break;
			}else if(ret == 2){
				if(*ATBufCur == '\0'){
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt(&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf.\n"); \
					goto __ACQ_DATA_STATE_NETOPEN_READER_AGAIN__;
				}
			}
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
			else if(ret == 3){
				ACQ_CHANNEL_LOCK_RD;
				if(ChannelNum != -1 && ChannelNum != 4){
					LOG("ERROR: reader() returns %d. Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ret, ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        msq_send_qlinkdatanode_run_stat_ind(16);
#endif

      //	When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
      //that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
      LOG("Retry to check signal...\n");
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
#endif // FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == 8)
    {
      //There is a str "ERROR" followed

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(13);
      isPdpNotActNotified = false; // Reset it
#endif

      isWaitRspError = true;
      error_flag = 0x03;
			if(*ATBufCur == '\0'){
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf. Continue to read more...\n");
					goto __ACQ_DATA_STATE_NETOPEN_READER_AGAIN__;
				}
			}else if(ret == 10){
				//read() return -1, and errno equals 11.
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				break;
			}else if(ret == 14){
				if(isWaitRspError){
					isWaitRspError = false;
					if(error_flag == 0x03){
						error_flag = 0x00; //Reset
						
						#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
							msq_send_qlinkdatanode_run_stat_ind(13);
							isPdpNotActNotified = false; // Reset it
						#endif
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
					}else{ // Including error_flag = 0x01 or 0x02
						LOG("ERROR: Wrong error_flag (0x%02x) found!\n", error_flag); \
						RESET_GLOBAL_VAR;
						break;
					}
				} // if(isWaitRspError)
			} // if(ret == 14)
			
    if(strcmp(ptr, "ERROR") == 0){

				#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
					msq_send_qlinkdatanode_run_stat_ind(16);
				#endif
				
				LOG("ERROR: Unexpected rsp \"ERROR\" found! Retry to check signal...\n");
				acq_data_state = ACQ_DATA_STATE_IDLE;
				chk_net_state = CHK_NET_STATE_CSQ;
				w_checkNetHdlr();
				break;
			}
			
			if(ret == 4){
				if(*ATBufCur == '\0'){
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf. Continue to read more...\n");
					goto __ACQ_DATA_STATE_NETOPEN_READER_AGAIN__;
				}
			}else if(ret == 1){
				int result = process_NETOPEN(ptr);
				
				if(isWaitToPwrDwn){
					LOG("acq_data_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
							acq_data_state, isWaitToPwrDwn);
					isWaitToPwrDwn = false;
					
					if(isTermRcvrNow){
						isTermRcvrNow = false;
					}
					
					ACQ_CHANNEL_LOCK_WR;
					if(isProcOfInitApp){
						//Situation Presentation:
						//Not send id auth msg yet, so directly shut down mdmEmodem.
	 					if(ChannelNum == -1){
							RLS_CHANNEL_LOCK; // Add by rjf at 20151015
							
							clr_power_down_notification_timer();
							
							//Not update ChannelNum
							
							set_at_evt_waiting();
							
							isMdmEmodemRunning = false;
							#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
								mdmEmodem_shutdown();
							#endif
							
							LOG("Send msg to notify other processes.\n");
							msq_send_pd_rsp();
							break;
						}else{
							LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ChannelNum, acq_data_state);
							RESET_GLOBAL_VAR;
							break;
						}
					}else{
						if(4 == ChannelNum || -1 == ChannelNum){
							if(ChannelNum == 4){
								isMifiConnected = false;
								ChannelNum = -1;
							}
							
							isNeedToSendOutReq0x65 = true;
							//Check result of cmd "AT+NETOPEN" below.
						}else{
							LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
									ChannelNum, acq_data_state);
							RESET_GLOBAL_VAR;
							break;
						}
					}
					RLS_CHANNEL_LOCK;
					
				}
				
				if(isTermRcvrNow){
					LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n",
							acq_data_state, isTermRcvrNow);
					isTermRcvrNow = false;
					
					//ChannelNum is updated when isTermRcvrNow is set.
					
					RcvrMdmEmodemConn = true; //Add by rjf at 20150914
					acq_data_state = ACQ_DATA_STATE_IDLE;
					suspend_state = SUSPEND_STATE_NETCLOSE;
					w_suspendMdmHdlr();
					break;
				}
				
				if(isWaitToSusp){
					LOG("acq_data_state=%d, isWaitToSusp=%d. Now, go to suspend mdmEmodem.\n",
							acq_data_state, isWaitToSusp);
					isWaitToSusp = false;
					
					ACQ_CHANNEL_LOCK_WR;
					if(4 == ChannelNum){
						ChannelNum = 2;
					}else if(-1 == ChannelNum){
						//Situation Presentation:
						//		If isWaitToSusp was set, and before executing the handler of "isWaitToSusp = 1", app has a need to terminate
						//	the suspension plan. So, ChannelNum might be -1 at that time.
						
						//Suspension is started when dialup completes.
						LOG("ERROR: Req to suspend mdmEmodem, but ChannelNum = -1!\n");
						RLS_CHANNEL_LOCK;
						goto __ACQ_DATA_STATE_NETOPEN_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
                                ChannelNum, acq_data_state);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;
					
				
          suspend_state = SUSPEND_STATE_NETCLOSE;
				
					w_suspendMdmHdlr();
					break;
				}
        
        //isWaitToRcvr is not applicable here 'cause it is set true when ChannelNum is 3 or 0.
__ACQ_DATA_STATE_NETOPEN_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__:

      //Handle the result of process_NETOPEN
      if(result == -1){
					LOG("ERROR: process_NETOPEN() error.\n");
					RESET_GLOBAL_VAR;
					break;
				}else if(result == -2){
					if(*ATBufCur == '\0'){
						LOG("Unexpected msg found. Continue to recv more...\n");
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						break;
					}else{
						LOG("There is still something unread in ATBuf. Continue to read...\n");
						goto __ACQ_DATA_STATE_NETOPEN_READER_AGAIN__;
					}
				}else if(result != 0){
					int sleep_time = 0; //Unit: sec


#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        if(!isPdpNotActNotified){
							msq_send_qlinkdatanode_run_stat_ind(12);
							isPdpNotActNotified = true;
				}
#endif

        //Add by rjf at 20150910 (The repeated attemps have ever caused "at+netopen" returns "ERROR".)
        //Add "1 == result" by rjf at 20151022 (Reduce repeated attemps.)
        switch(result){
        case 1:
          sleep_time = 3;break;
        case 10:
          sleep_time = 5;break;
        default:
          sleep_time = 0;break;
        }
        sleep(sleep_time);

					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(16);
					#endif
					
					LOG("Fail to activate PDP. Retry to check signal...\n");
					acq_data_state = ACQ_DATA_STATE_IDLE;
					chk_net_state = CHK_NET_STATE_CSQ;
					w_checkNetHdlr();
					break;
				}
			}else{
				LOG("ERROR: Wrong ret (%d) found!\n", ret);
				RESET_GLOBAL_VAR;
				break;
			}

			#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
				msq_send_qlinkdatanode_run_stat_ind(13);
				isPdpNotActNotified = false; // Reset it
			#endif


#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif

  } // case ACQ_DATA_STATE_NETOPEN

  case ACQ_DATA_STATE_CIPOPEN:
  {
    LOG("acq_data_state == ACQ_DATA_STATE_CIPOPEN.\n");

__ACQ_DATA_STATE_CIPOPEN_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);

    if(isWaitRspError == true && ret == 14){
      isWaitRspError = false;
      goto __HANDLE_CIPOPEN_4__;
    }
    if(ret == 0){
				LOG("ERROR: reader() error. ret=0.\n");
				RESET_GLOBAL_VAR;
				break;
		}else if(ret == 2){
			if(*ATBufCur == '\0'){
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				break;
			}else{
				LOG("acq_data_state=%d. There is still something unread in ATBuf.\n", acq_data_state);
				goto __ACQ_DATA_STATE_CIPOPEN_READER_AGAIN__;
			}
    }
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == 3)
    {
				ACQ_CHANNEL_LOCK_RD;
				if(ChannelNum != -1 && ChannelNum != 4){
					LOG("ERROR: reader() returns %d. Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ret, ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;

				#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
					msq_send_qlinkdatanode_run_stat_ind(16);
				#endif
				
				//Not care if the registration of mdm9215.
				//When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
				//that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
				LOG("Retry to check signal...\n");
				chk_net_state = CHK_NET_STATE_CSQ;
				w_checkNetHdlr();
				break;
    }
#endif // FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == 5)
    {
#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      if(!isServDisconnNotified){
        msq_send_qlinkdatanode_run_stat_ind(14);
        isServDisconnNotified = true;
      }
#endif

    ACQ_CHANNEL_LOCK_RD;
    if(ChannelNum != -1 && ChannelNum != 4){
      LOG("ERROR: ret=%d. Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ret, ChannelNum, acq_data_state);
      RESET_GLOBAL_VAR;
      break;
    }
    RLS_CHANNEL_LOCK;

 #ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
  }else if(ret == 9)
  {
    //The subsequent part of rsp, which is "ERROR", should be recved behind the formal part "+CIPOPEN: 0,4".
    if(isWaitToPwrDwn){
      goto __ACQ_DATA_STATE_CIPOPEN_CHECK_PWR_DWN__;
    }else{
      if( (*ATBufCur != '\0' && (strncmp("ERROR", ATBufCur, sizeof("ERROR")-1) != 0)) || \
        *ATBufCur == '\0' \
      ){
          LOG("The rsp \"ERROR\" behind \"+CIPOPEN: 0,4\" is missed.");
						isWaitRspError = true;
						//Maintain acq_data_state
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						break;
					}
					
					//CIPOPEN: 0,4 not count as conn built up.
__HANDLE_CIPOPEN_4__:
					ACQ_CHANNEL_LOCK_RD;
					if(-1 != ChannelNum && 4 != ChannelNum){
						LOG("ERROR: Wrong ChannelNum(%d) found when \"+CIPOPEN: 0,4\" reported.\n",
								ChannelNum);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;
					isServConnClosed = true;
					sleep(1); // Wait for closing socket completely.
					//Maintain ChannelNum
					suspend_state = SUSPEND_STATE_NETCLOSE;
					w_suspendMdmHdlr();
					break;
				}
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }else if(ret == 7 || ret == 8 || ret == 11 || ret == 12){
      LOG("ERROR: Unexpected ret(%d) found!acq_data_state=%d.\n", ret, acq_data_state);
      RESET_GLOBAL_VAR;
      break;
    }

    if(strncmp(ptr, "OK", 2) == 0){
      if(*ATBufCur == '\0'){
	  
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("acq_data_state=%d. There is still something unread in ATBuf.\n", acq_data_state);
        goto __ACQ_DATA_STATE_CIPOPEN_READER_AGAIN__;
      }
    }else if(strncmp(ptr, "ERROR", 5) == 0){
      LOG("Unexpected msg \"ERROR\" recved. Retry to check signal...\n");

      //Handle unexpected "AT+CIPOPEN..." unexpected rsp "ERROR"
      //Maintain original ChannelNum. It should be -1 or 4 usually.
      sleep(1);
      RcvrMdmEmodemConn = true;
      chk_net_state = CHK_NET_STATE_IDLE;
      acq_data_state = ACQ_DATA_STATE_IDLE;
      suspend_state = SUSPEND_STATE_NETCLOSE;
      w_suspendMdmHdlr();
      break;
    }else if(ptr == NULL){
      //Not possible to reach here.
      LOG("ERROR: Unexpected error found. ptr=null.\n");
      RESET_GLOBAL_VAR;
      break;
    }

    ret = process_CIPOPEN(ptr);
    
__ACQ_DATA_STATE_CIPOPEN_CHECK_PWR_DWN__:
    if(isWaitToPwrDwn){
				LOG("acq_data_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
						acq_data_state, isWaitToPwrDwn);
				isWaitToPwrDwn = false;
				
				if(isTermRcvrNow){
					isTermRcvrNow = false;
				}
				
				ACQ_CHANNEL_LOCK_WR;
				if(isProcOfInitApp){
					//Situation Presentation:
					//	Not send id auth msg yet, so directly shut down mdmEmodem.
	 				if(ChannelNum == -1){
						if(isPwrDwnNotificationTimerExpired){
							isPwrDwnNotificationTimerExpired = false; // reset
							
							LOG("NOTICE: Clear power down notification timer in Timeouts_list.\n");
							clr_timeout_evt(&power_down_notification_timeout_evt);
						}
						clr_power_down_notification_timer();
						
						//Not update ChannelNum
						

						set_at_evt_waiting();
						
						isMdmEmodemRunning = false;
            
						#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
							mdmEmodem_shutdown();
						#endif
						
						LOG("Send msg to notify other processes.\n");
						msq_send_pd_rsp();
						break;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
								ChannelNum, acq_data_state);
						RESET_GLOBAL_VAR;
						break;
					}
				}else{
					LOG("ChannelNum=%d, process_CIPOPEN() return %d.\n",
							ChannelNum, ret);
					if(4 == ChannelNum || -1 == ChannelNum){
						if(ChannelNum == 4){
							isMifiConnected = false;
							ChannelNum = -1;
						}
						
						if(ret != 0){
							isNeedToSendOutReq0x65 = true;
							//Go to the process of re-connecting to server below.
						}else{
							ChannelNum = 0; // All ChannelNums has been changed to -1 as before.
							
							#ifdef FEATURE_ENABLE_MSGQ
								msq_send_online_ind();
							#endif
							
							msq_send_pd_msg_to_srv();
							break;
						}
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
								ChannelNum, acq_data_state);
						RESET_GLOBAL_VAR;
						break;
					}
				}
				RLS_CHANNEL_LOCK;
    }

    if(isTermRcvrNow){
				LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n",
						acq_data_state, isTermRcvrNow);
				isTermRcvrNow = false;
				
				//ChannelNum is updated when isTermRcvrNow is set.
				
				RcvrMdmEmodemConn = true; //Add by rjf at 20150914
				acq_data_state = ACQ_DATA_STATE_IDLE;
				suspend_state = SUSPEND_STATE_CIPCLOSE;
				w_suspendMdmHdlr();
				break;
    }


    if(isWaitToSusp)//Fixed by rjf at 201508081714
    {
				LOG("acq_data_state=%d, isWaitToSusp=%d. Now, go to suspend mdmEmodem.\n",
						acq_data_state, isWaitToSusp);
				isWaitToSusp = false;
				
				ACQ_CHANNEL_LOCK_WR;
				if(4 == ChannelNum){
					ChannelNum = 2;
				}else if(-1 == ChannelNum){
					//Situation Presentation:
					//		If isWaitToSusp was set, and before executing the handler of "isWaitToSusp = 1", app has a need to terminate
					//	the suspension plan. So, ChannelNum might be -1 at that time.
					
					//Suspension is started when dialup completes.
					LOG("ERROR: Req to suspend mdmEmodem, but ChannelNum = -1!\n");
					RLS_CHANNEL_LOCK;
					goto __ACQ_DATA_STATE_CIPOPEN_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__;
				}else{
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ChannelNum, acq_data_state);
	#ifdef FEATURE_ENABLE_REBOOT_DEBUG
			//system("reboot");
	yy_popen_call("reboot -f");
			
	#endif
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				suspend_state = SUSPEND_STATE_CIPCLOSE;
				w_suspendMdmHdlr();
				break;
    }
      
  //isWaitToRcvr is not applicable here 'cause it is set true when ChannelNum is 3 or 0.
__ACQ_DATA_STATE_CIPOPEN_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__:

    //Handle the result of process_CIPOPEN
    //Note: 
    //There will be no possibility that ret equals 4. That means "+CIPOPEN: 0,4" will be handled right after reader().
    if(ret == 0)
    {
      CIPOPENCount = 0;
    }else if(ret == 1)
    {
				#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
					if(!isServDisconnNotified){
						msq_send_qlinkdatanode_run_stat_ind(14);
						isServDisconnNotified = true;
					}
				#endif
				
				#ifndef FEATURE_KEEP_EXECUTING_CIPOPEN_UNTIL_SUCCESS
					LOG("Fail to connect to TCP server!\n");
					if(CIPOPENCount < CIPOPEN_REPETITION_TIMES){
						sleep(1);
						CIPOPENCount++;
					}else{
						int i = 0;
						int time = 1;
						
						for(; i<CIPOPENCount-CIPOPEN_REPETITION_TIMES+1; i++, time*=2);
						sleep(time);
						CIPOPENCount++;
					}
					LOG("Retry \"AT+CIPOPEN\"...\n");
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
				#else
					LOG("Fail to connect to TCP server. Retry to check signal...\n");
					//Maintain original ChannelNum.
					RcvrMdmEmodemConn = true;
					acq_data_state = ACQ_DATA_STATE_IDLE;
					suspend_state = SUSPEND_STATE_NETCLOSE;
					w_suspendMdmHdlr();
					break;
				#endif
    }else if(ret == 4){
      LOG("ERROR: It should not reach here! Msg: %s.\n", ptr);
      RESET_GLOBAL_VAR;
      break;
    }else if(ret == 5)
    {
      LOG("Fail to create socket. Retry \"AT+CIPOPEN\"...\n");

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      if(!isServDisconnNotified){
        msq_send_qlinkdatanode_run_stat_ind(14);
        isServDisconnNotified = true;
      }
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      sleep(2);
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
    }else if(ret == 10){
      LOG("NOTICE: Cmd times out. Retry to check signal...\n");

				#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
					if(!isServDisconnNotified){
						msq_send_qlinkdatanode_run_stat_ind(14);
						isServDisconnNotified = true;
					}
				#endif
				
				//Maintain original ChannelNum.
				RcvrMdmEmodemConn = true;
				acq_data_state = ACQ_DATA_STATE_IDLE;
				suspend_state = SUSPEND_STATE_NETCLOSE;
				w_suspendMdmHdlr();
				break;
    }else if(ret == -2)
    {
				if(*ATBufCur == '\0'){
					LOG("Unexpected msg found. Continue to recv more...\n");
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf. Continue to read...\n");
					LOG("acq_data_state:: =.%d \n",acq_data_state);
					goto __ACQ_DATA_STATE_CIPOPEN_READER_AGAIN__;
				}
    }else
    {
      //Not possible that ret equals -1
      LOG("ERROR: Unexpected msg recved. Msg: %s.\n", ptr);

      //Handle unexpected "AT+CIPOPEN..." unexpected rsp "+CIPOPEN: X"
      //Maintain original ChannelNum.
      sleep(1);
      RcvrMdmEmodemConn = true;
      chk_net_state = CHK_NET_STATE_IDLE;
      acq_data_state = ACQ_DATA_STATE_IDLE;
      suspend_state = SUSPEND_STATE_CIPCLOSE;
      w_suspendMdmHdlr();
      break;
    }

//__SERV_CONN_ALREADY__:

#ifdef FEATURE_ENABLE_MSGQ
    msq_send_online_ind();
#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
    msq_send_qlinkdatanode_run_stat_ind(15);
    isServDisconnNotified = false; // Reset it
#endif
#endif // FEATURE_ENABLE_MSGQ

LOG("isProcOfInitApp = %d \n ", isProcOfInitApp);

#ifdef FEATURE_ENABLE_EmodemUDP_LINK

LOG("g_enable_EmodemUdp_link =%d \n ", g_enable_EmodemUdp_link);
    //Initiative mode
    if((isProcOfInitApp == true) ||(g_enable_EmodemUdp_link))

#else
  if(isProcOfInitApp == true) 

#endif
    {
      int length;

      isProcOfInitApp = false;
      set_ChannelNum(0);

      LOG("isProcOfInitApp = 1 and server connected. flow_state is changed to %d from %d.\n", FLOW_STATE_ID_AUTH, flow_info.flow_state);
      proto_update_flow_state(FLOW_STATE_ID_AUTH);
      reset_rsmp_transmit_buffer(p_req_cfg);
	  
// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

	if (g_enable_EmodemUdp_link){

		p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_Emodem_UDP, &sim5360_dev_info, NULL);

	}
	else{
			p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_ID, &sim5360_dev_info, NULL);
		}


#else

      p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_ID, &sim5360_dev_info, NULL);

#endif

	 p_req_cfg->size = length;

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

	  if (g_enable_EmodemUdp_link){

		 ret = reader(at_config.serial_fd, &ptr);

		 LOG("Debug: Read UDP string is OK  ret =.%d )...\n", ret);
	  }

 #endif

        GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
    }else 
    { // isProcOfInitApp == false
      //NOTICE:
      //ChannelNum hasn't been updated yet!(Although AT+CIPOPEN already succeed.)
      //So check_ChannelNum() below is valid.

      if(isNeedToSendOutReq0x65)
      {
        LOG("acq_data_state=2, isNeedToSendOutReq0x65=1.\n");
        isNeedToSendOutReq0x65 = false;

        set_ChannelNum(0);

        //No need to acq acq_data_state_mtx
        msq_send_pd_msg_to_srv();
        break;
      }

#ifdef FEATURE_ENABLE_APDU_RSP_TIMEOUT_SYS
      if(isRegOffForPoorSig)
      {
        LOG("acq_data_state=%d, isRegOffForPoorSig=%d.\n", acq_data_state, isRegOffForPoorSig);
        //Note: isRegOffForPoorSig will be updated later.

        if(isWaitToSusp){
          LOG("NOTICE: isWaitToSusp=1, but isRegOffForPoorSig=1 too. So cancel the suspension now.\n");
          isWaitToSusp = false;
        }

        set_ChannelNum(0); //Original ChannelNum should be -1.
        set_at_evt_waiting(); // Preparation for recving APDU req (from modem).

        pthread_mutex_lock(&pd_mtx);

        LOG("isUimPwrDwn=%d, isNeedToNotifyServCondVar=%d, isNeedToSendIDMsg=%d.\n", isUimPwrDwn, isNeedToNotifyServCondVar, isNeedToSendIDMsg);
        if(isUimPwrDwn){
__UPLOAD_STAT_INF_OR_SEND_OUT_ID_MSG__:
							if(isNeedToNotifyServCondVar){
								isNeedToNotifyServCondVar = false;
								
								status_info_upload_preparatory_work();
								
								pthread_mutex_unlock(&pd_mtx);
								
								proto_update_flow_state(FLOW_STATE_STAT_UPLOAD); // Add by rjf at 20151030
								GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
							}else if(isNeedToSendIDMsg){
								isNeedToSendIDMsg = false;
						
								isSendIdAfterAcqIMSI = true;
								proto_update_flow_state(FLOW_STATE_IMSI_ACQ);
								notify_Sender(QMISENDER_ACQ_IMSI);
								pthread_mutex_unlock(&pd_mtx);
								break; //NOTE: AT event is shut down for the time being.
							}else{
								//Situation Presentation:
								//	The app recved "+IPCLOSE: 0,2" when app has sent out status info and prepared to send id auth msg.
								LOG("isRegOffForPoorSig = 1, but isNeedToNotifyServCondVar = 0!\n");
								
								//Help to restore the wrong condition to right one.
								//isNeedToNotifyServCondVar = true;
								isNeedToSendIDMsg = true;
								goto __UPLOAD_STAT_INF_OR_SEND_OUT_ID_MSG__;
							}
					}else{
          LOG("NOTICE: acq_data_state=2 and isRegOffForPoorSig=1, but isUimPwrDwn=0!\n");

          PowerUpUsimAfterRecvingUrcRpt = true;
        }

        pthread_mutex_unlock(&pd_mtx);

        break;
      }
#endif

      if(check_ChannelNum(-1))
      {
        LOG("Server connected. Now, ChannelNum change to 0 from -1.\n");
        set_ChannelNum(0);

        if(!isNeedToNotifyServCondVar)
        {
          if(isNeedToSendIDMsg){
            // Situation Presentation:
            // ID auth rsp timer expired, and re-send ID auth msg.
            // NOTE: 
            // 1. AT event is shut down for the time being.
            // 2. IMSI is included in ID auth msg.
            isNeedToSendIDMsg = false;
            at_send_id_auth_msg(0x00);
            break;
          }

          if(isQueued)
          {
            if(1 == queue_check_if_node_valid()){
              if(1 == dequeue_and_send_req(p_req_cfg))
                break;
            }else{
              // Note: When queue_check_if_node_valid() is just for checking invalid APDU, this operation is valid.
              queue_clr_apdu_node();
            }
          }

          int check_rsp_timers_result = -1, check_reg_result = -1;
          
#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
          //isQueued is false, so not need to check the return value of set_at_evt_waiting().
          if(false == isNotNeedIdleTimer 
            && 0 == (check_rsp_timers_result = check_rsp_timers())
            && isRegOn // Add by rjf at 20150918
            && false == isSockListenerRecvPdpActMsg // Add by rjf at 20151014
          ){
            LOG("Set up idle timer for mdmEmodem.\n");
            add_timer_mdmEmodem_idle_timeout(0x00);
          }else{
            LOG("do not set up idle timer (acq_data_state: %d). "
                "check_rsp_timers_result=%d, "
                "check_reg_result=%d, "
                "isNotNeedIdleTimer=%d, "
                "isRegOn=%d, "
                "isSockListenerRecvPdpActMsg=%d.\n",
                acq_data_state,
                check_rsp_timers_result,
                check_reg_result,
                isNotNeedIdleTimer,
                isRegOn,
                isSockListenerRecvPdpActMsg
                );
          }
#endif // FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
              
          set_at_evt_waiting();
          break;
        }else
        {
          isNeedToNotifyServCondVar = false;

          status_info_upload_preparatory_work();
          proto_update_flow_state(FLOW_STATE_STAT_UPLOAD);
        }

        GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
      }else if(check_ChannelNum(4))
      {
        LOG("Server connected. Now, ChannelNum change to 3 from 4. isNeedToNotifyServCondVar=%d, isQueued=%d.\n", isNeedToNotifyServCondVar, isQueued);
        set_ChannelNum(3);

        if(isNeedToNotifyServCondVar){
          isNeedToNotifyServCondVar = false; // No ID auth, so reset it.

          status_info_upload_preparatory_work();
          proto_update_flow_state(FLOW_STATE_STAT_UPLOAD); // Add by rjf at 20151030
          GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
        }else{
          if(isQueued)
            {
							if(1 == queue_check_if_node_valid()){
								if(1 == dequeue_and_send_req(p_req_cfg))
									break;
							}else{
								// Note: When queue_check_if_node_valid() is just for checking invalid APDU, this operation is valid.
								queue_clr_apdu_node();
							}
						}
						
          int check_rsp_timers_result = -1, check_reg_result = -1;

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
         if(isYYDaemonNeedtoSendATCmd)
          {
            isYYDaemonNeedtoSendATCmd = false;
            proto_update_flow_state(FLOW_STATE_MDMEmodem_AT_CMD_REQ);
            acq_data_state = (ACQ_DATA_STATE_CUSTOM_AT_CMD);
            w_acqDataHdlr();
            break;
          }
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
							//isQueued is false, so not need to check the return value of set_at_evt_waiting().
							if(false == isNotNeedIdleTimer 
								&& 0 == (check_rsp_timers_result = check_rsp_timers())
								&& isRegOn // Add by rjf at 20150918
								&& false == isSockListenerRecvPdpActMsg // Add by rjf at 20151014
							){
								LOG("Set up idle timer for mdmEmodem.\n");
								add_timer_mdmEmodem_idle_timeout(0x00);
							}else{
								LOG("do not set up idle timer (acq_data_state: %d). "
										"check_rsp_timers_result=%d, "
										"check_reg_result=%d, "
										"isSockListenerRecvPdpActMsg=%d.\n",
										acq_data_state,
										check_rsp_timers_result,
										check_reg_result,
										isSockListenerRecvPdpActMsg
										);
							}
#endif // FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
						set_at_evt_waiting();
						break;
					} // isNeedToNotifyServCondVar == false
					
			}else
			{
        //ChannelNum == 0, 1, 2 or 4
        LOG("ERROR: ChannelNum=%d, but app recovered server connection (of mdmEmodem) just now!\n", get_ChannelNum());
        RESET_GLOBAL_VAR;
        break;
      }
    }// isProcOfInitApp == false
    //Not handle isWaitToRcvr here to reduce complexity. Because ChannelNum is set true just now.
    break;
  } // case ACQ_DATA_STATE_CIPOPEN

  case ACQ_DATA_STATE_CIPSEND_1:
  {
    LOG("acq_data_state == ACQ_DATA_STATE_CIPSEND_1.\n");

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

     if (b_enable_softwarecard_auth == true){

		 ret = reader(at_config.serial_fd, &ptr);

		 LOG("Debug: Read at string is OK  ret =.%d )...\n", ret);
		 LOG("Debug: Read at string is OK  ptr =.%s )...\n", ptr);

#ifdef CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR
      //Situation Presentation:
      //Once, "AT+CIPSEND=0,X" succeeds after "AT+CIPSEND=0,X" returns "+CIPSEND: 0,X,0" last time,
      //isCipsendFailedV01 should be reset false then.
      isCipsendFailedV01 = false; // Add by rjf at 20151016
#endif // CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR

    acq_data_state = ACQ_DATA_STATE_CIPSEND_2;
    w_acqDataHdlr();
    break;

		 
    	}
	 
#endif
	
__ACQ_DATA_STATE_CIPSEND_1_READER_EXT__:
    ret = reader_ext(at_config.serial_fd);
    if(ret == 0)
    {
      LOG("ERROR: reader_ext() error. acq_data_state=%d.\n", acq_data_state);
      RESET_GLOBAL_VAR;
      break;
    }else if(ret == 2 || ret == 5 || ret == 6 || ret == 7){ // Add "ret == 6" at 20150909
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf. \n");
	 LOG("acq_data_state = %d .\n", acq_data_state);
        goto __ACQ_DATA_STATE_CIPSEND_1_READER_EXT__;
      }
    }
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == 3)
    {
      //Assume rsp str "OK" is before this URC report. And no URC report of "AT+CIPSEND..." is followed.
      ACQ_CHANNEL_LOCK_RD;
      if(0 == ChannelNum){
        ChannelNum = -1;
      }else if(3 == ChannelNum){
        ChannelNum = 4;
      }else if(0 != ChannelNum && 3 != ChannelNum){
					LOG("ERROR: reader() returns %d. Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ret, ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;

        //Not care if registration of mdm9215.
        add_node(p_req_cfg, flow_info.flow_state);

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

        //When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
        //that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
        LOG("Retry to check signal...\n");
        chk_net_state = CHK_NET_STATE_CSQ;
        w_checkNetHdlr();
        break;
      }
#endif // FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
      else if(ret == 4 || ret == 8 || ret == 9 || ret == 10 || ret == 11)
      {
        //Note: When app send data by network link of mdmEmodem, ChannelNum must be 0 or 3.

        if(!isWaitForRptCIPERROR)
        {
          ACQ_CHANNEL_LOCK_WR;
          if(0 == ChannelNum)
          {
            ChannelNum = -1;
            
#ifdef FEATURE_ENABLE_MSGQ
            msq_send_offline_ind();
#endif // FEATURE_ENABLE_MSGQ

          }else if(3 == ChannelNum)
          {
            ChannelNum = 4;
            
#ifdef FEATURE_ENABLE_MSGQ
            msq_send_offline_ind();
#endif // FEATURE_ENABLE_MSGQ 

          }else if(1 == ChannelNum) //161616
          {
            set_at_evt_waiting();
          }
          else
          {
            LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ChannelNum, acq_data_state);
            RESET_GLOBAL_VAR;
            break;
          }
          RLS_CHANNEL_LOCK;
        }

        add_node(p_req_cfg, flow_info.flow_state);

        if(ret == 4)
        {
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
        }else if(ret == 8 || ret == 9)
        {
          isWaitForRptCIPERROR = true;
          isServConnClosed = true;
          sleep(1); // Wait for closing socket completely.

          //Wait for subsequent URC rpt
					if(*ATBufCur == '\0'){
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						break;
					}else{
						LOG("There is still something unread in ATBuf. \n");
						goto __ACQ_DATA_STATE_CIPSEND_1_READER_EXT__;
					}
				}else
				{ // ret == 10 or ret == 11
          isWaitForRptCIPERROR = false;

					if((*ATBufCur == '\0' || (*ATBufCur != '\0' && (strncmp("ERROR", ATBufCur, sizeof("ERROR")-1) != 0)))
						#ifdef CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR
							&& false == isCipsendFailedV01 // Add by rjf at 20151016
						#endif
					){
						LOG("The rsp \"ERROR\" behind \"+CIPERROR: %d\" is missed!\n", (ret==10)?4:2);
						isWaitRspError = true;
						switch(ret){
							case 10:
								error_flag = 0x02;break;
							case 11:
								error_flag = 0x01;break;
							default:
								break; // Not possible to reach here.
						}
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						break;
					}
            
#ifdef CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR
					else if((*ATBufCur == '\0' || (*ATBufCur != '\0' && (strncmp("ERROR", ATBufCur, sizeof("ERROR")-1) != 0)))
						&& true == isCipsendFailedV01
					){
						LOG("Rsp \"+CIPERROR: %d\" recved. Enter subsequent process.\n",
								(ret==10)?4:2);
						isCipsendFailedV01 = false;
					}
#endif // CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR

					//Rsp "ERROR" recved
					if(ret == 11){
						//add_node() is executed before.
						//ChannelNum is updated above.
						RcvrMdmEmodemConn = true; // Add by rjf at 20151016
						suspend_state = SUSPEND_STATE_NETCLOSE;
						w_suspendMdmHdlr();
						break;
					}else{ // ret == 10
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
					}
        }
      }else if(ret == 12)
      {
				if(isWaitRspError){
					isWaitRspError = false;
					if(error_flag == 0x01){
						error_flag = 0x00;
						
						ACQ_CHANNEL_LOCK_WR;
						if(3 == ChannelNum){
							ChannelNum = 4;
						}else if(0 == ChannelNum){
							ChannelNum = -1;
						}else{
							LOG("ERROR: acq_data_state=3, ret=12. Wrong ChannelNum (%d) found.\n", ChannelNum);
							RESET_GLOBAL_VAR;
							break;
						}
						RLS_CHANNEL_LOCK;
						suspend_state = SUSPEND_STATE_NETCLOSE;
						w_suspendMdmHdlr();
						break;
					}else if(error_flag == 0x02){
						error_flag = 0x00;
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
					}else{
						LOG("ERROR: Wrong error_flag (0x%02x) found!", error_flag);
						RESET_GLOBAL_VAR;
						break;
					}
				}else{
          LOG("ERROR: Unexpected rsp \"ERROR\" found!\n");

          //Handle unexpected "AT+CIPSEND..." (1) rsp "ERROR"
          sleep(1);

          RcvrMdmEmodemConn = true;
          //Situation Presentation:
          //When acq_data_state equals ACQ_DATA_STATE_CIPSEND_1 and recover mdmEmodem after recving rsp "ERROR",
          //ChannelNum should be updated then.
          ACQ_CHANNEL_LOCK_WR;
          if(0 == ChannelNum)
          {
            ChannelNum = -1;
          }else if(3 == ChannelNum){
            ChannelNum = 4;
          }else if(-1 != ChannelNum && 4 != ChannelNum){
            LOG("ERROR: Wrong ChannelNum(%d) found when acq_data_state is %d!\n", ChannelNum, acq_data_state);
            RESET_GLOBAL_VAR;
            return;
          }
          RLS_CHANNEL_LOCK;

          chk_net_state = CHK_NET_STATE_IDLE;
          acq_data_state = ACQ_DATA_STATE_IDLE;
          suspend_state = SUSPEND_STATE_CIPCLOSE;
          w_suspendMdmHdlr();
          break;
        }
      }else if(ret != 1){
        LOG("ERROR: Unexpected ret (%d) found!\n", ret);
        RESET_GLOBAL_VAR;
        break;
      }

      //Note:
      //Not check isWaitToPwrDwn and isWaitToSusp here. Because app need to exit from
      //transparent transmission mode safely.

#ifdef CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR
      //Situation Presentation:
      //Once, "AT+CIPSEND=0,X" succeeds after "AT+CIPSEND=0,X" returns "+CIPSEND: 0,X,0" last time,
      //isCipsendFailedV01 should be reset false then.
      isCipsendFailedV01 = false; // Add by rjf at 20151016
#endif // CORRECTION_NO_FOLLOWING_RSP_ERROR_FOR_CIPERROR

    acq_data_state = ACQ_DATA_STATE_CIPSEND_2;
    w_acqDataHdlr();
    break;
  } // case ACQ_DATA_STATE_CIPSEND_1

  case ACQ_DATA_STATE_CIPSEND_2:
  {
    LOG("acq_data_state == ACQ_DATA_STATE_CIPSEND_2.\n");


#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

	LOG("Debug: b_enable_softwarecard_auth = %d .\n",b_enable_softwarecard_auth);
       LOG("Debug: b_enable_softwarecard_send = %d .\n",b_enable_softwarecard_send);

	if (b_enable_softwarecard_auth == true){

#ifdef FEATURE_ENABLE_RST_APDU_RSP_EXT_TIMER_WHEN_APDU_DATA_SENT_OUT
        if(REQ_TYPE_APDU == flow_info.req_type)
        {
          //Clear timer or timeout event
          if(true == isApduRspExtTimerExpired)
          {
            if(isApduRspExtTimerExpired)
            {
              isApduRspExtTimerExpired = false; // reset

              LOG("NOTICE: Clear ApduRsp ext timer in Timeouts_list.\n");
              clr_timeout_evt(&ApduRsp_timeout_ext_evt);
            }
          }
          isATEvtAct = false;
          clr_ApduRsp_ext_timer();
          add_timer_apdu_rsp_timeout_ext(2, 0);
          if(isDumpApdu){
            LOG("NOTICE: APDU is sent out to server, so isDumpApdu will be set false.\n");
            isDumpApdu = false;
          }
        }else
        { // Non-APDU
          isApduRspExtTimerExpired = false;
        }
#endif //FEATURE_ENABLE_RST_APDU_RSP_EXT_TIMER_WHEN_APDU_DATA_SENT_OUT
	GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
	
	//break;

}

#endif

__ACQ_DATA_STATE_CIPSEND_2_READER_AGAIN__:
			ret = reader(at_config.serial_fd, &ptr);
			if(ret == 0){
				LOG("ERROR: reader() error.\n");
				RESET_GLOBAL_VAR;
				break;
			}else if(ret == 2){
				if(*ATBufCur == '\0'){
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf.\n");
					goto __ACQ_DATA_STATE_CIPSEND_2_READER_AGAIN__;
				}
			}
      
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
      else if(ret == 3)
      {
				//Assume rsp str "OK" is before this URC report. And no URC report of "AT+CIPSEND..." is followed.
				ACQ_CHANNEL_LOCK_RD;
				if(0 == ChannelNum){
					ChannelNum = -1;
				}else if(3 == ChannelNum){
					ChannelNum = 4;
				}else if(0 != ChannelNum && 3 != ChannelNum){
					LOG("ERROR: reader() returns %d. Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ret, ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				add_node(p_req_cfg, flow_info.flow_state);


#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

				//Not care if the registration of mdm9215.
				//When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
				//that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
				LOG("Retry to check signal...\n");
				chk_net_state = CHK_NET_STATE_CSQ;
				w_checkNetHdlr();
				break;
      }
#endif // FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
      else if(ret == 5){
				ACQ_CHANNEL_LOCK_WR;
		   		if(0 == ChannelNum){
					ChannelNum = -1;
				}else if(3 == ChannelNum){
					ChannelNum = 4;
				}else{
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", \
							ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				add_node(p_req_cfg, flow_info.flow_state);
				
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
			}else if(ret == 10){
				//read() return -1 and errno equals 11.
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				break;
			}
			
			if(strcmp(ptr, "ERROR") == 0){
				LOG("ERROR: Unexpected rsp \"ERROR\" found!\n");
				
				//Handle "AT+CIPSEND..." (2) unexpected rsp "ERROR"
				sleep(1);
				//Maintain acq_data_state
				w_acqDataHdlr();
				break;
			}
			
			if(ret == 4){
				//Able to deal with redundant rsp "OK".
				if(*ATBufCur == '\0'){
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}else{
					LOG("There is still something unread in ATBuf.\n");
					goto __ACQ_DATA_STATE_CIPSEND_2_READER_AGAIN__;
				}
			}else if(ret == 1){
				int result = process_CIPSEND(ptr);
				
				//Fixed by rjf at 20150810 (Ignore the AT cmd failure.)
				//Note:
				//If flow_state is not FLOW_STATE_STAT_UPLOAD, a rsp from server is inevitable. So sending pd msg to server and 
				//recving rsp of pd msg will be disturbed by last sent msg. Therefore app will check isWaitToPwrDwn later in the state of
				//ACQ_DATA_STATE_RECVING.
				if(true == isWaitToPwrDwn && FLOW_STATE_STAT_UPLOAD == flow_info.flow_state){
					LOG("acq_data_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n", acq_data_state, isWaitToPwrDwn);
					isWaitToPwrDwn = false;
					
					if(isWaitToSusp){
						isWaitToSusp = false;
					}
					
					ACQ_CHANNEL_LOCK_WR;
					if(3 == ChannelNum){
						isMifiConnected = false;
						ChannelNum = 0;
					}else if(3 != ChannelNum && 0 != ChannelNum){
						LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
								ChannelNum, acq_data_state);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;
					
					LOG("Send pd msg to server.\n");
					msq_send_pd_msg_to_srv();
					break;
				}
				
				//Note:
				//In the similar way as above, app will check isWaitToSusp in the state of ACQ_DATA_STATE_RECVING.
				if(true == isWaitToSusp && FLOW_STATE_STAT_UPLOAD == flow_info.flow_state){
					LOG("acq_data_state=%d, isWaitToSusp=%d. Now, go to suspend mdmEmodem.\n",
							acq_data_state, isWaitToSusp);
					isWaitToSusp = false;
					
					ACQ_CHANNEL_LOCK_WR;
					if(3 == ChannelNum){
						ChannelNum = 2;
					}else if(0 == ChannelNum){
						//Situation Presentation:
						//If isWaitToSusp was set, and before executing the handler of "isWaitToSusp = 1", app has a need to terminate
						//the suspension plan. So, ChannelNum might be 0 at that time.
						
						LOG("ERROR: Req to suspend mdmEmodem, but ChannelNum = 0!\n");
						RLS_CHANNEL_LOCK;
						goto __ACQ_DATA_STATE_CIPSEND_2_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ChannelNum, acq_data_state);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;
					
					suspend_state = SUSPEND_STATE_CIPCLOSE;
					w_suspendMdmHdlr();
					break;
				}
__ACQ_DATA_STATE_CIPSEND_2_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__:

				if(true == isWaitToRcvr && FLOW_STATE_STAT_UPLOAD == flow_info.flow_state){
					LOG("acq_data_state=%d, isWaitToRcvr=%d. Now, go to recover mdmEmodem.\n", acq_data_state, isWaitToRcvr);
					isWaitToRcvr = false;
					
					ACQ_CHANNEL_LOCK_WR;
					if(3 == ChannelNum){
						ChannelNum = 4;
					}else if(0 == ChannelNum){
						ChannelNum = -1;
					}else{
						LOG("ERROR: Wrong ChannelNum (%d) found!\n", ChannelNum);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;
					
					RcvrMdmEmodemConn = true; // Add by rjf at 20150916
					acq_data_state = ACQ_DATA_STATE_IDLE;
					suspend_state = SUSPEND_STATE_CIPCLOSE;
					w_suspendMdmHdlr();
					break;
				}
				
				if(result == 0){
					LOG("ERROR: process_CIPSEND() error.\n");
					RESET_GLOBAL_VAR;
					break;
				}else if(result == 2){
					LOG("ERROR: \"AT+CIPSEND\" failed. Retry it.\n");
					GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
				}else if(result == 3){
					if(*ATBufCur == '\0'){
						LOG("Unexpected msg found. Continue to recv more...\n");
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						break;
					}else{
						LOG("There is still something unread in ATBuf. Continue to read...\n");
						goto __ACQ_DATA_STATE_CIPSEND_2_READER_AGAIN__;
					}
				}
			}

      if(flow_info.req_type == REQ_TYPE_REMOTE_UIM_DATA)
      {
        GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CATR_1, w_acqDataHdlr);
      }else if(flow_info.req_type == REQ_TYPE_STATUS)
      {
        LOG("Status info upload completes. isRegOffForPoorSig=%d, isNeedToSendIDMsg=%d, isQueued=%d.\n", 
                                                  isRegOffForPoorSig, isNeedToSendIDMsg, isQueued);

        //Situation presentation:
        //The operation of powering up virtual card is executed after sending out ID authentication msg in consideration of
        //downloading new virtual card and setting operation mode then as server request.

        if(!isNeedToSendIDMsg)
        {
          if(isQueued)
          {
            if(1 == queue_check_if_node_valid()){
              if(1 == dequeue_and_send_req(p_req_cfg))
                break;
            }else{
              // Note: When queue_check_if_node_valid() is just for checking invalid APDU, this operation is valid.
              queue_clr_apdu_node();
            }
          }

          int check_rsp_timers_result = -1, check_reg_result = -1;

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD     
          if(isYYDaemonNeedtoSendATCmd)
          {
            isYYDaemonNeedtoSendATCmd = false;
            proto_update_flow_state(FLOW_STATE_MDMEmodem_AT_CMD_REQ);
            acq_data_state = (ACQ_DATA_STATE_CUSTOM_AT_CMD);
            w_acqDataHdlr();
            break;
          }
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
          //isQueued is false, so not need to check the return value of set_at_evt_waiting().
          if(false == isNotNeedIdleTimer 
              && 0 == (check_rsp_timers_result = check_rsp_timers())
              && isRegOn // Add by rjf at 20150918
              && false == isSockListenerRecvPdpActMsg // Add by rjf at 20151014
            ){
              LOG("Set up idle timer for mdmEmodem.\n");
              add_timer_mdmEmodem_idle_timeout(0x00);
            }else{
              LOG("do not set up idle timer (acq_data_state: %d). "
                  "check_rsp_timers_result=%d, "
                  "check_reg_result=%d, "
                  "isSockListenerRecvPdpActMsg=%d.\n",
                  acq_data_state,
                  check_rsp_timers_result,
                  check_reg_result,
                  isSockListenerRecvPdpActMsg
                  );
            }
#endif // FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
          set_at_evt_waiting();
          break;
				}else
				{
					isNeedToSendIDMsg = false; //reset
					
					isSendIdAfterAcqIMSI = true;
					proto_update_flow_state(FLOW_STATE_IMSI_ACQ);
					notify_Sender(QMISENDER_ACQ_IMSI);
					
						//NOTE: AT event is shut down for the time being.
					
					//Add by rjf at 20151029
					//	Not utilize set_at_evt_waiting(), because dequeue is not allowed here.
					acq_data_state = ACQ_DATA_STATE_WAITING;
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					break;
				}
      }else
      {
#ifdef FEATURE_ENABLE_RST_APDU_RSP_EXT_TIMER_WHEN_APDU_DATA_SENT_OUT
        if(REQ_TYPE_APDU == flow_info.req_type)
        {
          //Clear timer or timeout event
          if(true == isApduRspExtTimerExpired)
          {
            if(isApduRspExtTimerExpired)
            {
              isApduRspExtTimerExpired = false; // reset

              LOG("NOTICE: Clear ApduRsp ext timer in Timeouts_list.\n");
              clr_timeout_evt(&ApduRsp_timeout_ext_evt);
            }
          }
          isATEvtAct = false;
          clr_ApduRsp_ext_timer();
          add_timer_apdu_rsp_timeout_ext(2, 0);
          if(isDumpApdu){
            LOG("NOTICE: APDU is sent out to server, so isDumpApdu will be set false.\n");
            isDumpApdu = false;
          }
        }else
        { // Non-APDU
          isApduRspExtTimerExpired = false;
        }
#endif //FEATURE_ENABLE_RST_APDU_RSP_EXT_TIMER_WHEN_APDU_DATA_SENT_OUT

      GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
      }
  } // case ACQ_DATA_STATE_CIPSEND_2

  case ACQ_DATA_STATE_RECVING:
  {
    int ret;
    int total_len = 0;
    int recv_len = 0;
    int err_num = -1;
  unsigned  int i_num = 1;
#ifdef FEATURE_ENABLE_CATR
    bool isNeedCATR = false;
#endif


#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

	int softcardsend_len = 0;

#endif

    LOG("acq_data_state == ACQ_DATA_STATE_RECVING.\n");

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

    char *P_softcard_buf = NULL;

     LOG("Debug: b_enable_softwarecard_auth = %d .\n",b_enable_softwarecard_auth);
     if (b_enable_softwarecard_auth == true){

		 ret = reader(at_config.serial_fd, &ptr);

		 LOG("Debug: Read at string is OK  ret =.%d )...\n", ret); 
		 LOG("Debug:  c_softwarecard_auth_len = %d \n ", c_softwarecard_auth_len);
 
     	} 
	 
#endif

    //*************************************************************************************
    //Process of recving data
    //*************************************************************************************
#if 1   // debug jack li 1118
if (b_enable_softwarecard_auth == false){
    ret = reader_recv_start(at_config.serial_fd, &recv_len, &total_len, isRetryToRecv);

    LOG("reader_recv_start() return %d.\n", ret);
}

#endif 
	
 #if 1   // debug jack li 1118
if (b_enable_softwarecard_auth == false){
    //Impossible ret: ret == -1 
    if(ret == -2)
    {
      GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
    }else if(ret == -3){
      GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
    }
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == -4)
    {
      ACQ_CHANNEL_LOCK_RD;
      if(0 == ChannelNum){
        ChannelNum = -1;
      }else if(3 == ChannelNum){
        ChannelNum = 4;
      }else if(0 != ChannelNum && 3 != ChannelNum){
        LOG("ERROR: reader_recv_start() returns %d. Wrong ChannelNum(%d) found!\n", ret, ChannelNum);
        RESET_GLOBAL_VAR;
        break;
      }
      RLS_CHANNEL_LOCK;

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(16);
#endif

      //Not care if the registration of mdm9215.
      //Not recv rsp anymore because old socket is released in underlying system. So wait for APDU
      //rsp ext timer is expired.

      //When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
      //that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
      LOG("Retry to check signal...\n");
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
#endif // FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == -5)
    {
      GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
    }else if(ret == -6 || ret == -9 || ret == -10)
    {
      //Clring all rsp timers in recving when serv conn broke, no restoration for sending req again.
      //clr_all_rsp_timers();
				ACQ_CHANNEL_LOCK_WR;
				if(0 == ChannelNum){
					ChannelNum = -1;
					#ifdef FEATURE_ENABLE_MSGQ
						msq_send_offline_ind();
					#endif
				}else if(3 == ChannelNum){
					ChannelNum = 4;
					#ifdef FEATURE_ENABLE_MSGQ
						msq_send_offline_ind();
					#endif
				}else{
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				if(ret == -6){
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
				}else if(ret == -9 || ret == -10){
					sleep(1); // Wait for closing socket completely.
					
					//ChannelNum is updated above.
					RcvrMdmEmodemConn = true;
					suspend_state = SUSPEND_STATE_NETCLOSE;
					w_suspendMdmHdlr();
					break;
			}
    }else if(ret == -7)
    {
      isRetryToRecv = 1;
      last_recv_len = recv_len;
      GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
    }else if(ret == -8)
    {
      isRetryToRecv = 2;
      last_recv_len = recv_len;
      GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
    }

    if(isRetryToRecv != 0)
    {
      recv_len += last_recv_len;
      LOG("last_recv_len=%d, recv_len(updated)=%d.\n", last_recv_len, recv_len);
      last_recv_len = 0; // Reset last_recv_len. It is necessary.
    }

}
#endif  // debug jack li 1118
    isRetryToRecv = 0;


#if 1  // debug jack li 1118
 if (b_enable_softwarecard_auth == false){
    ret = process_RECV(at_config.serial_fd, recv_len, total_len);
 	}
#endif  // debug jack li 1118

#if 1  // debug jack li 1118
 if (b_enable_softwarecard_auth == false){
     LOG("Debug : process_RECV() ret = %d.\n",ret);
    if(ret == -1){
      LOG("ERROR: process_RECV() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
    else if(ret == 0)
    {
      LOG("NOTICE: process_RECV() returns 0. Wrong data recved (flow_state: %d).\n", flow_info.flow_state);
      if(flow_info.flow_state != FLOW_STATE_APDU_COMM)
      {
        clr_invalid_timer(); // No way to return 0 .
        clr_read_buf_v02(8, 0);
        acq_data_state = ACQ_DATA_STATE_CIPSEND_1; // Fixed by rjf at 20150922
        w_acqDataHdlr();
        break;
      }else{
        LOG("isDumpApdu = %d.\n", isDumpApdu);
        if(isDumpApdu){ 
          isDumpApdu = false;

          //No matter if ApduRsp ext timer is expired
          isApduRspExtTimerExpired = false;
          clr_timeout_evt(&ApduRsp_timeout_ext_evt); // No matter if ApduRsp_timeout_ext_evt is in Timeouts_list
          clr_ApduRsp_ext_timer(); // No matter if ApduRsp ext timer is set
          goto __AFTER_SENDING_DATA_TO_MDM__;
        }else{
          LOG("This APDU is valid but not recved properly, so wait APDU-related timer to expire...\n");
          clr_read_buf_v02(5, 0);
          set_at_evt_waiting();
          break;
        }
      } // if(flow_info.flow_state == FLOW_STATE_APDU_COMM)
    }
 	}
#endif  // debug jack li 1118


#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

  if (b_enable_softwarecard_auth == true){

	b_enable_softwarecard_auth = false;

	reset_rsmp_recv_buffer();
	g_rsmp_recv_buffer.data = malloc(c_softwarecard_auth_len);
	memcpy(g_rsmp_recv_buffer.data, c_softwarecard_auth_buf, c_softwarecard_auth_len);
	//g_rsmp_recv_buffer.data = c_softwarecard_auth_buf;
	g_rsmp_recv_buffer.size = c_softwarecard_auth_len;
	
	LOG("Debug:  g_rsmp_recv_buffer.size = %d\n ",g_rsmp_recv_buffer.size);
	LOG("Debug:   c_softwarecard_auth_len = %d \n ", c_softwarecard_auth_len);
	
	LOG5("**************APDU DATA START*******************\n");
	i_num = 1;
	for(; i_num<=(c_softwarecard_auth_len); i_num++){
		if(i_num%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(g_rsmp_recv_buffer.data, i_num-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(g_rsmp_recv_buffer.data, i_num-1) );
				
		if(i_num%20 == 0) //Default val of i is 1
				LOG3("\n");
		}
		if((i_num-1)%20 != 0) // Avoid print redundant line break
			LOG3("\n");
	LOG5("************************************************* \n");
  }
#endif


LOG("Debug:  isWaitToPwrDwn = %d \n ",isWaitToPwrDwn);
    //*************************************************************************************
    //Handler of isWaitToPwrDwn
    //*************************************************************************************
    if(isWaitToPwrDwn)
    {
				LOG("acq_data_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
						acq_data_state, isWaitToPwrDwn);
				isWaitToPwrDwn = false;
				
				if(isWaitToSusp){
					isWaitToSusp = false;
				}
				
				ACQ_CHANNEL_LOCK_WR;
				if(3 == ChannelNum){
					isMifiConnected = false;
					ChannelNum = 0;
				}else if(3 != ChannelNum && 0 != ChannelNum){
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				LOG("Send pd msg to server.\n");
				msq_send_pd_msg_to_srv();
				break;
    }

#ifdef FEATURE_ENABLE_PRIVATE_PROTOCOL
    //*************************************************************************************
    //Parse private protocol packet
    //*************************************************************************************

proto_decode_wrapped_data(g_rsmp_recv_buffer.data, &err_num, &flow_info);

LOG("Debug : g_rsmp_recv_buffer.size = %d.\n",g_rsmp_recv_buffer.size);
LOG("Debug : flow_info.data_len = %d.\n",flow_info.data_len);

    
// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

	if(err_num == 0xaa){
	      if (id_auth_req_retran_times > 0){
			id_auth_req_retran_times = 0;
		  }
		LOG("OK: Get IP and port is right via UDP .\n");
		LOG("Debug: g_EmodemUdp_link_server_state = %d .\n",g_EmodemUdp_link_server_state);
		LOG("Debug: g_enable_EmodemUdp_link = %d .\n",g_enable_EmodemUdp_link);
      		LOG("Debug: g_enable_EmodemUdp_linkcount = %d .\n",g_enable_EmodemUdp_linkcount);

		g_EmodemUdp_link_server_state = true;
	 	g_enable_EmodemUdp_link = false;
        	g_enable_EmodemUdp_linkcount = 0;
		isProcOfInitApp = true;
		
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			acq_data_state = ACQ_DATA_STATE_CIPOPEN_REQ;
			
#else

			acq_data_state = ACQ_DATA_STATE_CIPOPEN;

#endif
		clr_ID_auth_rsp_timer();
	    
              w_acqDataHdlr();

	      break;
	    }

	if (err_num == 0xbb){
		
		g_EmodemUdp_link_server_state = false;
		g_EmodemUdp_flag= false;
		g_isip1_use = false;
		g_isip2_use = false;
	 	g_enable_EmodemUdp_link = true;
              g_enable_EmodemTCP_link = true;
        	g_enable_EmodemUdp_linkcount = 0;
	       g_enable_TCP_IP01_linkcount = 0;
		g_enable_TCP_IP02_linkcount = 0;
		isProcOfInitApp = true;
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
			acq_data_state = ACQ_DATA_STATE_CIPOPEN_REQ;
			
#else

			acq_data_state = ACQ_DATA_STATE_CIPOPEN;

#endif
		clr_ID_auth_rsp_timer();
	     //  w_closeUdpHdlr(); // Close UDP link
              w_acqDataHdlr();
	       break;

	}

#endif

    //*************************************************************************************
    //Check req_type
    //*************************************************************************************
    if(err_num == 6){
      LOG("Wrong req_type found. So dump the wrong pack, and retry to recv...\n");
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    //*************************************************************************************
    //Clear invalid timers
    //*************************************************************************************
#ifdef FEATURE_ENABLE_TIMER_SYSTEM
    {
      int result = -1;
      result = clr_invalid_timer();
	   LOG("Debug:  result = %d\n ", result);
      if(0 == result){
        goto __AFTER_SENDING_DATA_TO_MDM__;
      }
    }
#endif // FEATURE_ENABLE_TIMER_SYSTEM

    //*************************************************************************************
    //Handler of parsing result
    //*************************************************************************************
    if((err_num == 1) || (err_num == 4) || (err_num == 10) || (err_num == 11) || (err_num == 12))
    {
      //Note: 
      //err_num = 4 is for unrecoverable authenticating rejection.
      if (err_num == 10)
      {
	LOG("Your ID authentication is refused. err_num=%d.\n", err_num);
	msq_send_qlinkdatanode_run_stat_ind(18);
	}
      else if (err_num == 11)
      {
	LOG("the device doesn't find at the server list. err_num=%d.\n", err_num);
	msq_send_qlinkdatanode_run_stat_ind(19);
	}else if (err_num == 12){
	
		msqSendToTrafficAndDateYydaemon(NOTE_TRAFFIC_OUTOF,0ULL);
	}

#ifdef FEATURE_ENABLE_F9_CIPCLOSE	

	  g_closesocket_F9 = true;
	  suspend_state = SUSPEND_STATE_CIPCLOSE;
	  LOG("Debug: g_closesocket_F9 = %d .\n",g_closesocket_F9);
	  w_suspendMdmHdlr();
	  RESET_GLOBAL_VAR;
 #else
	RESET_GLOBAL_VAR;
#endif
       break;
    }else if(err_num == 2 || err_num == 3 || err_num == 5 || err_num == 7)
    {
#ifndef FEATURE_ENABLE_REQ_RETRAN_SYS_FOR_ID_AUTH_REJ
      sleep(1);
#else

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

	LOG("Debug: g_EmodemUdp_flag = %d .\n",g_EmodemUdp_flag);

	if (g_EmodemUdp_flag == true){

		LOG("Debug: g_enable_EmodemUdp_link = %d .\n",g_enable_EmodemUdp_link);
      		LOG("Debug: g_enable_EmodemUdp_linkcount = %d .\n",g_enable_EmodemUdp_linkcount);

	 	g_enable_EmodemUdp_link = false;
        	g_enable_EmodemUdp_linkcount = 0;
	}

	

#endif

	LOG("Debug: id_auth_req_retran_times = %d .\n",id_auth_req_retran_times);

      if(err_num == 7){
		
        if(id_auth_req_retran_times <= 1){
          sleep(2);
          id_auth_req_retran_times ++;
        }else if(id_auth_req_retran_times <= 11){
          sleep(30);
          id_auth_req_retran_times ++;
        }else{
          sleep(30);
          id_auth_req_retran_times ++;
        }
      }else{
        sleep(1);
      } // err_num != 7
#endif // FEATURE_ENABLE_REQ_RETRAN_SYS_FOR_ID_AUTH_REJ

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

	if((err_num == 7)){

		msq_send_qlinkdatanode_run_stat_ind(3);
		
		LOG("OK: Device don't get simcard from server F3 or F5 .\n");
		LOG("Debug: g_IdcheckRspis_F3 = %d .\n",g_IdcheckRspis_F3);
		LOG("Debug: g_enable_EmodemUdp_link = %d .\n",g_enable_EmodemUdp_link);
		LOG("Debug: g_enable_IDcheckCountTimes = %d .\n",id_auth_req_retran_times);
		LOG("Debug: g_enable_EmodemUdp_linkcount = %d .\n",g_enable_EmodemUdp_linkcount);
		LOG("Debug: g_EmodemUdp_link_server_state = %d .\n",g_EmodemUdp_link_server_state);

		// add by jack li 20161214.
		if (id_auth_req_retran_times >= 5){

			g_isip1_use = false;
			g_isip2_use = false;
			isProcOfInitApp = true;
			g_EmodemUdp_flag= false;
			g_IdcheckRspis_F3 = true;
		 	g_enable_EmodemUdp_link = true;
	              g_enable_EmodemTCP_link = true;
		       id_auth_req_retran_times = 0;
	        	g_enable_EmodemUdp_linkcount = 0;
		       g_enable_TCP_IP01_linkcount = 0;
			g_enable_TCP_IP02_linkcount = 0;
			g_EmodemUdp_link_server_state = false;

			LOG("Debug: F3  more than 6 times,so link UDP again. \n", err_num);
		       GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);

		}else{

			LOG("Debug: Send ID CHECK REG to server after F3. \n", err_num);
			g_IdcheckRspis_F3 = false;
              	GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);

		}
	
	 }else{

		LOG("ERROR: proto_decode_wrapped_data() error. err_num=%d. Send req again.\n", err_num);
              GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);

	 }

#else

      LOG("ERROR: proto_decode_wrapped_data() error. err_num=%d. Send req again.\n", err_num);
      GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);

#endif
    }else if(err_num == 8)
    {
      LOG("Rsp of pd msg is recved. Now, go to suspend mdmEmodem.\n");

      if(isPwrDwnNotificationTimerExpired){
        isPwrDwnNotificationTimerExpired = false; // reset

        LOG("NOTICE: Clear power down notification timer in Timeouts_list.\n");
        clr_timeout_evt(&power_down_notification_timeout_evt);
      }
      clr_power_down_notification_timer();

      set_ChannelNum(-1);

      isPwrDwnRspAcq = true; // Changed by rjf at 20150825

      suspend_state = SUSPEND_STATE_CIPCLOSE;
      w_suspendMdmHdlr();
      break;
    }

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

	LOG("Debug:  g_SoftcardFlowState = %d\n ",g_SoftcardFlowState);

	if (g_SoftcardFlowState == SOFT_CARD_STATE_FLOW_GETKEY_REG){

		p_req_cfg->data = proto_encode_raw_data(NULL, &softcardsend_len, REQ_TYPE_SOFT_KEY, NULL, NULL);
		p_req_cfg->size = softcardsend_len;

		LOG("Debug:  Neet to send get key reg.\n ");
		LOG("Debug:  softcardsend_len = %d\n ",softcardsend_len);
	
		GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
	
	}else if(g_SoftcardFlowState == SOFT_CARD_STATE_FLOW_GETKIOPC_REG){

		p_req_cfg->data = proto_encode_raw_data(NULL, &softcardsend_len, REQ_TYPE_SOFT_KIOPC, NULL, NULL);
		p_req_cfg->size = softcardsend_len;

		LOG("Debug:  Neet to send get KI and OPC reg.\n ");
		LOG("Debug:  softcardsend_len = %d\n ",softcardsend_len);
	
		GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);

	}else{

		LOG("Debug:  Don't neet to do anything.\n ");
		g_SoftcardFlowState = SOFT_CARD_STATE_FLOW_DEFAULT;

	}

#endif
    
#ifdef FEATURE_ENABLE_REQ_RETRAN_SYS_FOR_ID_AUTH_REJ
    //If it is an ID auth rsp, it means that ID auth is passed. Otherwise proto_decode_wrapped_data() returns err_num 7.
    id_auth_req_retran_times = 0; // reset
    g_IdcheckRspis_F3 = false;
#endif
LOG("Debug:  flow_info.req_type = %d\n ",flow_info.req_type);
LOG("Debug:  flow_info.data_len = %d\n ",flow_info.data_len);


    //*************************************************************************************
    //Handler of APDU rsp and remote USIM data packet
    //*************************************************************************************
    if(flow_info.req_type == REQ_TYPE_REMOTE_UIM_DATA 
      || flow_info.req_type == REQ_TYPE_APDU)
    {
      //**********Check req_type and flow_state**********
      if(flow_info.req_type == REQ_TYPE_REMOTE_UIM_DATA && flow_info.flow_state != FLOW_STATE_REMOTE_UIM_DATA_REQ)
      {
        LOG("req_type of recved data and flow_state is conflicting. req_type=%d, flow_state=%d.\n",flow_info.req_type, flow_info.flow_state);
        RESET_GLOBAL_VAR;
        break;
      }
	
      //**********Add head byte**********//
      /* only the apdu bytes > 2 (i.e not 61 XX)*/
      if(g_rsmp_recv_buffer.size >= PROTOCOL_HEADER_LEN_OF_APDU_COMM+3+1 // 1: Checkbyte length
          && flow_info.flow_state == FLOW_STATE_APDU_COMM)
      {
               g_rsmp_recv_buffer.data = realloc(g_rsmp_recv_buffer.data, g_rsmp_recv_buffer.size+1);
		 memmove(g_rsmp_recv_buffer.data+PROTOCOL_HEADER_LEN_OF_APDU_COMM+1, 
               		   g_rsmp_recv_buffer.data+PROTOCOL_HEADER_LEN_OF_APDU_COMM, 
                		   g_rsmp_recv_buffer.size-PROTOCOL_HEADER_LEN_OF_APDU_COMM);

	 
        g_rsmp_recv_buffer.size += 1;
        ((unsigned char *)(g_rsmp_recv_buffer.data))[PROTOCOL_HEADER_LEN_OF_APDU_COMM] = APDU_setting.head_byte;

	flow_info.data_len++;

      }


      //**********Remove private protocol header**********
      if(flow_info.req_type == REQ_TYPE_REMOTE_UIM_DATA)
      {
      
        //NOTE: g_rsmp_recv_buffer.size include data type and data length
        memmove(g_rsmp_recv_buffer.data, 
                g_rsmp_recv_buffer.data + PROTOCOL_HEADER_LEN_OF_REMOTE_UIM_DATA_PCK - FIELD_LEN_OF_DATA_TYPE_AND_DATA_LEN, 
                flow_info.data_len);
        g_rsmp_recv_buffer.size = flow_info.data_len;
		
      }
      else
      {

	
        memmove(g_rsmp_recv_buffer.data, 
                g_rsmp_recv_buffer.data + PROTOCOL_HEADER_LEN_OF_APDU_COMM, 
                flow_info.data_len);
		
        g_rsmp_recv_buffer.size = flow_info.data_len;
		
      }

#ifdef FEATURE_ENABLE_CATR
      if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ)
      {
        isNeedCATR = true;
      }
#endif
   
    //**********Transmit data to modem**********
    if(isOnlineMode == true && flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ)
    {
    
      //Copy remote USIM data into modem
      proto_update_flow_state(FLOW_STATE_REMOTE_UIM_DATA_REQ);
      notify_Sender(QMISENDER_WRITE_SIMDATA);
    }else{
      if(flow_info.flow_state == FLOW_STATE_REMOTE_UIM_DATA_REQ){
        notify_Sender(QMISENDER_WRITE_SIMDATA);
		
      	}
      else // FLOW_STATE_APDU_COMM
      	{
  
		i_num = 1;
		LOG("Debug :  g_rsmp_recv_buffer.size = %d.\n",g_rsmp_recv_buffer.size);
		LOG5("SEND TO MODEM START\n");
				for(; i_num<=(g_rsmp_recv_buffer.size); i_num++){
					if(i_num%20 == 1)
						LOG4(" %02x", get_byte_of_void_mem(g_rsmp_recv_buffer.data, i_num-1) );
					else
						LOG3(" %02x", get_byte_of_void_mem(g_rsmp_recv_buffer.data, i_num-1) );
					
					if(i_num%20 == 0) //Default val of i is 1
						LOG3("\n");
				}
				if((i_num-1)%20 != 0) // Avoid print redundant line break
					LOG3("\n");
		LOG5("SEND TO MODEM END\n");
      	//20160307 jackli
      	     int retr =-1;char buff=1;
	    do{
		    if(retr == 0){
		      LOG("write() failed. ret=%d.\n", ret);
		    }
		
		    retr = write(DMS_wr_fd, &buff, 1);
		  }while ((retr<0 && (errno==EINTR || errno==EAGAIN)) || (retr==0));

		  //Not care the result, but keep listening to the QMI DMS.
		  if(retr < 0){
		    LOG("ERROR: write() failed. ret=%d. errno=%d.\n", retr, errno);
		  }
		  
    	}
      }
    }


    //*************************************************************************************
    //Handler of isWaitToRcvr and isWaitToSusp
    //*************************************************************************************
    //Note: 
    //Execute the process of "isWaitToRcvr==true", then the "isRegOffForPoorSig==true".
	LOG("Debug:   isWaitToRcvr = %d\n ",isWaitToRcvr);

	if(isWaitToRcvr){
      //Situation Presentation:
      //Actually, server connection is available at this moment but still recover the server connection. Because some
      //handling process of the val of isRegOffForPoorSig need to be executed right after executing "AT+CIPOPEN=x".
      LOG("acq_data_state=%d, isWaitToRcvr=1. Now, go to recover the mdmEmodem.\n",acq_data_state);
      isWaitToRcvr = false;

      ACQ_CHANNEL_LOCK_WR;
      if(3 == ChannelNum)
      {
					ChannelNum = 4;
			}else if(0 == ChannelNum){
					ChannelNum = -1;
			}else{
        LOG("ERROR: Wrong ChannelNum (%d) found!\n", ChannelNum);
        RESET_GLOBAL_VAR;
        break;
      }
      RLS_CHANNEL_LOCK;
				
				RcvrMdmEmodemConn = true; // Add by rjf at 20150916
				acq_data_state = ACQ_DATA_STATE_IDLE;
				suspend_state = SUSPEND_STATE_CIPCLOSE;
				w_suspendMdmHdlr();
				break;
    }
LOG("Debug:   isWaitToSusp = %d\n ",isWaitToSusp);
    if(isWaitToSusp){
				LOG("acq_data_state=%d, isWaitToSusp=%d. Now, go to suspend mdmEmodem.\n",
						acq_data_state, isWaitToSusp);
				isWaitToSusp = false;
				
				ACQ_CHANNEL_LOCK_WR;
				if(3 == ChannelNum){
					ChannelNum = (2);
				}else if(0 == ChannelNum){
					//Suspension is started when dialup completes.
					LOG("ERROR: Req to suspend mdmEmodem, but ChannelNum = 0!\n");
					RLS_CHANNEL_LOCK;
					goto __ACQ_DATA_STATE_RECVING_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__;
				}else{
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
      RLS_CHANNEL_LOCK;

      suspend_state = SUSPEND_STATE_CIPCLOSE;
      w_suspendMdmHdlr();
      break;
    } //if isWaitToSusp

__ACQ_DATA_STATE_RECVING_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__:

    //*************************************************************************************
    //Handler of ID authentication rsp
    //*************************************************************************************


   if(flow_info.req_type == REQ_TYPE_ID)
    {
#ifdef FEATURE_ENABLE_MSGQ
      //Notify other process the time-zone info.
      msq_send_online_ind();
#endif

      if(flow_info.isNeedtoReqRemoteUSIM == true && flow_info.flag == 0x01)
      {
        //Cover local USIM data
        int length;

        flow_info.isNeedtoReqRemoteUSIM = false; // reset
        proto_update_flow_state(FLOW_STATE_REMOTE_UIM_DATA_REQ);
        LOG("Download remote USIM data and cover local one. flow_state changed to %d from %d.\n", 
                              FLOW_STATE_REMOTE_UIM_DATA_REQ, FLOW_STATE_ID_AUTH);
        reset_rsmp_transmit_buffer(p_req_cfg);
        p_req_cfg->data = proto_encode_raw_data(NULL, &length, REQ_TYPE_REMOTE_UIM_DATA, NULL, NULL);
        p_req_cfg->size = length;

#ifdef FEATURE_ENABLE_PROTOCOL_PACKET_CONTENT_LOG
        {
          int i = 0;
          LOG("\n");
          for(; i<p_req_cfg->size; i++){
            LOG("%02x ", get_byte_of_void_mem(p_req_cfg->data, i));
          }
          LOG("\n");
        }
#endif /*FEATURE_ENABLE_PROTOCOL_PACKET_CONTENT_LOG*/

        if(isQueued)
          queue_clr_apdu_node(); // Fixed by rjf at 20150803

        GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPSEND_1, w_acqDataHdlr);
      }else if(flow_info.isNeedtoReqRemoteUSIM == true && flow_info.flag == 0x00)
      {
        //Remove local USIM data
        flow_info.isNeedtoReqRemoteUSIM = false; // reset

        //Changed by rjf at 20151030
        set_at_evt_waiting();

        //No flow_state need to be updated.
        notify_Sender(QMISENDER_DEL_SIMDATA);

        break;
      }else if(flow_info.isNeedtoReqRemoteUSIM == true && flow_info.flag != 0x00 && flow_info.flag != 0x01)
      {
        LOG("ERROR: flow_info.isNeedtoReqRemoteUSIM = 1, but flow_info.flag = 0x%02x!\n",
                                                            flow_info.flag);
        RESET_GLOBAL_VAR;
        return;
      }else
      {
        LOG("No need to req remote USIM data. isOnlineMode=%d, isUimPwrDwn=%d.\n", isOnlineMode, isUimPwrDwn);
        
        msq_send_imsi_ind(0x00);

        if(isOnlineMode == false)
        {
          if(isQueued)
          {
            queue_clr_apdu_node(); // Fixed by rjf at 20150803
          }

          LOG("Set operation mode to online, flow_state is changed to %d from %d.\n", 
                                        FLOW_STATE_SET_OPER_MODE, flow_info.flow_state);
          pthread_mutex_lock(&pd_mtx);
          if(!isUimPwrDwn)
          {
            if(MdmEmodemIsRcvrWhenSetPd)
            {
              LOG("ERROR: isUimPwrDwn=0 and MdmEmodemIsRcvrWhenSetPd=1! ID auth msg should be sent when Usim has been powered down.\n");
              break; // Not release lock
            }else{
            
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

              proto_update_flow_state(FLOW_STATE_SET_OPER_MODE);
              notify_Sender(QMISENDER_SET_ONLINE);
            }
          }else
          {
            if(MdmEmodemIsRcvrWhenSetPd)
            {
              MdmEmodemIsRcvrWhenSetPd = false; // Reset it
            }

            power_up_virtual_usim();
          }
          pthread_mutex_unlock(&pd_mtx);
          //Go to wait for APDUs in subsequent operations below.
        }else
        {
						if(isQueued){
							if(1 == queue_check_if_node_valid()){
								if(1 == dequeue_and_send_req(p_req_cfg))
									break;
							}else{
								// Note: When queue_check_if_node_valid() is just for checking invalid APDU, this operation is valid.
								queue_clr_apdu_node();
							}
						}
						
          int check_rsp_timers_result = -1, check_reg_result = -1;
            
#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
          if(isYYDaemonNeedtoSendATCmd)
          {
            isYYDaemonNeedtoSendATCmd = false;
            proto_update_flow_state(FLOW_STATE_MDMEmodem_AT_CMD_REQ);
            acq_data_state = (ACQ_DATA_STATE_CUSTOM_AT_CMD);
            w_acqDataHdlr();
            break;
          }
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
							//isQueued is false, so not need to check if the return value of set_at_evt_waiting().
							if(false == isNotNeedIdleTimer 
								&& 0 == (check_rsp_timers_result = check_rsp_timers())
								&& isRegOn // Add by rjf at 20150918
								&& false == isSockListenerRecvPdpActMsg // Add by rjf at 20151014
							){
								LOG("Set up idle timer for mdmEmodem.\n");
								add_timer_mdmEmodem_idle_timeout(0x00);
							}else{
								LOG("Unable to set up idle timer (acq_data_state: %d). "
										"check_rsp_timers_result=%d, "
										"check_reg_result=%d, "
										"isSockListenerRecvPdpActMsg=%d.\n",
										acq_data_state,
										check_rsp_timers_result,
										check_reg_result,
										isSockListenerRecvPdpActMsg
									   );
							}
						#endif
						set_at_evt_waiting();
						break;
        }
      } // flow_info.isNeedtoReqRemoteUSIM == false
    }
    else if(flow_info.req_type != REQ_TYPE_REMOTE_UIM_DATA 
            && flow_info.req_type != REQ_TYPE_APDU)
    {
      LOG("ERROR: Wrong req_type found. req_type=0x%02x.\n", flow_info.req_type);
      RESET_GLOBAL_VAR;
      break;
    }
    //Note: Powerdown msg rsp is handled after proto_decode_wrapped_data().
#else
#error code not present
#endif // FEATURE_ENABLE_PRIVATE_PROTOCOL

    //*************************************************************************************
    //Subsequent work
    //*************************************************************************************
__AFTER_SENDING_DATA_TO_MDM__:
    if(isNeedCATR)
    {
      isNeedCATR = false;

      //Note:
      //If isWaitToSusp = true, the suspension of mdmEmodem will be executed after completing
      //the operation of CATR.
      GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CATR_2, w_acqDataHdlr);
    }else{

      ACQ_CHANNEL_LOCK_RD;
      if(0 == ChannelNum || 3 == ChannelNum)
      {
     
        int set_result = -1, check_rsp_timers_result = -1, check_reg_result = -1;

        set_result = set_at_evt_waiting();
        
#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
        if(false == isNotNeedIdleTimer 
          && 0 == (check_rsp_timers_result = check_rsp_timers())
          && 1 == set_result // Add by rjf at 20150911
          && isRegOn // Add by rjf at 20150918
          && false == isSockListenerRecvPdpActMsg // Add by rjf at 20151014
        ){
          LOG("Set up idle timer for mdmEmodem.\n");
          add_timer_mdmEmodem_idle_timeout(0x00);
        }else{
          LOG("Unable to set up idle timer (acq_data_state: %d). "
              "check_rsp_timers_result=%d, "
              "set_result=%d, "
              "check_reg_result=%d, "
              "isSockListenerRecvPdpActMsg=%d.\n",
              acq_data_state,
              check_rsp_timers_result,
              set_result,
              check_reg_result,
              isSockListenerRecvPdpActMsg
            );
        }
#endif //FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER

        RLS_CHANNEL_LOCK;
        break;
      }else{
        LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n", ChannelNum, acq_data_state);
        RESET_GLOBAL_VAR;
        break;
      }
    } // isNeedCATR == false
  } // case ACQ_DATA_STATE_RECVING

  //Note:
  //In suspension process of mdmEmodem, app will directly suspend mdmEmodem when acq_data_state equals
  //ACQ_DATA_STATE_WAITING rather than set isWaitToSusp true.
  case ACQ_DATA_STATE_WAITING:
  {
    LOG("acq_data_state == ACQ_DATA_STATE_WAITING.\n");

__ACQ_DATA_STATE_WAITING_READER_EXT__:
    ret = reader_ext(at_config.serial_fd);

    if(isWaitToRcvr){
				LOG("acq_data_state=%d, isWaitToRcvr=%d. Now, go to recover the mdmEmodem.\n",
						acq_data_state, isWaitToRcvr);
				isWaitToRcvr = false;
				
				ACQ_CHANNEL_LOCK_WR;
				if(3 == ChannelNum){
					ChannelNum = 4;
				}else if(0 == ChannelNum){
					ChannelNum = -1;
				}
				//Add "1 != ChannelNum" by rjf at 20151106 (For non-standby)
				else if(1 != ChannelNum){
					LOG("ERROR: Wrong ChannelNum (%d) found!\n", ChannelNum);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				RcvrMdmEmodemConn = true; // Add by rjf at 20150916
				acq_data_state = ACQ_DATA_STATE_IDLE;
				suspend_state = SUSPEND_STATE_CIPCLOSE;
				w_suspendMdmHdlr();
				break;
    }

    LOG("reader_ext return %d.\n", ret);
    if(ret == 0 || ret == 1){
      LOG("ERROR: reader_ext() error. ret=%d.\n", ret);
      RESET_GLOBAL_VAR;
      break;
    }else if(ret == 2 || ret == 5 || ret == 7){
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n");
        goto __ACQ_DATA_STATE_WAITING_READER_EXT__;
      }
    }
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == 3)
    {
      clr_idle_timer();

      ACQ_CHANNEL_LOCK_RD;
      if(0 == ChannelNum)
      {
        ChannelNum = -1;
      }else if(3 == ChannelNum)
      {
        ChannelNum = 4;
      }
      else if(1 == ChannelNum || 2 == ChannelNum)
      {
        LOG("ChannelNum == 1 or 2, so dispose the netclose urc\n ");
        RLS_CHANNEL_LOCK;
        break;
      }else{
        LOG("ERROR: reader() returns %d. Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
                ret, ChannelNum, acq_data_state);
        RESET_GLOBAL_VAR;
        break;
      }
      RLS_CHANNEL_LOCK;

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(16);
#endif

      //Not care if the registration of mdm9215.
      //When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
      //that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
      LOG("Retry to check signal...\n");
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
#endif /* FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE */
    else if(ret == 4 || ret == 8 || ret == 9){
				ACQ_CHANNEL_LOCK_WR;
				if(0 == ChannelNum){
					ChannelNum = -1;
					#ifdef FEATURE_ENABLE_MSGQ
						msq_send_offline_ind();
					#endif
				}else if(3 == ChannelNum){
					ChannelNum = 4;
					#ifdef FEATURE_ENABLE_MSGQ
						msq_send_offline_ind();
					#endif
				}
				//Add "1 != ChannelNum" by rjf at 20151106 (For non-standby)
				else if(1 != ChannelNum){
					LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
							ChannelNum, acq_data_state);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				if(ret == 4){
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
#else

			GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);

#endif
				}else{ // ret == 8 or ret == 9
					isServConnClosed = true;
					sleep(1);
					
					//ChannelNum is updated above.
					suspend_state = SUSPEND_STATE_NETCLOSE;
					w_suspendMdmHdlr();
					break;
				}
			}else{
				//Don't care unexpected msg.
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				break;
			} // else
		} // case ACQ_DATA_STATE_WAITING

#ifdef FEATURE_ENABLE_PRIVATE_PROTOCOL
  case ACQ_DATA_STATE_CGSN:
  {
    //Acq IMEI of mdmEmodem
    LOG("acq_data_state == ACQ_DATA_STATE_CGSN\n");

    //Just enter this acq_data_state once.
__ACQ_DATA_STATE_CGSN_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
    if(ret == 0){
      LOG("ERROR: reader() error.\n");
      break;
    }
    //Add "ret == 3" by rjf at 20151105 (For macro FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION)
    //When ret == 3, the URC rpt will be ignored. Because the app will act PDP later.
    else if(ret == 2 || ret == 3)
    {
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n"); \
        goto __ACQ_DATA_STATE_CGSN_READER_AGAIN__;
      }
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    ret = strcmp(ptr, "OK");

    int imeivalue = 0;
    if(ret != 0){
      imeivalue = process_CGSN(ptr);
      LOG("Debug: imeivalue = %d.\n",imeivalue);
	  
#ifdef FEATURE_ENABLE_5360IMEI_TO_YYDAEMON
	if (imeivalue == 1)
	{
		msq_send_5360imei_to_yydaemon();
	}
#endif

      if (-1 == imeivalue)
      {
		
		 GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CGSN, w_acqDataHdlr);
	}
	  
      if((ATBufCur == '\0'))
        goto __ACQ_DATA_STATE_CGSN_READER_AGAIN__;

	
    }

    GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_COPS_1, w_acqDataHdlr);
  } // case ACQ_DATA_STATE_CGSN

  case ACQ_DATA_STATE_COPS_1:
  case ACQ_DATA_STATE_CREG_1:

#ifdef  FEATURE_ENABLE_CPOL_CONFIG
  case ACQ_DATA_STATE_COPS_3:
  case ACQ_DATA_STATE_COPS_4:
#endif



  case ACQ_DATA_STATE_CREG_3:
#ifdef FEATURE_ENABLE_CATR
  case ACQ_DATA_STATE_CATR_1:
  case ACQ_DATA_STATE_CATR_2:
#endif // FEATURE_ENABLE_CATR
  {
    LOG("acq_data_state=%d.\n", acq_data_state);

    if(acq_data_state == ACQ_DATA_STATE_COPS_1 ||
      acq_data_state == ACQ_DATA_STATE_CREG_1 ||
   #ifdef  FEATURE_ENABLE_CPOL_CONFIG 
       acq_data_state == ACQ_DATA_STATE_COPS_3 ||
        acq_data_state == ACQ_DATA_STATE_COPS_4 ||
   #endif

 
    
      acq_data_state == ACQ_DATA_STATE_CREG_3
    ){
__ACQ_DATA_STATE_MISC_1_READER_AGAIN__:
      ret = reader(at_config.serial_fd, &ptr);
      if(ret == 0)
      {
        LOG("ERROR: reader() error.\n");
        RESET_GLOBAL_VAR;
        break;
      }
      //Add "ret == 3" by rjf at 20151105 (For macro FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION)
      //When ret == 3, the URC rpt will be ignored. Because the app will act PDP later.
      else if(ret == 2 || ret == 3)
      {
					LOG("reader() returns %d.\n", ret);
					if(*ATBufCur == '\0'){
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						break;
					}else{
						LOG("There is still something unread in ATBuf. Continue to read...\n"); \
						goto __ACQ_DATA_STATE_MISC_1_READER_AGAIN__;
					}
      }else if(ret == 10)
      {
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }
      //ret == 1 or ret == 4
    }else
    {
__ACQ_DATA_STATE_MISC_2_READER_AGAIN__:
      READER__ACQ_DATA(__ACQ_DATA_STATE_MISC_2_READER_AGAIN__);
      //That ret==3 is impossible. Because rmt USIM data was recved just before.
    }

    if(NULL == strstr(ptr, "OK")){
      LOG("Rsp \"OK\" not found. Message: %s.\n", ptr);
	  
#ifdef FEATURE_ENABLE_CPOL_CONFIG_DEBUG

      if (getcpolAtCommandNumber)
      {
		getcpolAtCommandNumber--;
		if (0 == getcpolAtCommandNumber)
		{
			getcpolAtnumber = 0;
		}
      }
      else
      {
		getcpolAtnumber = 0;
		getcpolAtCommandNumber = 0;
      }
#endif

	
      sleep(1);
      LOG("44444444444444444444444444 acq_data_state=%d.\n", acq_data_state);

  #ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG 
         if (acq_data_state != ACQ_DATA_STATE_CCLK ){
		 w_acqDataHdlr();
		  break;
	   }
   #else
  //Maintain acq_data_state
      w_acqDataHdlr();
      break;
   
   #endif
 
    }

    if(isTermRcvrNow)
    {
      LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n",
      acq_data_state, isTermRcvrNow);
      isTermRcvrNow = false;

      //ChannelNum is updated when isTermRcvrNow is set.

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      acq_data_state = ACQ_DATA_STATE_IDLE;
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
    //isWaitToRcvr is not applicable here 'cause it is set true when ChannelNum is 3 or 0.

    if(acq_data_state == ACQ_DATA_STATE_CREG_3)
    {
      LOG("acq_data_state=%d, flow_state changed to %d from %d.\n", ACQ_DATA_STATE_CREG_3, FLOW_STATE_IMSI_ACQ, flow_info.flow_state);
      proto_update_flow_state(FLOW_STATE_IMSI_ACQ);
      notify_Sender(QMISENDER_ACQ_IMSI);
      break; //NOTE: AT event is shut down for the time being.
#ifdef FEATURE_ENABLE_CATR
  }else if(acq_data_state == ACQ_DATA_STATE_CATR_1){
    GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_RECVING, r_acqDataHdlr, 'r');
  }else if(acq_data_state == ACQ_DATA_STATE_CATR_2){
    if(isWaitToSusp){
					LOG("NOTICE: isWaitToSusp=%d, acq_data_state=%d. Now, go to suspend mdmEmodem.\n", isWaitToSusp, acq_data_state);
					isWaitToSusp = false;
					
					ACQ_CHANNEL_LOCK_WR;
					if(3 == ChannelNum){
						ChannelNum = (2);
					}else if(0 == ChannelNum){
						//Suspension is started when dialup completes.
						LOG("ERROR: Req to suspend mdmEmodem, but ChannelNum = 0!\n");
						RLS_CHANNEL_LOCK;
						goto __ACQ_DATA_STATE_CATR_2_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! acq_data_state(%d) dismatch with ChannelNum.\n",
								ChannelNum, acq_data_state);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;
					
          suspend_state = SUSPEND_STATE_CIPCLOSE;
          w_suspendMdmHdlr();
          break;
        }else{
__ACQ_DATA_STATE_CATR_2_EXIT_OF_IS_WAIT_TO_SUSP_HANDLER__:
        //Note: 
        //After downloading and setting up remote USIM data, app doesn't need to consider suspending
        //mdmEmodem for that new APDU would definitely come.
        GO_TO_NEXT_STATE_R(ACQ_DATA_STATE_WAITING, r_acqDataHdlr, 'r');
      }
#endif // FEATURE_ENABLE_CATR
    }else
	{
#ifdef  FEATURE_ENABLE_CPOL_CONFIG
		 
		 LOG("Debug: the last getcpolAtCommandNumber = %d.\n",getcpolAtCommandNumber);
		 if (getcpolAtCommandNumber) 
		 {
			getcpolAtCommandNumber--;
			if (0 == getcpolAtCommandNumber )
			{
				GO_TO_NEXT_STATE_W(acq_data_state+1, w_acqDataHdlr);
				getcpolAtnumber = 0;
			}
			else
			{
				GO_TO_NEXT_STATE_W(acq_data_state, w_acqDataHdlr);
			}
			
		 }
		 else
		 {
			 GO_TO_NEXT_STATE_W(acq_data_state+1, w_acqDataHdlr);
			 getcpolAtnumber = 0;
			 getcpolAtCommandNumber = 0;
		 }

#else
		 GO_TO_NEXT_STATE_W(acq_data_state+1, w_acqDataHdlr);

#endif		 
		 
	}
  } 
  case ACQ_DATA_STATE_COPS_2:
  {
    LOG("acq_data_state == ACQ_DATA_STATE_COPS_2.\n");

__ACQ_DATA_STATE_COPS_2_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);

    LOG("Debug:. ret: %d.\n", ret);
	 
    if(ret == 0)
    {
      LOG("ERROR: reader() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
    //Add "ret == 3" by rjf at 20151105 (For macro FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION)
    //When ret == 3, the URC rpt will be ignored. Because the app will act PDP later.
    else if(ret == 2 || ret == 3)
    {
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n"); \
        goto __ACQ_DATA_STATE_COPS_2_READER_AGAIN__;
      }
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    ret = strcmp(ptr, "OK");
	
    if(ret != 0){
      if ((-1) == process_COPS(&sim5360_dev_info, ptr)){

		 LOG("Debug:process_COPS PLMN is 0xff\n");
		// system("reboot");

      }else{

		 LOG("Debug:process_COPS PLMN is OK\n");
      }

      if(ATBufCur == '\0')
        goto __ACQ_DATA_STATE_COPS_2_READER_AGAIN__;
    }else // xiaobin.wang modify 20160215 for rsp chaos
    {
      goto __ACQ_DATA_STATE_COPS_2_READER_AGAIN__;
    }

    if(isTermRcvrNow)
    {
      LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n", acq_data_state, isTermRcvrNow);
      isTermRcvrNow = false;

      //ChannelNum is updated when isTermRcvrNow is set.

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      acq_data_state = ACQ_DATA_STATE_IDLE;
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
 
#ifdef  FEATURE_ENABLE_CPOL_CONFIG
    GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_COPS_3, w_acqDataHdlr);
   
#else

	 #ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG
		GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CCLK, w_acqDataHdlr);
        #else 
              GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CREG_1, w_acqDataHdlr);
        #endif
  
#endif
      
  } // case ACQ_DATA_STATE_COPS_2

 #ifdef  FEATURE_ENABLE_Emodem_SET_TIME_DEBUG
   case ACQ_DATA_STATE_CCLK:

	{
    LOG("acq_data_state == ACQ_DATA_STATE_CCLK.\n");

__ACQ_DATA_STATE_CCLK_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
	LOG("999999999999999. ret: %d.\n", ret);
	 
    if(ret == 0)
    {
      LOG("ERROR: reader() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
   
    else if(ret == 2 || ret == 3)
    {
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n"); \
        goto __ACQ_DATA_STATE_CCLK_READER_AGAIN__;
      }
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    ret = strcmp(ptr, "OK");

    LOG("Debug :  ret:=  %d.\n", ret);
    LOG("Debug : ACQ_DATA_STATE_CCLK =%s.\n", ptr);

    P_cclk1 = strchr(ptr, '\"');
   
   for (cclk_m = 0; cclk_m < 17; cclk_m++){

	 P_cclk1 ++;
	
	 if ((cclk_m  != 2) && (cclk_m  != 5) && (cclk_m!= 8) && (cclk_m  != 11) && (cclk_m  != 14)){

		 cclkbuf[cclk_i] = *P_cclk1;
		 cclk_i++;
	 }
	 
	
   }

    rsp_Emodemcclk_ime.year = Ascii_To_int(cclkbuf,2);
    rsp_Emodemcclk_ime.month = Ascii_To_int(&cclkbuf[2],2);
    rsp_Emodemcclk_ime.mday = Ascii_To_int(&cclkbuf[4],2);
    rsp_Emodemcclk_ime.hour =  Ascii_To_int(&cclkbuf[6],2);
    rsp_Emodemcclk_ime.min =  Ascii_To_int(&cclkbuf[8],2);
    rsp_Emodemcclk_ime.sec =  Ascii_To_int(&cclkbuf[10],2);
	
    rsp_Emodemcclk_ime.year += 2000;

    LOG("Debug :  Get cclk time year :=  %d.\n", rsp_Emodemcclk_ime.year);
    LOG("Debug :  Get cclk time month :=  %d.\n", rsp_Emodemcclk_ime.month);
    LOG("Debug :  Get cclk time mday :=  %d.\n", rsp_Emodemcclk_ime.mday);
    LOG("Debug :  Get cclk time hour :=  %d.\n", rsp_Emodemcclk_ime.hour);
    LOG("Debug :  Get cclk time min :=  %d.\n", rsp_Emodemcclk_ime.min);
    LOG("Debug :  Get cclk time sec :=  %d.\n", rsp_Emodemcclk_ime.sec);


    if(isTermRcvrNow)
    {
      LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n", acq_data_state, isTermRcvrNow);
      isTermRcvrNow = false;

      //ChannelNum is updated when isTermRcvrNow is set.

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      acq_data_state = ACQ_DATA_STATE_IDLE;
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
 

    GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CREG_1, w_acqDataHdlr);
   

      
  }


	
#endif

#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
  
   case ACQ_DATA_STATE_CIPOPEN_REQ:

		{
    LOG("acq_data_state == ACQ_DATA_STATE_CIPOPEN_REQ.\n");

__ACQ_DATA_STATE_CIPOPEN_REQ_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
	//LOG("CCCCCCCCCCCC. ret: %d.\n", ret);
	 
    if(ret == 0)
    {
      LOG("ERROR: reader() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
   
    else if(ret == 2 || ret == 3)
    {
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n"); \
        goto __ACQ_DATA_STATE_CIPOPEN_REQ_READER_AGAIN__;
      }
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    ret = strcmp(ptr, "OK");

    LOG("ACQ_DATA_STATE_CIPOPEN_REQ ret = %d.\n", ret);
	

    if(isTermRcvrNow)
    {
      LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n", acq_data_state, isTermRcvrNow);
      isTermRcvrNow = false;

      //ChannelNum is updated when isTermRcvrNow is set.

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      //msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      acq_data_state = ACQ_DATA_STATE_IDLE;
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }
 

    GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPCLOSE, w_acqDataHdlr);
   

      
  }
	
   case ACQ_DATA_STATE_CIPCLOSE:

		{
    LOG("acq_data_state == ACQ_DATA_STATE_CIPCLOSE.\n");

__ACQ_DATA_STATE_CIPCLOSE_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
	//LOG("CCCCCCCCCCCC. ret: %d.\n", ret);
	 
    if(ret == 0)
    {
      LOG("ERROR: reader() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
   
    else if(ret == 2 || ret == 3)
    {
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n"); \
        goto __ACQ_DATA_STATE_CIPCLOSE_READER_AGAIN__;
      }
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    ret = strcmp(ptr, "OK");
	
    LOG("Debug . ret = %d.\n", ret);
    LOG("Debug . g_isat_command_AT = %d.\n", g_isat_command_AT);
    LOG("Debug ACQ_DATA_STATE_CIPCLOSE return rsp  =%s.\n", ptr);


    if ((ret == 0) && (g_isat_command_AT == false)){
		
      LOG("Debug Need to read more data.\n");
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;

    }

    if(isTermRcvrNow)
    {
      LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n", acq_data_state, isTermRcvrNow);
      isTermRcvrNow = false;

      //ChannelNum is updated when isTermRcvrNow is set.

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
    //  msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      acq_data_state = ACQ_DATA_STATE_IDLE;
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }

  LOG("Debug: g_EmodemUdpTcp_socket_linkCount = %d.\n", g_EmodemUdpTcp_socket_linkCount);

   if (g_EmodemUdpTcp_socket_linkCount > 0)
    g_EmodemUdpTcp_socket_linkCount--;

   if (g_EmodemUdpTcp_socket_linkCount == 0){
   	
   	g_EmodemUdpTcp_socket_linkCount = 2;
	GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN, w_acqDataHdlr);
	
   }else{

	GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CIPOPEN_REQ, w_acqDataHdlr);
   }

      
  }
  
  #endif
  
  case ACQ_DATA_STATE_CREG_2:
  {
    LOG("acq_data_state == ACQ_DATA_STATE_CREG_2.\n");

__ACQ_DATA_STATE_CREG_2_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &ptr);
    if(ret == 0){
      LOG("ERROR: reader() error.\n");
      RESET_GLOBAL_VAR;
      break;
    }
    //Add "ret == 3" by rjf at 20151105 (For macro FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION)
    //When ret==3, the URC rpt will be ignored. Because the app will act PDP later.
    else if(ret == 2 || ret == 3)
    {
      if(*ATBufCur == '\0'){
        INIT_ATBUF;
        set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
        add_evt (&at_config.AT_event, 'r');
        break;
      }else{
        LOG("There is still something unread in ATBuf.\n"); \
        goto __ACQ_DATA_STATE_CREG_2_READER_AGAIN__;
      }
    }else if(ret == 10){
      INIT_ATBUF;
      set_evt(&at_config.AT_event, at_config.serial_fd, false, r_acqDataHdlr, NULL);
      add_evt (&at_config.AT_event, 'r');
      break;
    }

    ret = strcmp(ptr, "OK");
    if(ret != 0){
      LOG("Get rsp cgreg. ptr: %s.\n", ptr);

      ret = process_CGREG_lac_and_cellid(&sim5360_dev_info, ptr);
      LOG("process_CGREG_lac_and_cellid result = : %d.\n", ret);
      if (ret == (-2)){
	  	LOG("process_CGREG_lac_and_cellid result = : %d.\n", ret);
			//system("reboot");
	  }

      if(ret == 0){

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

        LOG("NOTICE: Reg (mdmEmodem) has fail off. Retry to check signal...\n");
        acq_data_state = ACQ_DATA_STATE_IDLE; // Reset
        chk_net_state = CHK_NET_STATE_CSQ;
        w_checkNetHdlr();
        break;
      }else if(ATBufCur == '\0'){
        goto __ACQ_DATA_STATE_CREG_2_READER_AGAIN__;
      }
#ifdef FEATURE_ENABLE_Emodem_LCDSHOW
      else if(ret == 1)
      {
       LOG("Emodem net ok..\n");
       chk_net_state = CHK_NET_STATE_MAX; // fix Emodem busy issue by jack li 20160622
       msq_send_qlinkdatanode_run_stat_ind(11); 
      }
#endif
    }

    if(isTermRcvrNow)
    {
      LOG("acq_data_state=%d, isTermRcvrNow=%d. Now, cancel the recovering process of mdmEmodem.\n", acq_data_state, isTermRcvrNow);
      isTermRcvrNow = false;

      //ChannelNum is updated when isTermRcvrNow is set.
      
#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
      msq_send_qlinkdatanode_run_stat_ind(16);
#endif // FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND

      //RcvrMdmEmodemConn = true; //Fixed by rjf at 20150914 
      acq_data_state = ACQ_DATA_STATE_IDLE;
      chk_net_state = CHK_NET_STATE_CSQ;
      w_checkNetHdlr();
      break;
    }

    GO_TO_NEXT_STATE_W(ACQ_DATA_STATE_CREG_3, w_acqDataHdlr);

  } // case ACQ_DATA_STATE_CREG_2
#endif // FEATURE_ENABLE_PRIVATE_PROTOCOL

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
  case ACQ_DATA_STATE_CUSTOM_AT_CMD:
  {
    int read_bytes = 0, already_read_bytes = 0, set_result = -1;
    char rsp_buf[MAX_PROC_MSG_AT_RSP_STR_LEN] = {0}, term_str[] = "OK";

    LOG("ACQ_DATA_STATE_CUSTOM_AT_CMD.....\n");

__ACQ_DATA_STATE_CUSTOM_AT_CMD_READ_AGAIN__:
    read_bytes = read(at_config.serial_fd, rsp_buf+already_read_bytes, MAX_PROC_MSG_AT_RSP_STR_LEN-1-already_read_bytes);
    if(read_bytes < 0 || read_bytes == 0)
    {
      if(read_bytes == 0 || (read_bytes < 0 && (errno == EINTR)))
        goto __ACQ_DATA_STATE_CUSTOM_AT_CMD_READ_AGAIN__;

      LOG("ERROR: read() failed. errno=%d.\n", errno);
      memset(proc_msg_at_config.rsp_str, 0, MAX_PROC_MSG_AT_RSP_STR_LEN);
    }else if(read_bytes > 0)
    {
      char *p_term_str = NULL;

      if(already_read_bytes + read_bytes > MAX_PROC_MSG_AT_RSP_STR_LEN)
      {
        LOG("ERROR: rsp_buf is not enough to recv the whole rsp! rsp_buf: %s.\n", rsp_buf);
        memset(proc_msg_at_config.rsp_str, 0, MAX_PROC_MSG_AT_RSP_STR_LEN);
        goto __ACQ_DATA_STATE_CUSTOM_AT_CMD_SEND_OUT_PROC_MSG__;
      }

      p_term_str = find_sub_content((void *)(rsp_buf+already_read_bytes), (void *)term_str, read_bytes, strlen(term_str));
      if(NULL == p_term_str)
      {
        if(already_read_bytes + read_bytes == MAX_PROC_MSG_AT_RSP_STR_LEN-1){
          LOG("ERROR: \"OK\" is not recved, but rsp_buf is all occupied! rsp_buf: %s.\n", rsp_buf);
          memset(proc_msg_at_config.rsp_str, 0, MAX_PROC_MSG_AT_RSP_STR_LEN);
          goto __ACQ_DATA_STATE_CUSTOM_AT_CMD_SEND_OUT_PROC_MSG__;
        }

        already_read_bytes += read_bytes;
        goto __ACQ_DATA_STATE_CUSTOM_AT_CMD_READ_AGAIN__;
      }else
      { // rsp all recved.
        already_read_bytes += read_bytes;
        memcpy(proc_msg_at_config.rsp_str, rsp_buf, already_read_bytes);
#ifdef FEATURE_ENABLE_PRINT_CUSTOMED_AT_CMD_RSP
        LOG("AT cmd rsp: %s\n", proc_msg_at_config.rsp_str);
#endif
      }
    }

__ACQ_DATA_STATE_CUSTOM_AT_CMD_SEND_OUT_PROC_MSG__:
    msq_send_custom_at_rsp();

    proto_update_flow_state(FLOW_STATE_IDLE);

    if(isFactoryWriteIMEI5360)
    {
      isFactoryWriteIMEI5360 = false;
      LOG("only factory!!!!!\n");
      w_checkNetHdlr();
      break;
    }
    
    set_result = set_at_evt_waiting();
#ifdef FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER
    if(false == isNotNeedIdleTimer 
      && 0 == check_rsp_timers() 
      && 1 == set_result // Add by rjf at 20150911
      && isRegOn // Add by rjf at 20150918
      && false == isSockListenerRecvPdpActMsg // Add by rjf at 20151014
    ){
      //Rsp timers are added before setting idle timer.
      LOG("Set up idle timer for mdmEmodem.\n");
      add_timer_mdmEmodem_idle_timeout(0x00);
    }
#endif //FEATURE_ENABLE_MDMEmodem_IDLE_TIMEOUT_TIMER

    break;
  } // case ACQ_DATA_STATE_CUSTOM_AT_CMD
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

  default:
  {
    LOG("ERROR: Wrong acq_data_state found. acq_data_state=%d.\n", acq_data_state);
    RESET_GLOBAL_VAR;
    break;
  }
  } // switch(acq_data_state)

  RLS_ACQ_DATA_STATE_MTX;

  return;
}

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK
void w_closeUdpHdlr(void)
{
	int ret;
       char *cmd = NULL;

	cmd = "AT+CIPCLOSE=1";

	LOG("    --> \"%s\"  \n", cmd);
	ret = writer(at_config.serial_fd, cmd, strlen(cmd));
	if(ret != 1){
	    LOG("ERROR: writer() error. w_closeUdpHdlr\n");
	    return;
	}
	
	INIT_ATBUF;
	
	set_evt(&at_config.AT_event, at_config.serial_fd, false, r_closeUdpHdlr, NULL);
	add_evt (&at_config.AT_event, 'r');
	
	return;

}



void r_closeUdpHdlr(void *userdata)
{
  int ret;
  char *rsp = NULL;


   ret = reader(at_config.serial_fd, &rsp);
    if(ret == 0){
      LOG("ERROR: reader() error.\n");
     /// RESET_GLOBAL_VAR;
      return;
    }

LOG("ERROR: reader() ret = %d.\n",ret);
 return;
  
}


#endif

void w_suspendMdmHdlr(void)
{
  int ret;
  char *cmd = NULL;

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
  if(isMdmEmodemControledByYydaemon){
    LOG("NOTICE: isMdmEmodemControledByYydaemon=1. w_suspendMdmHdlr() failed.\n");
    return;
  }
#endif //FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
  
  clr_idle_timer();

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK

 LOG("Debug: g_EmodemUdp_socket_link = %d .\n",g_EmodemUdp_socket_link);
 LOG("Debug: g_EmodemTcp_socket_link = %d .\n",g_EmodemTcp_socket_link);

 LOG("Debug: g_isip1_use = %d .\n",g_isip1_use);
 LOG("Debug: g_isip2_use = %d .\n",g_isip2_use);
 LOG("Debug: g_EmodemUdp_link_server_state = %d .\n",g_EmodemUdp_link_server_state);
 
 LOG("Debug: g_enable_EmodemUdp_link = %d .\n",g_enable_EmodemUdp_link);
 LOG("Debug: g_enable_EmodemTCP_link = %d .\n",g_enable_EmodemTCP_link);
 LOG("Debug: g_enable_EmodemUdp_linkcount = %d .\n",g_enable_EmodemUdp_linkcount);
 LOG("Debug: g_enable_TCP_IP01_linkcount = %d .\n",g_enable_TCP_IP01_linkcount);
 LOG("Debug: g_enable_TCP_IP02_linkcount = %d .\n",g_enable_TCP_IP02_linkcount);

#endif

 LOG("Debug: suspend_state = %d .\n",suspend_state);
  switch(suspend_state)
  {
  case SUSPEND_STATE_CIPCLOSE:

// 20160928 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK


	if (g_enable_EmodemUdp_link == true){

		if (g_EmodemUdp_socket_link == true){

			g_EmodemUdp_socket_link = false;
			cmd = "AT+CIPCLOSE=1";

		}else{

			 cmd = "AT";
		}
		
		g_enable_EmodemUdp_linkcount++;
	}
	else{
		
		if (g_EmodemTcp_socket_link == true){

			g_EmodemTcp_socket_link = false;
			cmd = "AT+CIPCLOSE=0";

		}else{

			 cmd = "AT";
		}

		if (g_enable_EmodemTCP_link == true){

			g_enable_EmodemTCP_link = false;
			g_enable_EmodemUdp_linkcount = 0;
			
			if(g_isip1_use == false){
				if(g_enable_TCP_IP01_linkcount >= 3){
					if (g_EmodemUdp_link_server_state == true){

						g_enable_EmodemUdp_link = true;
						g_enable_EmodemUdp_linkcount = 0;
						g_enable_TCP_IP01_linkcount = 0;
						g_EmodemUdp_link_server_state = false;
						
					}
					else{
						g_enable_TCP_IP01_linkcount = 3;
					}
					
				}else{
					g_enable_TCP_IP01_linkcount++;
				}
				
			}
			
			if((g_enable_TCP_IP01_linkcount >= 3) && (g_EmodemUdp_link_server_state == false)){
				
				if(g_isip2_use == false){
					
					if(g_enable_TCP_IP02_linkcount >= 3){
						g_enable_TCP_IP02_linkcount = 3;
					}else{
						g_enable_TCP_IP02_linkcount++;
					}
				
				}
			}

		}
		
	}

	

#else

    cmd = "AT+CIPCLOSE=0";

#endif

    break;

  case SUSPEND_STATE_NETCLOSE:
#ifdef FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION
    cmd = "AT"; //xiaobin.wang modify 
#else
    cmd = "AT+NETCLOSE";
#endif
// 20161008 by jackli
#ifdef FEATURE_ENABLE_EmodemUDP_LINK
            if (g_enable_EmodemTCP_link == true){

			g_enable_EmodemTCP_link = false;
			g_enable_EmodemUdp_linkcount = 0;
			
			if(g_isip1_use == false){
				if(g_enable_TCP_IP01_linkcount >= 3){
					if (g_EmodemUdp_link_server_state == true){

						g_enable_EmodemUdp_link = true;
						g_enable_EmodemUdp_linkcount = 0;
						g_enable_TCP_IP01_linkcount = 0;
						
					}
					else{
						g_enable_TCP_IP01_linkcount = 3;
					}
					
				}else{
					g_enable_TCP_IP01_linkcount++;
				}
				
			}
			
			if((g_enable_TCP_IP01_linkcount >= 3) && (g_EmodemUdp_link_server_state == false)){
				
				if(g_isip2_use == false){
					
					if(g_enable_TCP_IP02_linkcount >= 3){
						g_enable_TCP_IP02_linkcount = 3;
					}else{
						g_enable_TCP_IP02_linkcount++;
					}
				
				}
			}

		}
#endif

    break;

  case SUSPEND_STATE_IDLE:
  default:
  {
    LOG("Wrong case entered. suspend_state=%d.\n", suspend_state);
    return;
  }
  } // switch

  LOG("    --> \"%s\"  \n", cmd);
  ret = writer(at_config.serial_fd, cmd, strlen(cmd));
  if(ret != 1){
    LOG("ERROR: writer() error. suspend_state=%d.\n", suspend_state);
    return;
  }
  INIT_ATBUF;
  set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
  add_evt (&at_config.AT_event, 'r');
  if(suspend_state == SUSPEND_STATE_CIPCLOSE)
    trigger_EventLoop();
  return;
}

void r_suspendMdmHdlr(void *userdata)
{
  int ret;
  char *rsp = NULL;

  LOG("suspend_state = %d.\n", suspend_state);
  
  #ifdef FEATURE_ENABLE_STATUS_REPORT_10_MIN	
	
	  static bool count_var = false;
#endif

  switch(suspend_state)
  {
  case SUSPEND_STATE_CIPCLOSE:
  {
    int result;

__SUSPEND_STATE_CIPCLOSE_READER_AGAIN__:
    ret = reader(at_config.serial_fd, &rsp);
    if(ret == 0){
      LOG("ERROR: reader() error.\n");
      RESET_GLOBAL_VAR;
      return;
    }else if(ret == 2 
						#ifndef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
							|| ret == 3 /*Ignore "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY"*/
						#endif
			){
				if(*ATBufCur == '\0'){
					LOG("reader() return %d. Need to recv more rsp.\n", ret);
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}else{
					LOG("There is still something unread in ATBuf. Continue to read more.\n");
					goto __SUSPEND_STATE_CIPCLOSE_READER_AGAIN__;
				}
			}
#ifdef FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
			else if(ret == 3){
				ACQ_CHANNEL_LOCK_RD;
				LOG("ChannelNum = %d.\n", ChannelNum);
				if(ChannelNum != -1 && ChannelNum != 2 && ChannelNum != 4){
					LOG("ERROR: reader() returns %d. Wrong ChannelNum(%d) found!\n",
							ret, ChannelNum);
					RESET_GLOBAL_VAR;
					break;
				}
				RLS_CHANNEL_LOCK;
				
				#ifdef FEATURE_ENABLE_MSGQ
					msq_send_offline_ind();
				#endif
				
				//Not care if the registration of mdm9215.
				//	When "+CIPEVENT: NETWORK CLOSED UNEXPECTEDLY" is recved, it is equivalent
				//that "AT+CIPCLOSE=X" and "AT+NETCLOSE" are all executed.
				suspend_state = SUSPEND_STATE_MAX;
				
				ACQ_CHANNEL_LOCK_WR;
				if(-1 == ChannelNum || 4 == ChannelNum){
					//FIXME: Recover mdmEmodem below, so clear all variables on recovering mdmEmodem in case of repeative recovering
					//process.
					isTermRcvrNow = false;
					RcvrMdmEmodemConn = false;
					isRegOffForPoorSig = false;
					isServConnClosed = false;
					
					//Maintain ChannelNum

					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(16);
					#endif
					
					LOG("Retry to check signal...\n");
					chk_net_state = CHK_NET_STATE_CSQ;
					w_checkNetHdlr();
				}else{ // 2 == ChannelNum
					if(isTermSuspNow){
						LOG("isTermSuspNow=1, so go to recover mdmEmodem.\n");
						isTermSuspNow = false;
						if(RcvrMdmEmodemConnWhenSuspTerm)
							RcvrMdmEmodemConnWhenSuspTerm = false;
						
						ChannelNum = 1;

						#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
							msq_send_qlinkdatanode_run_stat_ind(16);
						#endif
						
						LOG("Retry to check signal...\n");
						chk_net_state = CHK_NET_STATE_CSQ;
						w_checkNetHdlr();
						RLS_CHANNEL_LOCK;
						break;
					}
					
					#ifndef FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
						LOG("Socket is cleared and PDP is deactiviated by underlying system! Now, go to suspend mdmEmodem.\n");
						mdmEmodem_set_standby();
						isMdmEmodemRunning = false;
					#else
						LOG("Socket is cleared and PDP is deactiviated by underlying system!\n");
						isMdmEmodemRunning = false; // Pretend that mdmEmodem is shut down
					#endif
					
					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(17);
					       qlinkdatanode_run_stat = 0;
					#endif
					
					ChannelNum = 1;
					
					ACQ_ACQ_DATA_STATE_MTX;
					set_at_evt_waiting();
					RLS_ACQ_DATA_STATE_MTX;
				}
				RLS_CHANNEL_LOCK;
				
				break;
			}
		#endif // FEATURE_ENABLE_MDMEmodem_HDL_RPT_UNEXP_NETWORK_CLOSE
    else if(ret == 5){ //Note: It is regarded as AT cmd unsolicited report.
				result = 256;
				if(isWaitToPwrDwn)
					goto __CIPCLOSE_SUBSEQUENT_HANDLING_1__;
				else
					goto __CIPCLOSE_SUBSEQUENT_HANDLING_2__;
			}else if(ret == 10){
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				return;
			}else if(ret == 14){
				//Note:
				//		If cmd "AT+NETOPEN" and "AT+CIPOPEN=..." are all not executed before executing
				//	"AT+CIPCLOSE=<link_num>", "ERROR" will be only returned. (There should be a string "+CIPCLOSE: 0,4" in rsp.)
				//		If cmd "AT+CIPOPEN=..." is not executed before executing "AT+CIPCLOSE=<link_num>",
				//	"+CIPCLOSE: 0,4\r\n\r\nERROR" will be returned.
				//	No matter isWaitRspError is true or not.
				isWaitRspError = false;
				//Restoration
				ret = 15;
				memset(RespArray, 0, sizeof(RespArray));
				memcpy(RespArray, "+CIPCLOSE: 0,4", sizeof("+CIPCLOSE: 0,4")-1);
				rsp = RespArray;
			}else if(ret == 15){
				if(*ATBufCur == '\0' || \
					(*ATBufCur != '\0' && (strncmp("ERROR", ATBufCur, sizeof("ERROR")-1) != 0)) \
				){
					LOG("The rsp \"ERROR\" behind \"+CIPCLOSE: 0,4\" is missed.");
					isWaitRspError = true;
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}
			}

			if(ret == 4 && strncmp(rsp, "OK", 2) == 0){
				if(*ATBufCur != '\0'){
					LOG("There is still something in ATBuf. Try to recv more.\n");
					goto __SUSPEND_STATE_CIPCLOSE_READER_AGAIN__;
				}else{
					LOG("\"OK\" recved. Need to recv more.\n");
					INIT_ATBUF;	
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}
			}
			
    //ret = 1
    //result = 4: +CIPCLOSE: 0,4
    //result = 256: +CIPCLOSE: 0,1 (255 is largest val of <err>.)
    result = process_CIPCLOSE(rsp);
__CIPCLOSE_SUBSEQUENT_HANDLING_1__:
    //Need to recv OK to know if CIPCLOSE succeed.
    if(isWaitToPwrDwn){
				LOG("suspend_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
						suspend_state, isWaitToPwrDwn);
				isWaitToPwrDwn = false;
				
				ACQ_CHANNEL_LOCK_WR;
				if(4 == ChannelNum){
					isMifiConnected = false;
					ChannelNum = -1;
				}else if(2 == ChannelNum){
					isMifiConnected = false;
					ChannelNum = -1;
					
					RcvrMdmEmodemConn = true;
				}else if(ChannelNum != -1){ // Fixed by rjf at 20150810
					LOG("ERROR: Wrong ChannelNum(%d) found! suspend_state(%d) dismatch with ChannelNum.\n", ChannelNum, suspend_state);
					RESET_GLOBAL_VAR;
					return;
				}
				RLS_CHANNEL_LOCK;
				
				//Send pd msg when mdmEmodem has connected to server
				isNeedToSendOutReq0x65 = true;
				
			}

__CIPCLOSE_SUBSEQUENT_HANDLING_2__:
    if(result == 0 || result == 4 || result == 256)
    {
				if(result == 0){
					LOG("\"AT+CIPCLOSE=0 or AT+CIPCLOSE=1\" succeed.\n");
				}else if(result == 4){
					LOG("Server connection is already closed.\n");
				}else{
					LOG("Server connection has already been closed by peer.\n");
				}
				
      LOG("isPwrDwnRspAcq=%d, isTermSuspNow=%d, isTermRcvrNow=%d.\n",
        isPwrDwnRspAcq, isTermSuspNow, isTermRcvrNow);

				if(isPwrDwnRspAcq == false 
				  ){
					//Note:
					//		Handle isRegOffForPoorSig = true and isServRspTimeout = true as normal suspension step 
					//	of closing server connection.
					
					#ifdef FEATURE_ENABLE_MSGQ
						msq_send_offline_ind();
					#endif // FEATURE_ENABLE_MSGQ
					
					if(true == isTermSuspNow && true == isTermRcvrNow){
						LOG("ERROR: isTermSuspNow and isTermRcvrNow are all true!\n");
						RESET_GLOBAL_VAR;
						break;
					}
					
					/***************Handle isTermSuspNow***************/
					if(isTermSuspNow){
						LOG("isTermSuspNow=1. RcvrMdmEmodemConnWhenSuspTerm=%d. RcvrMdmEmodemConn=%d.\n", 
								RcvrMdmEmodemConnWhenSuspTerm, RcvrMdmEmodemConn);
						//Setting isTermSuspNow and RcvrMdmEmodemConn true at same time is incorrect!
						isTermSuspNow = false;
						
						ACQ_CHANNEL_LOCK_WR;
						if(2 == ChannelNum){
							if(isRegOn)
								ChannelNum = 4;
							else
								ChannelNum = -1;
							
							LOG("ChannelNum is changed to %d from 2.\n", ChannelNum);
						}else{
							LOG("ERROR: Wrong ChannelNum(%d) found! The val of isTermSuspNow(1) dismatch with ChannelNum.\n", ChannelNum);
							RESET_GLOBAL_VAR;
							return;
						}
						RLS_CHANNEL_LOCK;
						
						//isMdmEmodemRunning must be true.
						
						if(!RcvrMdmEmodemConnWhenSuspTerm){
							suspend_state = SUSPEND_STATE_IDLE;
							ACQ_ACQ_DATA_STATE_MTX;
#ifdef FEATURE_ENABLE_EmodemUDP_DEBUGDEBUGDEBUG
			acq_data_state = ACQ_DATA_STATE_CIPOPEN_REQ;
			
#else

			acq_data_state = ACQ_DATA_STATE_CIPOPEN;

#endif
							w_acqDataHdlr();
							RLS_ACQ_DATA_STATE_MTX;
							return;
						}else{
							RcvrMdmEmodemConnWhenSuspTerm = false;
							RcvrMdmEmodemConn = true;
						}
					} // if(isTermSuspNow)
					
					/***************Handle isTermRcvrNow***************/
					if(isTermRcvrNow){
						LOG("isTermRcvrNow=1. Because suspend_state=1, nothing need to be done.\n");
						isTermRcvrNow = false;
						
						RcvrMdmEmodemConn = true;
						//Keep working on old process of "recovering" mdmEmodem
					}

        /***************Suspend MdmEmodem Without Deactivate PDP***************/
#ifdef FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION
        int chan_num = get_ChannelNum();

        if(2 == chan_num || 
          (-1 == chan_num && (false == isRegOffForPoorSig && false == RcvrMdmEmodemConn && false == isServConnClosed))
        ){
          LOG("ChannelNum=%d, isRegOffForPoorSig=%d, RcvrMdmEmodemConn=%d, isServConnClosed=%d.\n", 
            chan_num, isRegOffForPoorSig, RcvrMdmEmodemConn, isServConnClosed);

          //Sometimes, it has been false already, but it's fine to exec operation below.
          isSockListenerRecvPdpActMsg = false;

#ifndef FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
          mdmEmodem_set_standby();
          isMdmEmodemRunning = false;
          LOG("mdmEmodem has been suspended successfully!\n");
#else
	
         isMdmEmodemRunning = false;
         LOG("(first)The network link (of mdmEmodem) has been cleared successfully!\n");
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
		 //BaseTimeOutDate();
#endif

#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810

	g_qlinkdatanode_run_status_01 = CPIN_READY_OK_AFTER;
	LOG("g_qlinkdatanode_run_status_01 =%d.\n", g_qlinkdatanode_run_status_01);

#endif

#ifdef FEATURE_ENABLE_3GONLY
	LOG("Debug:01 ChannelNum = %d.\n", ChannelNum);
	LOG("Debug:isneedtoset3Gonly =%d.\n", isneedtoset3Gonly);
       LOG("Debug:suspend_state = %d.\n", suspend_state);
	LOG("Debug:acq_data_state = %d.\n", acq_data_state);
	LOG("Debug:rsmp_net_info.basic_val = %d.\n", rsmp_net_info.basic_val);
	LOG("Debug isOnlineMode=%d, isUimPwrDwn=%d.\n", isOnlineMode, isUimPwrDwn);

	if (isneedtoset3Gonly == true)
	{
		LOG("DEBUG:isneedtoset3Gonly =%d.\n", isneedtoset3Gonly);
		pthread_mutex_lock(&acq_nas_inf_mtx);
      		notify_NasSender(0x04);
      		while(0 == nas_inf_acqed){
       	 pthread_cond_wait(&acq_nas_inf_cnd, &acq_nas_inf_mtx);
     		 }
     		 nas_inf_acqed = 0; // Reset it for using it again
     		 pthread_mutex_unlock(&acq_nas_inf_mtx);
      
	}

#endif
		  
#endif // FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
#ifdef FEATURE_ENABLE_STATUS_REPORT_10_MIN	

	if(count_var == false){
		count_var = true;
		notify_Sender_status(0x04);
		msq_send_IMEI_to_update();
	}

#endif
					
						ACQ_CHANNEL_LOCK_WR;
						if(2 == ChannelNum)
            {
              ChannelNum = 1;
						}
						else if(-1 != ChannelNum){
							LOG("ERROR: Wrong ChannelNum(%d) found! suspend_state(%d) dismatch with ChannelNum.\n",
									ChannelNum, suspend_state);
							RESET_GLOBAL_VAR;
							break;
						}
						RLS_CHANNEL_LOCK;
						
						#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
							msq_send_qlinkdatanode_run_stat_ind(17);
							qlinkdatanode_run_stat = 0;
						#endif
						
						suspend_state = SUSPEND_STATE_MAX;
						ACQ_ACQ_DATA_STATE_MTX;
						set_at_evt_waiting();
						RLS_ACQ_DATA_STATE_MTX;
						
						break;
					}
					#endif /*FEATURE_MDMEmodem_NOT_DEACT_PDP_IN_SUSPENSION*/
					
					//Not check ChannelNum here!!!
					//Up to now, the value of ChannelNum -1, 2, 4 are all valid.
					
					if(4 == result)
						sleep(1); //Add by rjf at 20150811 (Wait for closing socket completely.)

        suspend_state = SUSPEND_STATE_NETCLOSE;

        w_suspendMdmHdlr();
      }else if(isPwrDwnRspAcq){
					suspend_state = SUSPEND_STATE_MAX;
					ACQ_ACQ_DATA_STATE_MTX;
					set_at_evt_waiting();
					RLS_ACQ_DATA_STATE_MTX;
					
					isMdmEmodemRunning = false; // Set it false in advance.
					
					#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_SHUTDOWN
						mdmEmodem_shutdown();
					#endif
					
					LOG("Send msg to notify other processes.\n");
					msq_send_pd_rsp();
				}

			}else{
				LOG("ERROR: Unexpected situation happened. process_CIPCLOSE() return %d.\n", result);
				sleep(1);
				//Maintain suspend_state
				w_suspendMdmHdlr();
    }
    break;
  } // case SUSPEND_STATE_CIPCLOSE
  case SUSPEND_STATE_NETCLOSE:
  {
    int result;

__SUSPEND_STATE_NETCLOSE_READER_AGAIN__:
			ret = reader(at_config.serial_fd, &rsp);
			if(ret == 0){
				LOG("ERROR: reader() error.\n");
				RESET_GLOBAL_VAR;
				return;
			}else if(ret == 2){
				if(*ATBufCur == '\0'){
					LOG("reader() return %d. Need to recv more rsp.\n", ret);
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}else{
					LOG("There is still something unread in ATBuf. Continue to read more.\n");
					goto __SUSPEND_STATE_NETCLOSE_READER_AGAIN__;
				}
			}else if(ret == 3){
				//Once "AT+NETCLOSE" is executed, no URC reports will be reported. This is designed by hzc.
				LOG("ERROR: Unexpected situation happened. reader() return 3.\n");
				RESET_GLOBAL_VAR;
				return;
			}
			else if(ret == 10){
				INIT_ATBUF;
				set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
				add_evt (&at_config.AT_event, 'r');
				return;
			}else if(ret == 14){
				if(isNetclose9Recved){
					LOG("NOTICE: The rsp string \"ERROR\" behind \"+NETCLOSE: 9\" found! Go to exec \"AT+CIPCLOSE=0\" again.\n");
					isNetclose9Recved = false;
					
					//Maintain ChannelNum
					suspend_state = SUSPEND_STATE_CIPCLOSE;
					w_suspendMdmHdlr();
					break;
				}
				
				//Note:
				//		If cmd "AT+NETOPEN" is not executed before executing "AT+NETCLOSE",
				//	"ERROR" will be returned. (There should be string "+NETCLOSE: 2" in rsp.)
				//	No matter isWaitRspError is true or not.
				isWaitRspError = false;
				//Restoration
				ret = 16;
				memset(RespArray, 0, sizeof(RespArray));
				memcpy(RespArray, "+NETCLOSE: 2", sizeof("+NETCLOSE: 2")-1);
				rsp = RespArray;
			}else if(ret == 16){
				if(*ATBufCur == '\0' || \
					(*ATBufCur != '\0' && (strncmp("ERROR", ATBufCur, sizeof("ERROR")-1) != 0)) \
				){
					LOG("The rsp \"ERROR\" behind \"+NETCLOSE: 2\" is missed.");
					isWaitRspError = true;
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					return;
				}
			}
			
			if(ret == 4 && strncmp(rsp, "OK", 2) == 0){
				if(*ATBufCur != '\0'){
					LOG("There is still something in ATBuf. Try to recv more.\n");
					goto __SUSPEND_STATE_NETCLOSE_READER_AGAIN__;
				}else{
					#ifndef CORRECTION_NO_URC_FOR_NETCLOSE
						LOG("\"OK\" recved. Need to recv more rsp.\n");
						INIT_ATBUF;
						set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
						add_evt (&at_config.AT_event, 'r');
						return;
					#else
						LOG("\"OK\" recved. Enter subsequent process.\n");
						result = 0;
						if(isWaitToPwrDwn)
							goto __NETCLOSE_SUBSEQUENT_HANDLING_1__; // Ugly coding style
						else
							goto __NETCLOSE_SUBSEQUENT_HANDLING_2__;
					#endif	
				}
			}
			
__NETCLOSE_SUBSEQUENT_HANDLING_1__:
			if(isWaitToPwrDwn){
				LOG("suspend_state=%d, isWaitToPwrDwn=%d. Now, go to shut down app.\n",
						suspend_state, isWaitToPwrDwn);
				isWaitToPwrDwn = false;
				
				ACQ_CHANNEL_LOCK_WR;
				if(4 == ChannelNum){
					isMifiConnected = false;
					ChannelNum = -1;
				}else if(2 == ChannelNum){
					isMifiConnected = false;
					ChannelNum = -1;
					
					RcvrMdmEmodemConn = true;
				}else if(-1 != ChannelNum){ // Fixed by rjf at 20150810
					LOG("ERROR: Wrong ChannelNum(%d) found! suspend_state(%d) dismatch with ChannelNum.\n",
							ChannelNum, suspend_state);
					RESET_GLOBAL_VAR;
					return;
				}
				RLS_CHANNEL_LOCK;
				
				isNeedToSendOutReq0x65 = true;
				
			}

    //ret = 1
    //+NETCLOSE: 0 and +NETCLOSE: 2 are handled below.
    result = process_NETCLOSE(rsp);
__NETCLOSE_SUBSEQUENT_HANDLING_2__:
    if(result == 0 || result == 2 || result == 256)
    {
      if(result == 0){
        LOG("\"AT+NETCLOSE\" succeeded.\n");
      }else if(result == 2){
        LOG("PDP has been deactivated already.\n");
      }else{
        LOG("PDP has been deactivated passively.\n");
      }

      LOG("isRegOffForPoorSig=%d, RcvrMdmEmodemConn=%d, isServConnClosed=%d, "
          "isTermSuspNow=%d, isTermRcvrNow=%d.\n",
            isRegOffForPoorSig, RcvrMdmEmodemConn, isServConnClosed, 
            isTermSuspNow, isTermRcvrNow
      );

      if(true == isTermSuspNow && true == isTermRcvrNow){
        LOG("ERROR: isTermSuspNow=1 and isTermRcvrNow=1!\n");
        RESET_GLOBAL_VAR;
        break;
      }

// add by jack li 20161123 start
#ifdef FEATURE_ENABLE_F9_CIPCLOSE	

 LOG("Debug: g_closesocket_F9 = %d .\n",g_closesocket_F9);
 LOG("Debug: isRegOffForPoorSig = %d .\n",isRegOffForPoorSig);
	  if (g_closesocket_F9 == true){

		isRegOffForPoorSig = false;
		g_closesocket_F9 = false;
	  }
 LOG("Debug: g_closesocket_F9 = %d .\n",g_closesocket_F9);
 LOG("Debug: isRegOffForPoorSig = %d .\n",isRegOffForPoorSig);
#endif
// add by jack li 20161123 end

      if( false == isRegOffForPoorSig 
        && false == isTermSuspNow
        && false == isTermRcvrNow 
        && false == RcvrMdmEmodemConn 
        && false == isServConnClosed)
      {
        //Sometimes, it has been false already, but it's fine to exec operation below.
        isSockListenerRecvPdpActMsg = false; //(This spot is where pdp act handler ends.)

#ifndef FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
        mdmEmodem_set_standby();
        isMdmEmodemRunning = false;
        LOG("mdmEmodem has been suspended successfully!\n");
#else

        isMdmEmodemRunning = false; // Pretend that mdmEmodem is shut down
        LOG("The network link (of mdmEmodem) has been cleared successfully!\n");

	#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
		// BaseTimeOutDate();
	#endif
#endif // !FEATURE_MDMEmodem_CANCEL_STANDBY_MODE

#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810

	g_qlinkdatanode_run_status_01 = CPIN_READY_OK_AFTER;
	LOG("g_qlinkdatanode_run_status_01 =%d.\n", g_qlinkdatanode_run_status_01);

#endif

#ifdef FEATURE_ENABLE_3GONLY

	LOG("01 isneedtoset3Gonly =%d.\n", isneedtoset3Gonly);

	if (isneedtoset3Gonly == true)
	{
		LOG("DEBUG:isneedtoset3Gonly =%d.\n", isneedtoset3Gonly);
		pthread_mutex_lock(&acq_nas_inf_mtx);
      		notify_NasSender(0x04);
      		while(0 == nas_inf_acqed){
       	 pthread_cond_wait(&acq_nas_inf_cnd, &acq_nas_inf_mtx);
     		 }
     		 nas_inf_acqed = 0; // Reset it for using it again
     		 pthread_mutex_unlock(&acq_nas_inf_mtx);
      
	}

#endif


#ifdef FEATURE_ENABLE_STATUS_REPORT_10_MIN	

	if(count_var == false){
		count_var = true;
		notify_Sender_status(0x04);
		msq_send_IMEI_to_update();
	}

#endif


#ifndef FEATURE_MDMEmodem_CANCEL_STANDBY_MODE //xiaobin add
        //Situation Presentation:
        //Although isTermSuspNow equaled true before mdmEmodem_set_standby() is called, app would wait for 2s
        //to make standby mode take effect. So isTermSuspNow might be set true during this period.
        if(isTermSuspNow)
        {
          LOG("NOTICE: isTermSuspNow = 1! Now, go to resume mdmEmodem.\n");
          isTermSuspNow = false;

          ACQ_CHANNEL_LOCK_WR;
          if(2 == ChannelNum){
            ChannelNum = 4;
          }else if(-1 != ChannelNum){
            LOG("ERROR: Wrong ChannelNum(%d) found! suspend_state(%d) dismatch with ChannelNum.\n",
                  ChannelNum, suspend_state);
            RESET_GLOBAL_VAR;
            break;
          }
          RLS_CHANNEL_LOCK;

          suspend_state = SUSPEND_STATE_MAX;
          w_resumeMdmHdlr();
          break;
        }
#endif // !FEATURE_MDMEmodem_CANCEL_STANDBY_MODE

        ACQ_CHANNEL_LOCK_WR;
        if(2 == ChannelNum){
          ChannelNum = 1;
        }
        // Fixed by rjf at 20150810 (Once "+CGREG: 2" recved and no obvious registration fall was found, it would go here with ChannelNum equaled -1.)
        else if(-1 != ChannelNum){
          LOG("ERROR: Wrong ChannelNum(%d) found! suspend_state(%d) dismatch with ChannelNum.\n",
                    ChannelNum, suspend_state);
          RESET_GLOBAL_VAR;
          break;
        }
        RLS_CHANNEL_LOCK;

#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
        msq_send_qlinkdatanode_run_stat_ind(17);
#endif

        suspend_state = SUSPEND_STATE_MAX;
        ACQ_ACQ_DATA_STATE_MTX;
        set_at_evt_waiting();
        RLS_ACQ_DATA_STATE_MTX;
      }else if(isTermSuspNow)
      {
					LOG("isTermSuspNow = 1. Now cancel the suspension of mdmEmodem and resume mdmEmodem.\n");
					isTermSuspNow = false;
					RcvrMdmEmodemConnWhenSuspTerm = false; //Reset it anyway
					
					ACQ_CHANNEL_LOCK_WR;
					if(2 == ChannelNum){
						if(isRegOn)
							ChannelNum = 4;
						else
							ChannelNum = -1;
					}else{
						LOG("ERROR: Wrong ChannelNum(%d) found! isTermSuspNow=1.\n", ChannelNum);
						RESET_GLOBAL_VAR;
						break;
					}
					RLS_CHANNEL_LOCK;

					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(16);
					#endif
					
					//isMdmEmodemRunning must be true.
					suspend_state = SUSPEND_STATE_MAX;
					//No matter if RcvrMdmEmodemConnWhenSuspTerm is true or not.
					chk_net_state = CHK_NET_STATE_CSQ;
					w_checkNetHdlr();
				}
				else if(isRegOffForPoorSig){
					if(isTermRcvrNow){
						isTermRcvrNow = false; // No need to terminate recovering process at this point.
					}
					if(RcvrMdmEmodemConn){
            RcvrMdmEmodemConn = false; // Process the recovering flow of isRegOffForPoorSig
					}

					if(isServConnClosed)
						isServConnClosed = false;
					
					//Offline ind sent already.
					set_ChannelNum(-1);

					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(16);
					#endif
					
					//isMdmEmodemRunning must be true.
					suspend_state = SUSPEND_STATE_MAX;
					chk_net_state = CHK_NET_STATE_CSQ;
					w_checkNetHdlr();
      }else if(RcvrMdmEmodemConn)
      {
				LOG("suspend_state=%d, RcvrMdmEmodemConn=1.\n",
						suspend_state);
				RcvrMdmEmodemConn = false;
				
				if(isTermRcvrNow)
					isTermRcvrNow = false; // No need to terminate recovering process at this point.
				

				if(isServConnClosed)
					isServConnClosed = false;
				
				//ChannelNum is set even before RcvrMdmEmodemConn is set. (20150814)

				#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
          msq_send_qlinkdatanode_run_stat_ind(16);
				#endif
				
				suspend_state = SUSPEND_STATE_MAX;
				chk_net_state = CHK_NET_STATE_CSQ;
				w_checkNetHdlr();
      }else if(isServConnClosed)
      {
					isServConnClosed = false;
					
					#ifdef FEATURE_ENABLE_MSGQ
						msq_send_offline_ind();
					#endif

					#ifdef FEATURE_ENHANCED_qlinkdatanode_RUN_STAT_IND
						msq_send_qlinkdatanode_run_stat_ind(16);
					#endif
					
					suspend_state = SUSPEND_STATE_MAX;
					chk_net_state = CHK_NET_STATE_CSQ;
					w_checkNetHdlr();
			}
      else
      {
        LOG("do none\n");
      }
    }else if(9 == result)
    {
				LOG("Socket of mdmEmodem is still open. Try to close it again.\n");				
				if(*ATBufCur == '\0'){
					LOG("Not found \"ERROR\" behind \"+NETCLOSE: 9\". Continue to recv more.\n");
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
					isNetclose9Recved = true;
					break;
				}

				//Maintain ChannelNum
				suspend_state = SUSPEND_STATE_CIPCLOSE;
				w_suspendMdmHdlr();
    }else if(-3 == result){
				if(*ATBufCur == '\0'){
					LOG("process_NETCLOSE() return -3. Need to recv more rsp.\n");
					INIT_ATBUF;
					set_evt(&at_config.AT_event, at_config.serial_fd, false, r_suspendMdmHdlr, NULL);
					add_evt (&at_config.AT_event, 'r');
				}else{
					LOG("process_NETCLOSE() return -3. There is still something unread in ATBuf. Continue to read more.\n");
					goto __SUSPEND_STATE_NETCLOSE_READER_AGAIN__;
				}
    }else{
      LOG("ERROR: Unexpected situation happened. process_NETCLOSE return %d.\n", result);
      sleep(1);
      //Maintain suspend_state
      w_suspendMdmHdlr();
    }
    break;
  } // case SUSPEND_STATE_NETCLOSE

  case SUSPEND_STATE_IDLE:
  default:
  {
    LOG("ERROR: Wrong case entered. suspend_state=%d.\n", suspend_state);
    break;
  }
  } // switch(suspend_state)

}

void w_resumeMdmHdlr(void)
{

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

   LOG("Debug:   isCancelRecoveryNet = %d \n",isCancelRecoveryNet);

   if (isCancelRecoveryNet == true){

	LOG("NOTICE: Don't resume Emodem..\n");
    	return;

   }


#endif


#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
  if(isMdmEmodemControledByYydaemon){
    LOG("NOTICE: isMdmEmodemControledByYydaemon=1. w_suspendMdmHdlr() failed.\n");
    return;
  }
#endif //FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
  
#ifndef FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
  if(1 != mdmEmodem_set_awake())
  {
    return;
  }
#endif //!FEATURE_MDMEmodem_CANCEL_STANDBY_MODE
  
  isMdmEmodemRunning = true;

  pthread_mutex_lock(&i1r_mtx);
  is1stRunOnMdmEmodem = false;
  pthread_mutex_unlock(&i1r_mtx);
  
  isProcOfRecvInitInfo = false;

  LOG("NOTICE: mdmEmodem has been waken up. Now, try to connect to server.\n");
  
  chk_net_state = CHK_NET_STATE_CSQ;
  w_checkNetHdlr();

  return;
}

#ifdef FEATURE_ENABLE_TIMER_SYSTEM
static void BootInfo_Timeout_cb(void *userdata)
{
  LOG6("~~~~ BootInfo_Timeout_cb ~~~~\n");
  //chk_net_state is CHK_NET_STATE_IDLE now. No need to set it here.
  clr_read_buf_v02(1, 0);
  isProcOfRecvInitInfo = false;
  w_checkNetHdlr();
}
#endif //FEATURE_ENABLE_TIMER_SYSTEM

//Description:
//opt:
// 0: Start mdmEmodem.
// 1: Restart mdmEmodem.
// 2: Start mdmEmodem again for failing to start mdmEmodem at first.
static void openDev(char opt)
{
  int isMdmEmodemStarted = -99;
  struct timeval tv;

  if(opt > 2 || opt < 0){
    LOG("ERROR: Wrong param input. opt=%d.\n", (int)opt);
    RESET_GLOBAL_VAR;
    return;
  }

  LOG6("~~~~ openDev (%d) ~~~~\n", (int)opt);

  if(0 == opt)
  { 
    at_config.serial_fd = -1;
  }

  while(at_config.serial_fd < 0)
  {
    if(device_path != NULL)
    {
      at_config.serial_fd = open(device_path, O_RDWR | O_NONBLOCK | O_NOCTTY);

      if(at_config.serial_fd >= 0)
      {
        struct termios old_termios;
        struct termios new_termios;

        if(tcgetattr(at_config.serial_fd, &old_termios) != 0){
          LOG("tcgetattr() error. errno=%s(%d).\n", strerror(errno), errno);
          return;
        }
#ifdef FEATURE_ENABLE_CANCEL_LOG_OUTPUT  //20160708
        LOG("old_termios.c_lflag=0x%x.\n", old_termios.c_lflag);
        LOG("old_termios.c_cflag=0x%x.\n", old_termios.c_cflag);
        LOG("old_termios.c_iflag=0x%x.\n", old_termios.c_iflag);
        LOG("old_termios.c_oflag=0x%x.\n", old_termios.c_oflag);
#endif
        new_termios = old_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
        new_termios.c_cflag |= (CREAD | CLOCAL);
        new_termios.c_cflag &= ~(CSTOPB | PARENB);
        new_termios.c_cflag &= ~(CBAUD | CSIZE) ;
        new_termios.c_cflag |= CRTSCTS;
        new_termios.c_cflag |= CS8;
        cfsetispeed(&new_termios, B115200);
        cfsetospeed(&new_termios, B115200);
        new_termios.c_iflag &= ~(INLCR | ICRNL | IGNCR);
        new_termios.c_oflag &= ~(ONLCR | OCRNL | ONOCR | ONLRET); 
        new_termios.c_cc[VSTOP] = _POSIX_VDISABLE;
        new_termios.c_cc[VSTART] = _POSIX_VDISABLE;

#ifdef FEATURE_ENABLE_CANCEL_LOG_OUTPUT  //20160708
        LOG("new_termios.c_lflag=0x%x.\n", new_termios.c_lflag);
        LOG("new_termios.c_cflag=0x%x.\n", new_termios.c_cflag);
        LOG("new_termios.c_iflag=0x%x.\n", new_termios.c_iflag);
        LOG("new_termios.c_oflag=0x%x.\n", new_termios.c_oflag);

#endif

        tcsetattr(at_config.serial_fd, TCSANOW, &new_termios);

        LOG("serial_fd = %d.\n", at_config.serial_fd);
      }
    }else
    {
      LOG("ERROR: No device_path found! device_path=null.\n");
      return;
    }

    if(at_config.serial_fd < 0){
      LOG("open() failed. errno=%d. Retry opening device...\n", errno);
      usleep(10000);
      /* never returns */
    }
  }
  

  if(0 == opt || 2 == opt)
  {

#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_STARTUP
__CHECK_MDMEmodem_STARTUP_AGAIN__:
    isMdmEmodemStarted = mdmEmodem_check_startup();
    if(isMdmEmodemStarted == 0)
    {
      LOG("mdmEmodem start test 02 ..\n");
      msq_send_qlinkdatanode_run_stat_ind(21); //	0: mdmEmodem is power off. by jack li 2016 0303
      if(0 == opt)
      {
        LOG("mdmEmodem start test 01 ..\n");
        mdmEmodem_init(0);
	 msq_send_qlinkdatanode_run_stat_ind(22); //	0: mdmEmodem is power on. by jack li 2016 0303
      }else if(2 == opt)
      {
        //Fixed by rjf at 20151019
        //Not need to pull gpio "power key". If you did pull it, mdmEmodem may be shut down.
        //mdmEmodem_init(1);
        LOG("mdmEmodem is not started up! Wait for 1 sec...\n");
        poll(NULL, 0, 1000);
        goto __CHECK_MDMEmodem_STARTUP_AGAIN__;
      }
    }else if(isMdmEmodemStarted == -1){
      goto __CHECK_MDMEmodem_STARTUP_AGAIN__;
    }
#endif

#ifdef FEATURE_MDMEmodem_BOOT_STAT_QRY
    write_file(MDMEmodem_BOOT_STAT_FILE_PATH, 0, "1");
#endif
  }

  

  tv.tv_sec = MAX_STARTUP_TIMER_SEC;
  tv.tv_usec = MAX_STARTUP_TIMER_USEC;
  set_evt(&at_config.BootInfo_evt, -1, false, BootInfo_Timeout_cb, NULL);
  add_timer(&at_config.BootInfo_evt, &tv);
  trigger_EventLoop();
  
  LOG("NOTICE: Boot info event=%x.\n", (unsigned int)(&at_config.BootInfo_evt));

  return;
}

//Return Values:
// 1: success
// 0: error
int startSerialComm(int argc, char **argv)
{
  int opt;
  char server_addr[MAX_SERV_ADDR_CHARS] = {'\0'};

  if(argc != 5){
    if(argc == 6)
    {
      isUimPwrDwn = true;
    }else
    {
      LOG("ERROR: Wrong params input. argc=%d.\n", argc);
      return 0;
    }
  }

  while(-1 != (opt = getopt(argc, argv, "d:s:z"))){
    switch(opt){
    case 'd':{
      memcpy(device_path, optarg, strlen(optarg));
      LOG("device path=%s.\n", device_path);
      break;
    }
    case 's':{
      memcpy(server_addr, optarg, strlen(optarg));
      LOG("server address=%s.\n", server_addr);
      break;
    }
    case 'z':{
      LOG("we donot care.\n");
      break;
    }
    case '?':{
      LOG("ERROR: Wrong param(s) input.\n");
      return 0;
    }
    default:{
      LOG("ERROR: Wrong option(s) input.\n");
      return 0;
    }
    }
  }

  if(device_path[0] == '\0'){
      LOG("ERROR: Wrong device path input.\n");
      return 0;
  }
  
  if(server_addr[0] == '\0'){
    LOG("ERROR: Wrong server IP address input.\n");
    return 0;
  }

  if( !check_dev_addr(device_path) ){
    LOG("ERROR: check_dev_addr() error.\n");
    return 0;
  }
  
  memset(IP_addr, 0, 16);
  if(!parse_serv_addr(server_addr, IP_addr, &server_port)){
    LOG("ERROR: parse_serv_addr() error.\n");
    return 0;
  }
  
  if( !check_IP_addr(IP_addr) ){
    LOG("ERROR: check_IP_addr() error.\n");
    return 0;
  }

  if( !check_port(server_port) ){
    LOG("ERROR: check_port() error.\n");
    return 0;
  }

  #ifdef FEATURE_ENABLE_EmodemUDP_LINK

  //strncpy(Udp_IP_addr,IP_addr, 15);
 // udp_server_port = server_port;

   // IP1 *********/
  Udp_IP_addr[0] = 0x34;
  Udp_IP_addr[1] = 0x37;
  
  Udp_IP_addr[2] = 0x2e;

  Udp_IP_addr[3] = 0x39;
  Udp_IP_addr[4] = 0x30;

  Udp_IP_addr[5] = 0x2e;

  Udp_IP_addr[6] = 0x38;
  Udp_IP_addr[7] = 0x39;


  Udp_IP_addr[8] = 0x2e;
		
  Udp_IP_addr[9] = 0x34;
  Udp_IP_addr[10] = 0x33;



  udp_server_port = 9734;
  //udp_server_port = 9111;

  
  Udp_IP_addr_default1[0] = 0x34;
  Udp_IP_addr_default1[1] = 0x37;
  
  Udp_IP_addr_default1[2] = 0x2e;

  Udp_IP_addr_default1[3] = 0x39;
  Udp_IP_addr_default1[4] = 0x30;

  Udp_IP_addr_default1[5] = 0x2e;

  Udp_IP_addr_default1[6] = 0x35;
  Udp_IP_addr_default1[7] = 0x37;
  Udp_IP_addr_default1[8] = 0x2e;
		
  Udp_IP_addr_default1[9] = 0x31;
  Udp_IP_addr_default1[10] = 0x36;
  Udp_IP_addr_default1[11] = 0x30;

  udp_server_port_default1 = 9734;


  Udp_IP_addr_default2[0] = 0x31;
  Udp_IP_addr_default2[1] = 0x31;
  Udp_IP_addr_default2[2] = 0x36;
  
  Udp_IP_addr_default2[3] = 0x2e;

  Udp_IP_addr_default2[4] = 0x36;
  Udp_IP_addr_default2[5] = 0x32;

  Udp_IP_addr_default2[6] = 0x2e;

  Udp_IP_addr_default2[7] = 0x34;
  Udp_IP_addr_default2[8] = 0x33;
  Udp_IP_addr_default2[9] = 0x2e;
		
  Udp_IP_addr_default2[10] = 0x36;
  Udp_IP_addr_default2[11] = 0x33;


  udp_server_port_default2 = 9734;


  Udp_IP_addr_default3[0] = 0x34;
  Udp_IP_addr_default3[1] = 0x37;
  
  Udp_IP_addr_default3[2] = 0x2e;

  Udp_IP_addr_default3[3] = 0x38;
  Udp_IP_addr_default3[4] = 0x39;

  Udp_IP_addr_default3[5] = 0x2e;

  Udp_IP_addr_default3[6] = 0x34;
  Udp_IP_addr_default3[7] = 0x30;
  Udp_IP_addr_default3[8] = 0x2e;
		
  Udp_IP_addr_default3[9] = 0x32;
  Udp_IP_addr_default3[10] = 0x33;
  Udp_IP_addr_default3[11] = 0x36;

  udp_server_port_default3 = 9734;


  // IP1 *********/
  tcp_IP_addr[0] = 0x35;
  tcp_IP_addr[1] = 0x38;
  
  tcp_IP_addr[2] = 0x2e;

  tcp_IP_addr[3] = 0x39;
  tcp_IP_addr[4] = 0x36;

  tcp_IP_addr[5] = 0x2e;

  tcp_IP_addr[6] = 0x31;
  tcp_IP_addr[7] = 0x38;
  tcp_IP_addr[8] = 0x38;

  tcp_IP_addr[9] = 0x2e;
		
  tcp_IP_addr[10] = 0x31;
  tcp_IP_addr[11] = 0x39;
  tcp_IP_addr[12] = 0x37;


  tcp_server_port = 9988;
  

  // IP2 *********/
  tcp_IP_addr_2[0] = 0x34;
  tcp_IP_addr_2[1] = 0x37;
  
  tcp_IP_addr_2[2] = 0x2e;

  tcp_IP_addr_2[3] = 0x39;
  tcp_IP_addr_2[4] = 0x30;

  tcp_IP_addr_2[5] = 0x2e;

  tcp_IP_addr_2[6] = 0x38;
  tcp_IP_addr_2[7] = 0x39;
  tcp_IP_addr_2[8] = 0x2e;


  tcp_IP_addr_2[9] = 0x34;
  tcp_IP_addr_2[10] = 0x33;



  tcp_server_port_2 = 9988;

#endif


  openDev(0);
  

  return 1;
}
