#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "msq_qlinkdatanode.h"
#include "common_qlinkdatanode.h"
#include "socket_qlinkdatanode.h" // Udp_Config, Evt
#include "protocol_qlinkdatanode.h" // Flow_Info
#include "feature_macro_qlinkdatanode.h"
#include "qmi_sender_qlinkdatanode.h"

/******************************External Functions*******************************/
extern void at_send_pd_msg_to_srv(void);
extern void at_assembly_pd_msg(void);
extern void msq_send_pd_rsp(void);
extern int mdmEmodem_check_busy(void);
extern void clr_all_timers(void);
extern void clr_timeout_evt(Evt *tev);
extern void notify_Sender(char opt);
extern void notify_EvtLoop(const char msg);
extern void push_ChannelNum(const int chan_num);
//extern int proto_update_flow_state(Flow_State state);
extern void add_timer_power_down_notification_timeout(void);
extern int bit_set(const int idx, int bit_val, int * p_src_val);
extern int WriteFluValueToFile(long long ttvalue,unsigned char ucfile_mode);


/******************************External Variables*******************************/
extern long long flu_maxValue_current;
extern int ChannelNum;
#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
extern pthread_rwlock_t ChannelLock;
#else
extern pthread_mutex_t ChannelLock;
#endif

extern pthread_mutex_t evtloop_mtx;
extern pthread_mutex_t acq_data_state_mtx;

extern Dev_Info rsmp_dev_info;
#ifdef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
extern Time_Zone_S timezone_qlinkdatanode;
#endif //FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
extern Suspend_Mdm_State suspend_state;
extern bool isWaitToSusp;
extern bool isTermSuspNow;
extern bool isMdmEmodemRunning;
extern bool isWaitToRcvr;
extern bool isTermRcvrNow;

extern bool isMifiConnected;

extern unsigned long thread_id_array[THD_IDX_MAX];

extern unsigned char g_qlinkdatanode_run_status_01;
extern unsigned char g_qlinkdatanode_run_status_02;

extern unsigned char apnMessageBuff[200];

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

	extern bool isCancelRecoveryNet;

#endif


/******************************Local Variables*******************************/
static int msgloopStarted = 0;
static pthread_t msgloop_tid;
static pthread_mutex_t msgloop_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t msgloop_cond = PTHREAD_COND_INITIALIZER;

static int snd_qid = -1;
static int rcv_qid = -1; 
static int ind_qid = -1; 
static Ext_Ind_Msg_S_Type tz_ind_msg;
static Common_Proc_Msg_S_Type ind_msg;

#ifdef FEATURE_ENABLE_SEND_MSGTO_UPDATE
static int snd_up_qid = -1; 
#endif

/******************************Global Variables*******************************/
//Function:
//When mdmEmodem is poweroff, isMdmEmodemPwrDown is true.
#ifdef FEATURE_ENABLE_MDMEmodem_SOFT_STARTUP
bool isMdmEmodemPwrDown = true;
#else
bool isMdmEmodemPwrDown = false;
#endif

//Function:
//Once app has recved reset msg, it will be set true. After recving ind of completing removing local USIM data,
//it will be reset false.
bool isRstReqAcq = false;

//Function:
//Once app recvs pwr-down req msg from yy_daemon, it will be set true. And it doesn't need to be reset false 
//then.
bool isFTMPowerOffInd = false;

//Function:
//Once app recvs pwr-down rsp msg from server, it will be set true. And it doesn't need to be reset false then.
bool isPwrDwnRspAcq = false;

//Function:
//If app is trying to shut down itself and mdmEmodem is busy, it will be set true.
bool isWaitToPwrDwn = false;

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
//Description:
//Once the app qlinkdatanode recved the proc msg of requesting to ctrl the mdmEmodem standby mode, this bool var will
//be set true, it can't be recovered.
bool isMdmEmodemControledByYydaemon = false;
Proc_Msg_At_Cmd_Cfg_S_Type proc_msg_at_config;
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
#ifdef  FEATURE_ENABLE_FLU_LIMIT
bool isgetNptByYydaemon = false;
#endif

pthread_mutex_t pwr_inf_qry_mtx;
pthread_cond_t pwr_inf_qry_cnd;
int pwr_inf_acqed = 0;

bool isNeedToSendOutReq0x65 = false;

//Description:
//This bool var is used as a conditional var to notify Sender to upload IMSI to other process (like yy_daemon)
//after acquiring the IMSI of mdm9215.
bool isUploadImsiToOtherProc = false;

int qlinkdatanode_run_stat = 0;

static int msg_queue_init(key_t qkey)
{
  int qid = -1;
  
  qid = msgget(qkey, IPC_CREAT | 0666);
  if(qid < 0){
    LOG("ERROR: msgget() failed. errno=%d.\n", errno);
    return -1;
  }else{
    //LOG("Msg queue init completes. qid=%d.\n", qid);
  }
  
  return qid;
}

void msq_send_pd_msg_to_srv(void)
{
  at_assembly_pd_msg();
  at_send_pd_msg_to_srv();
}

//Param:
//len : The length of msg data.
//Ret:
//  0: msgsnd() succeed.
// -1: Wrong param input.
// -2: msgsnd() failed.
//other: Unknown results.
static int msq_send_msg_to_proc(int qid, void *ptr, unsigned int len)
{
  int ret;

  if(qid < 0 || ptr == NULL || len < 0){
    LOG("ERROR: Wrong param found. qid=%d, ptr=%lu, len=%u.\n", 
            qid, (unsigned long)ptr, len);
    return -1;
  }

__RESEND_MSG_TO_PROC__:
  ret = msgsnd(qid, ptr, len, IPC_NOWAIT);
  if(ret == -1){
    if(errno == EINTR){
      goto __RESEND_MSG_TO_PROC__;
    }else{
      LOG("ERROR: msgsnd() error. errno=%s(%d).\n",
                          strerror(errno), errno);
      return -2;
    }
  }

  return ret;
}

void msq_send_online_ind(void)
{
#ifndef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
  ind_msg.msg_id = YY_PROCESS_MSG_ONLINE_IND;
  ind_msg.msg_data = 0x01;
  msq_send_msg_to_proc(ind_qid, (void *)(&ind_msg), 1);
#else
  tz_ind_msg.msg_id = YY_PROCESS_MSG_ONLINE_IND;
  memset(tz_ind_msg.tz_v, 0, 3);
  memcpy(tz_ind_msg.tz_v, timezone_qlinkdatanode.val, 3);
  LOG("Time zone info: 0x%02x 0x%02x 0x%02x.\n",
                  (unsigned int)tz_ind_msg.tz_v[0], (unsigned int)tz_ind_msg.tz_v[1], (unsigned int)tz_ind_msg.tz_v[2]);
  msq_send_msg_to_proc(ind_qid, (void *)(&tz_ind_msg), 3);
#endif
}

void msq_send_offline_ind(void)
{
#ifndef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
  ind_msg.msg_id = YY_PROCESS_MSG_OFFLINE_IND;
  ind_msg.msg_data = 0x00;
  msq_send_msg_to_proc(ind_qid, (void *)(&ind_msg), 1);
#else
  tz_ind_msg.msg_id = YY_PROCESS_MSG_OFFLINE_IND;
  memset(tz_ind_msg.tz_v, 0, 3);
  memcpy(tz_ind_msg.tz_v, timezone_qlinkdatanode.val, 3);
  LOG("Time zone info: 0x%02x 0x%02x 0x%02x.\n",
    (unsigned int)tz_ind_msg.tz_v[0], (unsigned int)tz_ind_msg.tz_v[1], (unsigned int)tz_ind_msg.tz_v[2]);
  msq_send_msg_to_proc(ind_qid, (void *)(&tz_ind_msg), 3);
#endif
}

void msq_send_dialup_ind(void)
{
  LOG6("~~~~ msq_send_dialup_ind ~~~~\n");
  ind_msg.msg_id = YY_PROCESS_MSG_DIALUP_IND;
  ind_msg.msg_data = 0x01;
  msq_send_msg_to_proc(ind_qid, (void *)(&ind_msg), 1);
}


#define qlinkdatanode_RUN_STAT_BIT_SIG (5)
#define qlinkdatanode_RUN_STAT_BIT_REG (6)
#define qlinkdatanode_RUN_STAT_BIT_PDP_ACT (7)
#define qlinkdatanode_RUN_STAT_BIT_SERV_CONN (8)

#define qlinkdatanode_RUN_STAT_BIT_MDMEmodemPOWEROFF (9)
#define qlinkdatanode_RUN_STAT_BIT_CPINSTATUS (10)
#define qlinkdatanode_RUN_STAT_BIT_RECEIVEDATA (11)
#define qlinkdatanode_RUN_STAT_BIT_7100_CGREG2 (12)

//#define qlinkdatanode_RUN_STAT_BIT_SERV_CONN (11)
//#define qlinkdatanode_RUN_STAT_BIT_SERV_CONN (12)


//Parameters:
//	Description:
//		Indications for mdm9215:
//		 Evt0   _ _ _ _ _ 0 0 0 : ID auth failed. IMEI not applicable.
//		 Evt1   _ _ _ _ _ 0 0 1 : ID auth succeeded. Rm local virt USIM data and download new one.
//		 Evt2   _ _ _ _ _ 0 1 0 : ID auth succeeded. Utilize local virt USIM data.
//		 Evt3   _ _ _ _ _ 0 1 1 : ID auth succeeded. No remote USIM card is available in server. Retry to req.
//		 Evt4   _ _ _ _ _ 1 0 0 : ID auth succeeded. Download new USIM data and cover the local one.
//		 Evt5   _ _ _ _ _ 1 0 1 : Local USIM data startup failed. (sim err)
//		 Evt6   _ _ _ _ _ 1 1 0 : Local USIM data suspended. (power down)
//		 Evt7   _ _ _ _ _ 1 1 1 : Local USIM data startup succeeded. (sim ready)
//		Indications for mdmEmodem:
//		 Evt8   _ _ _ 0 _ _ _ _ : Weak signal or unknown. (rssi <= 3 or rssi == 99 or not apply AT+CSQ)
//		 Evt9   _ _ _ 1 _ _ _ _ : Fine signal. (rssi > 3 and rssi != 99)
//		 Evt10  _ _ 0 _ _ _ _ _ : Reg failed or unknown. (AT+CGREG? "failed" or not apply AT+CGREG?)
//		 Evt11  _ _ 1 _ _ _ _ _ : Reg succeeded. (AT+CGREG? "succeeded".)
//		 Evt12  _ 0 _ _ _ _ _ _ : PDP act failed or unknown. (AT+NETOPEN failed or not apply AT+NETOPEN)
//		 Evt13  _ 1 _ _ _ _ _ _ : PDP act succeeded. (AT+NETOPEN succeeded.)
//		 Evt14  0 _ _ _ _ _ _ _ : Server TCP conn failed or unknown. (AT+CIPOPEN failed or not apply AT+CIPOPEN)
//		 Evt15  1 _ _ _ _ _ _ _ : Server TCP conn succeeded. (AT+CIPOPEN succeeded.)
//
//		 Evt16  0 0 0 0 _ _ _ _ : Restore MdmEmodem.
//		 Evt17  fffffffe : @YOUYOU
//            Evt18   _ _ _ _ 1 0 0 0 : ID auth failed. IMEI not applicable.  // add  by jack  li 2016/02/18
//            Evt19   _ _ _ _ 1 0 0 1 : the device doesn't find at the server list.  // add  by jack  li 2016/02/18
//            Evt20   _ _ _ _ 1 0 1 0 : Receive data is 98 62.  // add  by jack  li 2016/02/29 FA
//            Evt21   _ _ _1  _ _ _ _  _ _ _ _ : MDMEmodem power off.  // add  by jack  li 2016/03/03 
//            Evt22   _ _ _0  _ _ _ _  _ _ _ _ : MDMEmodem power on.  // add  by jack  li 2016/03/03 
//            Evt23   _ _ 1_  _ _ _ _  _ _ _ _ : : cpin error.  // add  by jack  li 2016/03/03 
//            Evt24   _ _ 0_  _ _ _ _  _ _ _ _ : : cpin ok.  // add  by jack  li 2016/03/03 
//            Evt25   _ _ _ _ 1 0 1 1 : APDU auth.  // add  by jack  li 2016/10/11 FB
//            Evt26   _ _ _ _ 1 1 0 0 : NULL.  // add  by jack  li 2016/10/11 FC
//            Evt27   _ 1 _  _  _ _ _ _  _ _ _ _ : Don't get data from server.  // add  by jack  li 2016/12/01
//            Evt28   _ 0 _  _  _ _ _ _  _ _ _ _ : Get data from server.  // add  by jack  li 2016/12/01 
//            Evt29   1 _ _ _  _  _ _ _ _  _ _ _ _ : PDP state :2.  // add  by jack  li 2016/12/07
//            Evt30   0 _ _ _  _ _ _ _  _ _ _ _ : PDPstate :1   // add  by jack  li 2016/12/07 


//            Evt40   xxxxxxxxxxx: out of traffic show customer  // add  by jack  li 2017/2/016

void msq_send_qlinkdatanode_run_stat_ind(int _evt)
{
  struct {
    long id;
    int data;
  }ind_msg;
  
qlinkdatanode_Run_Stat_E_Type evt = (qlinkdatanode_Run_Stat_E_Type)_evt;
bool MdmEmodem_Susp = false;

#ifdef FEATURE_ENABLE_UPDATE_STATUS

static int last_status = 0xff;

 LOG("Debug: _evt = %d \n", _evt);
 LOG("Debug: last_status = %d \n", last_status);
 
 if (last_status == _evt)
 {
 	LOG("Debug: Don't send message to yy_daemon \n", _evt);
	return;
 }

 #endif


  if(evt < qlinkdatanode_RUN_STAT_IMEI_REJ || evt >= qlinkdatanode_RUN_STAT_MAX){
    LOG("ERROR: Wrong evt (%d) found!\n", _evt);
  }

  LOG6("~~~~ msq_send_qlinkdatanode_run_stat_ind (evt: %d) ~~~~\n", _evt);

  switch(evt)
  {
		case qlinkdatanode_RUN_STAT_IMEI_REJ:{
			bit_set(1, 0, &qlinkdatanode_run_stat);
			bit_set(2, 0, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_RM_USIM:{
			bit_set(1, 1, &qlinkdatanode_run_stat);
			bit_set(2, 0, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_HLD_USIM:{
			bit_set(1, 0, &qlinkdatanode_run_stat);
			bit_set(2, 1, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_RMT_USIM_UNAVL:{
			bit_set(1, 1, &qlinkdatanode_run_stat);
			bit_set(2, 1, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_CVR_USIM:{
			bit_set(1, 0, &qlinkdatanode_run_stat);
			bit_set(2, 0, &qlinkdatanode_run_stat);
			bit_set(3, 1, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_USIM_ERR:{
			bit_set(1, 1, &qlinkdatanode_run_stat);
			bit_set(2, 0, &qlinkdatanode_run_stat);
			bit_set(3, 1, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_USIM_PD:{
			bit_set(1, 0, &qlinkdatanode_run_stat);
			bit_set(2, 1, &qlinkdatanode_run_stat);
			bit_set(3, 1, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_USIM_RDY:{
			bit_set(1, 1, &qlinkdatanode_run_stat);
			bit_set(2, 1, &qlinkdatanode_run_stat);
			bit_set(3, 1, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_SIG_WEAK:
			bit_set(qlinkdatanode_RUN_STAT_BIT_SIG, 0, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_SIG_FINE:
			bit_set(qlinkdatanode_RUN_STAT_BIT_SIG, 1, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_NOT_REG:
			bit_set(qlinkdatanode_RUN_STAT_BIT_REG, 0, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_REG:
			bit_set(qlinkdatanode_RUN_STAT_BIT_REG, 1, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_PDP_NOT_ACT:
			bit_set(qlinkdatanode_RUN_STAT_BIT_PDP_ACT, 0, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_PDP_ACT:
			bit_set(qlinkdatanode_RUN_STAT_BIT_PDP_ACT, 1, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_SERV_DISCONN:
			bit_set(qlinkdatanode_RUN_STAT_BIT_SERV_CONN, 0, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_SERV_CONN:
			bit_set(qlinkdatanode_RUN_STAT_BIT_SERV_CONN, 1, &qlinkdatanode_run_stat);break;
		case qlinkdatanode_RUN_STAT_MDMEmodem_RESTORING:{
			bit_set(qlinkdatanode_RUN_STAT_BIT_SIG, 0, &qlinkdatanode_run_stat);
			bit_set(qlinkdatanode_RUN_STAT_BIT_REG, 0, &qlinkdatanode_run_stat);
			bit_set(qlinkdatanode_RUN_STAT_BIT_PDP_ACT, 0, &qlinkdatanode_run_stat);
			bit_set(qlinkdatanode_RUN_STAT_BIT_SERV_CONN, 0, &qlinkdatanode_run_stat);
			break;
		}
		case qlinkdatanode_RUN_STAT_MDMEmodem_SUSPENDED:
			MdmEmodem_Susp = true;
     		 break;
			 
	   //  add  by jack  li 2016/02/18 f8
	  case qlinkdatanode_RUN_STAT_IMEI_REJ_SERVER:{
			bit_set(1, 0, &qlinkdatanode_run_stat);
			bit_set(2, 0, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			bit_set(4, 1, &qlinkdatanode_run_stat);
			break;
		}
	    //  add  by jack  li 2016/02/18 f9
	  case qlinkdatanode_RUN_STAT_DEVICE_NOTFIND:{
			bit_set(1, 1, &qlinkdatanode_run_stat);
			bit_set(2, 0, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			bit_set(4, 1, &qlinkdatanode_run_stat);
			break;
		}
	    //  add  by jack  li 2016/02/29 fa
	  case qlinkdatanode_RUN_STAT_DATAIS_ERROR:{
			bit_set(1, 0, &qlinkdatanode_run_stat);
			bit_set(2, 1, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			bit_set(4, 1, &qlinkdatanode_run_stat);
			break;
		}

	   //  add  by jack  li 2016/10/11 FB
	  case qlinkdatanode_RUN_STAT_AUTR_AGAIN:{
			bit_set(1, 1, &qlinkdatanode_run_stat);
			bit_set(2, 1, &qlinkdatanode_run_stat);
			bit_set(3, 0, &qlinkdatanode_run_stat);
			bit_set(4, 1, &qlinkdatanode_run_stat);
			break;
		}

	      //  add  by jack  li 2016/03/03 
	  case qlinkdatanode_RUN_STAT_MDMEmodem_POWEROFF:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_MDMEmodemPOWEROFF, 1, &qlinkdatanode_run_stat);
			break;
		}
	 //  add  by jack  li 2016/03/03 
	   case qlinkdatanode_RUN_STAT_MDMEmodem_POWERON:{
	   	
			bit_set(qlinkdatanode_RUN_STAT_BIT_MDMEmodemPOWEROFF, 0, &qlinkdatanode_run_stat);
			break;
		}
	      //  add  by jack  li 2016/03/03 
	  case qlinkdatanode_RUN_STAT_Emodem_CPIN_ERROR:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_CPINSTATUS, 1, &qlinkdatanode_run_stat);
			break;
		}
	    //  add  by jack  li 2016/03/03  
	  case qlinkdatanode_RUN_STAT_Emodem_CPIN_REDAY:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_CPINSTATUS, 0, &qlinkdatanode_run_stat);
			break;
		}
	   //  add  by jack  li 2016/12/01
	  case qlinkdatanode_RUN_STAT_DONTGETDATA:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_RECEIVEDATA, 1, &qlinkdatanode_run_stat);
			break;
		}
	 //  add  by jack  li 2016/12/01
	  case qlinkdatanode_RUN_STAT_GETDATA:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_RECEIVEDATA, 0, &qlinkdatanode_run_stat);
			break;
		}
	   //  add  by jack  li 2016/12/06
	  case qlinkdatanode_RUN_STAT_7100CGREG2:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_7100_CGREG2, 1, &qlinkdatanode_run_stat);
			break;
		}
	 //  add  by jack  li 2016/12/06
	  case qlinkdatanode_RUN_STAT_7100CGREG1:{
	  	
			bit_set(qlinkdatanode_RUN_STAT_BIT_7100_CGREG2, 0, &qlinkdatanode_run_stat);
			MdmEmodem_Susp = true;
			break;
		}

	  
	default:
    break;
  }


  if (evt == qlinkdatanode_RUN_STAT_USIM_PD){

	qlinkdatanode_run_stat &= 0xff6;
	LOG("Neet to set f6.\n");

  }else if (evt == qlinkdatanode_RUN_STAT_RM_USIM){

	qlinkdatanode_run_stat &= 0xff1;
	LOG("Neet to set f1.\n");
  }else if (evt == qlinkdatanode_RUN_STAT_HLD_USIM){

	qlinkdatanode_run_stat &= 0xff2;
	LOG("Neet to set f2.\n");
  }else if (evt == qlinkdatanode_RUN_STAT_RMT_USIM_UNAVL){

	qlinkdatanode_run_stat &= 0xff3;
	LOG("Neet to set f3.\n");
  }else if (evt == qlinkdatanode_RUN_STAT_IMEI_REJ_SERVER){

	qlinkdatanode_run_stat &= 0xff8;
	LOG("Neet to set f8.\n");
  }else if (evt == qlinkdatanode_RUN_STAT_DEVICE_NOTFIND){

	qlinkdatanode_run_stat &= 0xff9;
	LOG("Neet to set f9.\n");
  }

  
  LOG("qlinkdatanode_run_stat=0x%02x.\n", qlinkdatanode_run_stat);
  LOG("MdmEmodem_Susp=0x%02x.\n", MdmEmodem_Susp); // by jack li 20160308

  ind_msg.id = YY_PROCESS_MSG_qlinkdatanode_RUN_STAT_IND;
  if(!MdmEmodem_Susp)
    ind_msg.data = qlinkdatanode_run_stat;
  else
    ind_msg.data = 0xfffffffe;
  
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206


  LOG("Debug:   isCancelRecoveryNet = %d \n",isCancelRecoveryNet);

  if( (evt == 40) || (isCancelRecoveryNet)){

	 ind_msg.data = 0xff;
  }
#endif

 /* if ((evt == qlinkdatanode_RUN_STAT_7100CGREG1) || (evt == qlinkdatanode_RUN_STAT_MDMEmodem_SUSPENDED)){

	LOG("Neet to set  qlinkdatanode_run_stat = 0.\n");
	qlinkdatanode_run_stat = 0;
  }*/
  
  msq_send_msg_to_proc(ind_qid, (void *)(&ind_msg), 4);

#ifdef FEATURE_ENABLE_UPDATE_STATUS

  last_status = _evt;

#endif

}

void msq_send_pd_rsp(void)
{
  Common_Proc_Msg_S_Type tmpmsg = {
    .msg_id = YY_PROCESS_MSG_POWER_DOWN_RSP,
    .msg_data = ' '
  };

  LOG6("~~~~ msq_send_pd_rsp ~~~~\n");
#ifdef FEATURE_DEBUG_LOG_FILE
  LOG5("\n\n\n\n\n\n\n\n\n\n");
#else
  LOG2("\n\n\n\n\n\n\n\n\n\n");
#endif

  msq_send_msg_to_proc(snd_qid, (void *)(&tmpmsg), 1);
}

#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
void msq_send_custom_at_rsp(void)
{
  Proc_Req_Msg_S_Type tmpmsg;

  LOG6("~~~~ msq_send_custom_at_rsp ~~~~\n");

  tmpmsg.id = YY_PROCESS_MSG_MDMEmodem_AT_CMD_RSP;
  memset(tmpmsg.data.at_cmd_config.data, 0, MAX_PROC_MSG_AT_RSP_STR_LEN);
  memcpy(tmpmsg.data.at_cmd_config.data, proc_msg_at_config.rsp_str, MAX_PROC_MSG_AT_RSP_STR_LEN);
  
#ifdef FEATURE_ENABLE_PRINT_CUSTOMED_AT_CMD_RSP
  LOG("AT cmd rsp: %s\n", tmpmsg.data.at_cmd_config.data);
#endif

  msq_send_msg_to_proc(snd_qid, (void *)(&tmpmsg), sizeof(tmpmsg.data));
}
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

//Ret:
//  1: Success
// -1: msq_send_msg_to_proc() failed.
// -2: msgrcv() failed.
int msq_qry_pwr_inf(void)
{
	typedef struct{
		long id;
		char data;
	}Tmp_Msg_S_Type;
	
	Tmp_Msg_S_Type pi_req_msg = {
		.id = YY_PROCESS_MSG_BATTERY_REQ,
		.data = ' '
	};
	int ret;
	
	LOG6("~~~~ msq_qry_pwr_inf ~~~~\n");
	
	ret = msq_send_msg_to_proc(snd_qid, (void *)(&pi_req_msg), 1);
	if(ret != 0){
		LOG("msq_send_msg_to_proc() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}


#ifdef FEATURE_ENABLE_SEND_MSGTO_UPDATE
// add by jack li 20160105
extern Dev_Info sim5360_dev_info;
int msq_send_IMEI_to_update(void)
{
	typedef struct{
		long id;
		char data[15];
	}Tmp_Msg_S_Type;

	int ret;

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.id = YY_PROCESS_MSG_SEND_IMEI_TO_UPDATE;
	
	memset(&(send_msgto_update.data[0]), 0, IMEI_DATA_LEN);
	memcpy(&(send_msgto_update.data[0]), &(sim5360_dev_info.IMEI[0]), IMEI_DATA_LEN);

	/*for(temp = 0; temp < 15;temp++)
	{
		LOG("temp =0x%02x.\n", temp);
		LOG("sim5360_dev_info.IMEI[0] =0x%02x.\n", sim5360_dev_info.IMEI[temp]);
	}*/

	ret = msq_send_msg_to_proc(snd_up_qid, (void *)(&send_msgto_update), IMEI_DATA_LEN);
	if(ret != 0){
		LOG("msq_send_IMEI_to_update() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}
#endif


#ifdef FEATURE_ENABLE_FLU_LIMIT
// add by jack li 20160105

int msq_send_flu_to_yydaemon(long long fluvalue, int showflag)
{
	typedef struct{
		long id;
		char  databuf[20];
		int   fluflag;
	}Tmp_Msg_S_Type;

	int ret;
	int datalen;
	int templen;
	

	Tmp_Msg_S_Type send_msgto_update;

       memset(&(send_msgto_update.databuf[0]), 0, 20);
	send_msgto_update.id = YY_PROCESS_MSG_SEND_FLU_CURRENT;


	sprintf(send_msgto_update.databuf,"%lld",fluvalue);
	
	LOG("fluvalue =%lld.\n", fluvalue);
	LOG("send_msgto_update.databuf =%s.\n", send_msgto_update.databuf);

	datalen = strlen(send_msgto_update.databuf);

	//LOG("datalen =%d.\n", datalen);

	send_msgto_update.fluflag = showflag;
	LOG("send_msgto_update.fluflag = 0x%02x.\n", send_msgto_update.fluflag);

	templen =  sizeof(send_msgto_update);
	//LOG("templen =%d.\n", templen);
	

	//LOG(" msq_send_flu_to_yydaemon  send_msgto_update.uldata[0] =%d \n", send_msgto_update.uldata[0]);
	//LOG(" msq_send_flu_to_yydaemon  send_msgto_update.uldata[1] =%d \n", send_msgto_update.uldata[1]);

	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), sizeof(send_msgto_update));
	if(ret != 0){
		LOG("msq_send_flu_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}


#endif

#ifdef FEATURE_ENABLE_5360IMEI_TO_YYDAEMON

int msq_send_5360imei_to_yydaemon(void)
{
	typedef struct{
		long id;
		char data[15];
	}Tmp_Msg_S_Type;

	int ret;
	//int temp;

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.id = YY_PROCESS_MSG_SEND_IMEI_TO_YY_DAEMON;
	
	memset(&(send_msgto_update.data[0]), 0, IMEI_DATA_LEN);
	memcpy(&(send_msgto_update.data[0]), &(sim5360_dev_info.IMEI[0]), IMEI_DATA_LEN);

	/*for(temp = 0; temp < 15;temp++)
	{
		LOG("temp =0x%02x.\n", temp);
		LOG("sim5360_dev_info.IMEI[0] =0x%02x.\n", sim5360_dev_info.IMEI[temp]);
	}*/

	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), IMEI_DATA_LEN);
	if(ret != 0){
		LOG("msq_send_5360imei_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}

#endif

#ifdef FEATURE_ENABLE_APN_MESSAGE_TO_YYDAEMON

int msq_send_APN_message_to_yydaemon(void)
{
	typedef struct{
		long id;
		char data[APN_MESSAGE_LEN];
	}Tmp_Msg_S_Type;

	int ret;
	int temp;

	//int i;

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.id = YY_PROCESS_MSG_SEND_APN_MESSAGE_TO_YY_DAEMON;
	
	memset(&(send_msgto_update.data[0]), 0, APN_MESSAGE_LEN);
	memcpy(&(send_msgto_update.data[0]), &(apnMessageBuff[0]), APN_MESSAGE_LEN);

	
	/*LOG2("apn -> daemon: \n");
	i = 1;					
	for(; i<=200; i++){
		if(i%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(send_msgto_update.data, i-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(send_msgto_update.data, i-1) );
												
		if(i%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i-1)%20 != 0) // Avoid print redundant line break
	LOG3("\n");*/

	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), APN_MESSAGE_LEN);
	if(ret != 0){
		LOG("msq_send_APN_message_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}else{

		LOG("Debug: send apn message to yy_daemon is successfull.\n");
	}
	
	return 1;
}

#endif


//add by zhijie 20170330,Begin
#ifdef  FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG
 int msq_send_SoftSim_mode_message_to_yydaemon()
{
	typedef struct{
		long id;
		char  uldata;
	}Tmp_Msg_S_Type;

	int ret;
	

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.uldata  = 1;
	send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_SOFT_SIM_MODE;
	
	LOG("Debug:send_msgto_update.uldata =  %d .\n", send_msgto_update.uldata);


	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), 1);
	if(ret != 0){
		LOG("msq_send_SoftSim_mode_message_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}


#endif

 //add by zhijie 20170330,End



 #ifdef FEATURE_ENABLE_IDCHECK_C7_C8

 int msq_send_Lcdversion_to_yydaemon(char versionf)
{
	typedef struct{
		long id;
		char  uldata;
	}Tmp_Msg_S_Type;

	int ret;
	

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.uldata  = versionf;
	send_msgto_update.id = YY_PROCESS_MSG_SEND_VERSION_TO_YY_DAEMON;
	
	LOG("Debug:send_msgto_update.uldata =  %d .\n", send_msgto_update.uldata);


	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), 1);
	if(ret != 0){
		LOG("msq_send_pdp_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}




 #endif

  #ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

 int msqSendToTrafficAndDateYydaemon(char versionf,long long userRate)
{
	typedef struct{
		long id;
		char  databuf[20];
	}Tmp_Msg_S_Type;

	int ret;

	Tmp_Msg_S_Type send_msgto_update;

	memset(&(send_msgto_update.databuf[0]), 0, sizeof(send_msgto_update.databuf));

	if (!userRate){

		userRate = 1ULL;
		LOG("vvvvvvvvvvvvvvvvvvvvvvvv.\n");
	}

	sprintf(send_msgto_update.databuf,"%lld",userRate);
	LOG("send_msgto_update.databuf =%s.\n", send_msgto_update.databuf);
	LOG("userRate = %lld.\n", userRate);
	
	
	if (versionf == NOTE_TRAFFIC_OUTOF){
		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_OUT_OF_TRAFFIC;
	}else if (versionf == NOTE_DATE_OUTOF){
		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_OUT_OF_DATE;
	}else if (versionf == NOTE_SHOW_DATA_RATE){
		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_SHOW_RATE;
	}else if (versionf == NOTE_SHOW_BOOK){
		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_BOOKORDER;
	}else if (versionf == NOTE_SHOW_MIN){
		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_USERMIN;
	}
	else if (versionf == NOTE_JUMP_WINDOWN){
		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_WINDOW;
	}else{

		send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_APPLE;
	}
	
	LOG("Debug:send_msgto_update.id =  %d .\n", send_msgto_update.id);
	
	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), sizeof(send_msgto_update));
	if(ret != 0){
		LOG("msq_send_pdp_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}

 #endif

 #ifdef FEATURE_ENABLE_STSRT_TEST_MODE
int msq_send_TestMode_to_yydaemon(void)
{
	typedef struct{
		long id;
		long  uldata;
	}Tmp_Msg_S_Type;

	int ret;
	

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.uldata  = 0x00;
	send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_ISTEST_MODE;

	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), 4);
	if(ret != 0){
		LOG("msq_send_TestMode_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}
 
#endif

 #ifdef FEATURE_ENABLE_STSRT_TEST_MODE
int msq_send_TestModeisPullWindown_to_yydaemon(char ismode)
{
	typedef struct{
		long id;
		char  uldata;
	}Tmp_Msg_S_Type;

	int ret;
	

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.uldata  = ismode;
	send_msgto_update.id = YY_PROCESS_MSG_SEND_YYDAEMON_ISLIMIT_PULL_WINDOWN;
	
	LOG("Debug:send_msgto_update.uldata =  %d .\n", send_msgto_update.uldata);


	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), 1);
	if(ret != 0){
		LOG("msq_send_pdp_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}
 
#endif

int msq_send_pdp_to_yydaemon(void)
{
	typedef struct{
		long id;
		long  uldata;
	}Tmp_Msg_S_Type;

	int ret;
	

	Tmp_Msg_S_Type send_msgto_update;

	
	send_msgto_update.uldata  = 0x00;
	send_msgto_update.id = YY_PROCESS_MSG_SEND_PDP_NOTACTIVE;

	ret = msq_send_msg_to_proc(ind_qid, (void *)(&send_msgto_update), 4);
	if(ret != 0){
		LOG("msq_send_pdp_to_yydaemon() failed. ret=%d.\n", ret);
		return -1;
	}
	
	return 1;
}


void msq_send_imsi_ind_internal(void)
{
	Proc_Req_Msg_S_Type imsi_ind_msg;
	
	imsi_ind_msg.id = YY_PROCESS_MSG_MDM9215_IMSI_IND;
	memset(&(imsi_ind_msg.data.imsi_data), 0, sizeof(imsi_ind_msg.data.imsi_data));
	memcpy(imsi_ind_msg.data.imsi_data.data, rsmp_dev_info.IMSI, sizeof(rsmp_dev_info.IMSI));
	
	msq_send_msg_to_proc(ind_qid, (void *)(&imsi_ind_msg), sizeof(imsi_ind_msg.data.imsi_data));
	return;
}

//Param:
//	opt:
//		1: Call notify_Sender(0x03) to acquire latest IMSI.
//		0: Not call notify_Sender(0x03) to acquire IMSI.
void msq_send_imsi_ind(char opt)
{
	LOG6("~~~~ msq_send_imsi_ind (%02x) ~~~~\n", opt);
	
	if(0x00 == opt){
		Proc_Req_Msg_S_Type imsi_ind_msg;
	
		imsi_ind_msg.id = YY_PROCESS_MSG_MDM9215_IMSI_IND;
		memset(&(imsi_ind_msg.data.imsi_data), 0, sizeof(imsi_ind_msg.data.imsi_data));
		memcpy(imsi_ind_msg.data.imsi_data.data, rsmp_dev_info.IMSI, sizeof(rsmp_dev_info.IMSI));
		
		msq_send_msg_to_proc(ind_qid, (void *)(&imsi_ind_msg), sizeof(imsi_ind_msg.data.imsi_data));
	}else if(0x01 == opt){
		isUploadImsiToOtherProc = true;
		notify_Sender(QMISENDER_ACQ_IMSI);
	}else{
		LOG("ERROR: Wrong opt (%02x) found!\n", opt);
	}
	
	return;
}

void msq_send_mdm9215_usim_rdy_ind(int opt)
{
	Proc_Req_Msg_S_Type mdm9215_usim_ind_msg;
	
	LOG6("~~~~ msq_send_mdm9215_usim_rdy_ind (%02x) ~~~~\n", opt);
	
	mdm9215_usim_ind_msg.id = YY_PROCESS_MSG_MDM9215_USIM_RDY_IND;
	mdm9215_usim_ind_msg.data.usim_status = opt;
	msq_send_msg_to_proc(ind_qid, (void *)(&mdm9215_usim_ind_msg), sizeof(mdm9215_usim_ind_msg.data.usim_status));
	
	return;
}

void msq_send_mdm9215_reg_status_ind(int opt)
{
  Proc_Req_Msg_S_Type mdm9215_reg_ind_msg;

  LOG6("~~~~ msq_send_mdm9215_reg_status_ind (%02x) ~~~~\n", opt);

  mdm9215_reg_ind_msg.id = YY_PROCESS_MSG_MDM9215_REG_IND;
  mdm9215_reg_ind_msg.data.reg_status = opt;
  msq_send_msg_to_proc(ind_qid, (void *)(&mdm9215_reg_ind_msg), sizeof(mdm9215_reg_ind_msg.data.reg_status));
}

void msq_send_mdm9215_nw_sys_mode_ind(int val)
{
	Proc_Req_Msg_S_Type mdm9215_nw_sys_mode_ind_msg;
	
	LOG6("~~~~ msq_send_mdm9215_nw_sys_mode_ind (%02x) ~~~~\n", val);
	
	mdm9215_nw_sys_mode_ind_msg.id = YY_PROCESS_MSG_MDM9215_NW_SYS_MODE_IND;
	mdm9215_nw_sys_mode_ind_msg.data.nw_sys_mode = val;
	msq_send_msg_to_proc(ind_qid, 
												(void *)(&mdm9215_nw_sys_mode_ind_msg), 
												sizeof(mdm9215_nw_sys_mode_ind_msg.data.nw_sys_mode)
											  );
	
	return;
}

//Param:
//	0: Notify yy_daemon that sys rst begins. The pwr dwn req should be not sent to qlinkdatanode after this.
//	1: Notify yy_daemon that sys rst completes.
void msq_send_sys_rst_req(int msg)
{
	Proc_Req_Msg_S_Type sys_rst_msg;

	sys_rst_msg.id = YY_PROCESS_MSG_SYS_RST;
	sys_rst_msg.data.sys_rst_data = msg;
	msq_send_msg_to_proc(ind_qid, 
												(void *)(&sys_rst_msg), 
												sizeof(sys_rst_msg.data.sys_rst_data)
											  );
	
	return;
}

//Description:
//		When the device was fallen onto groud or something solid, the USIM card of mdmEmodem might be
//	loosen in card slot. This might cause the registration fell unrecoverably. So restart the whole device 
//	is necessary. (The affected mdmEmodem network link may cause reg failure or reg timeout in mdm9215.)
void msq_send_dev_rst_req(int msg)
{
	Proc_Req_Msg_S_Type dev_rst_msg;

	dev_rst_msg.id = YY_PROCESS_MSG_DEV_RST;
	dev_rst_msg.data.dev_rst_data = msg; //Not cared
	msq_send_msg_to_proc(ind_qid, 
												(void *)(&dev_rst_msg), 
												sizeof(dev_rst_msg.data.sys_rst_data)
											  );
	
}



#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
static int conv_ctrl_word_to_proc_msg(int ctrl_word){
  if(ctrl_word != 0 && ctrl_word != 1){
    return 5;
  }

  if(0 == ctrl_word)
    return 6;
  else
    return 5;
}

//Description:
//The function generates the proc_msg_at_config.at_cmd_id according to proc_msg_at_config.cmd_str.
static void msq_gen_at_cmd_id(void){
  char *pos = NULL;

  if(proc_msg_at_config.cmd_str[0] == '\0'){
    LOG("ERROR: No AT cmd string found!\n");
    return;
  }

  pos = strstr(proc_msg_at_config.cmd_str, "AT+SIMEI=");
  if(pos != NULL){
    proc_msg_at_config.at_cmd_id = AT_CMD_ID_SIMEI;
    return;
  }

  proc_msg_at_config.at_cmd_id = AT_CMD_ID_MAX;
}
#endif //FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD

static void MsgLoop(int qid)
{
  int ret;
  Proc_Req_Msg_S_Type msg;

  if(qid < 0){
    LOG("ERROR: Wrong qid found. qid=%d.\n", qid);
    return;
  }

  do{
    ret = msgrcv(qid, 
                (void *)(&msg), 
                MAX_SIZE_OF_REQ_MSG_DATA, 
                -YY_PROCESS_MSG_MAX, 
                IPC_NOWAIT | MSG_NOERROR
                );
    if(ret == -1 && (errno == EINTR || errno == ENOMSG)){
      usleep(1000);
      continue;
    }else if(ret == -1){
      LOG("ERROR: msgrcv() error. errno=%d.\n", errno);
      continue;
    }else{
      switch(msg.id)
      {
      case YY_PROCESS_MSG_BATTERY_RSP:
      {
        print_cur_gm_time("   Battery Info   ");
        LOG("NOTICE: Battery power message is recved. Battery percentage: %d.\n", msg.data.pwr_inf.per);
        rsmp_dev_info.power_lvl = (unsigned char)msg.data.pwr_inf.per;
        pthread_mutex_lock(&pwr_inf_qry_mtx);
        pwr_inf_acqed = 1;
        pthread_cond_broadcast(&pwr_inf_qry_cnd);
        pthread_mutex_unlock(&pwr_inf_qry_mtx);
        break;
      } // case YY_PROCESS_MSG_BATTERY_RSP
      case YY_PROCESS_MSG_POWER_DOWN_REQ:
      {
        isFTMPowerOffInd = true; // Including action of factory reset.

        if(msg.data.flag == 0x01)
        {
          isRstReqAcq = true;
          print_cur_gm_time("   Factory Reset   ");

#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810

	g_qlinkdatanode_run_status_01 = 0xff;
	g_qlinkdatanode_run_status_02 = 0xff;

	LOG("DEBUG: g_qlinkdatanode_run_status_01 =%d.\n", g_qlinkdatanode_run_status_01);
	LOG("DEBUG: g_qlinkdatanode_run_status_01 =%d.\n", g_qlinkdatanode_run_status_02);

#endif
		
          notify_Sender(0x05); // Not wait for task completed
          //After clearing local USIM data, app go to the process of power down.
          }else{ // msg.data.flag == 0x00
            print_cur_gm_time("   Power down   ");
	    
          }

#ifdef FEATURE_ENABLE_FLU_LIMIT

	 notify_Sender_status(0x06);

       if (WriteFluValueToFile(flu_maxValue_current,0) == (-1))
	{
		LOG("ERROR: MsgLoop.c: ~~ffffff~~ WriteFluValueToFile  flu_maxValue_current \n");
	}

	  unsigned long pre_net0_value;
	  pre_net0_value = 0;
	// fix qlinkdatanode restart issuesoc
	if (WriteFluValueToFile(pre_net0_value,1) == (-1))
	{
		LOG("ERROR: MsgLoop.c: ~~ffffff~~ WriteFluValueToFile pre_net0_value \n");
	}
#endif
   

          clr_all_timers();
          clr_timeout_evt(NULL); // Clear all timeout evts.

          add_timer_power_down_notification_timeout();

          ACQ_CHANNEL_LOCK_WR;
					LOG("ChannelNum = %d.\n", ChannelNum);
					if(ChannelNum == -1 || ChannelNum == 4){
						LOG("mdmEmodem is on the process of recovering. Wait to shut down mdmEmodem...\n");
						//Note: It can't set isNotNeedSusp true if cmd "AT+CIPOPEN" succeed and ChannelNum = -1.
						
						if(isTermRcvrNow){
							LOG("Cancel the plan of terminating the recovering process.\n");
							isTermRcvrNow = false;
						}
						
						isWaitToPwrDwn = true;
						//No matter if isNetworkClosedUnexpectedly or isRegOffForPoorSig is set, it is fine to execute their handling process.
						
						RLS_CHANNEL_LOCK;
					}
					else if(ChannelNum == 0 || ChannelNum == 3){
						if(ChannelNum == 3){
							isMifiConnected = false;
							ChannelNum = 0;
						}
						
						if(isWaitToSusp){
							LOG("Cancel the suspension plan of mdmEmodem.\n");
							isWaitToSusp = false;
						}
						
						//ChannelNum == 0
						if(mdmEmodem_check_busy()){
							LOG("mdmEmodem is busy. Wait to shut down mdmEmodem...\n");
							isWaitToPwrDwn = true;
						}else{
							LOG("Send pd msg to server.\n");
							ACQ_ACQ_DATA_STATE_MTX;
							//Note: It needs to acquire acq_data_state_mtx for at_send_pd_msg_to_srv() inside.
							msq_send_pd_msg_to_srv();
							RLS_ACQ_DATA_STATE_MTX;
						}
						
						RLS_CHANNEL_LOCK;
					}
					else if(ChannelNum == 1){
						if(isWaitToRcvr){
							LOG("Cancel the recovering plan of mdmEmodem.\n");
							isWaitToRcvr = false;
						}
						
						RLS_CHANNEL_LOCK; // Add by rjf at 20151102 (ChannelLock will be requested in RecvCallback().)
						
						isNeedToSendOutReq0x65 = true;
						pthread_mutex_lock(&evtloop_mtx);
						push_ChannelNum(4); // Fixed by rjf at 20151016
						notify_EvtLoop(0x02);
						pthread_mutex_unlock(&evtloop_mtx);
					}
					else if(ChannelNum == 2){
						if(isTermSuspNow == true){
							LOG("Cancel the plan of terminating the suspension process.\n");
							isTermSuspNow = false;
						}
						if(isWaitToRcvr){
							LOG("Cancel the recovering plan of mdmEmodem.\n");
							isWaitToRcvr = false;
						}
						
						isWaitToPwrDwn = true;
						
						RLS_CHANNEL_LOCK;
					}
					
					break;
      } // case YY_PROCESS_MSG_POWER_DOWN_REQ
#ifdef FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
      case YY_PROCESS_MSG_MDMEmodem_WAKEUP_REQ:
      {
        int proc_msg;

        LOG("Process msg (id: %ld) recved.\n", msg.id);

        isMdmEmodemControledByYydaemon = true;

        proc_msg = conv_ctrl_word_to_proc_msg(msg.data.ctrl_word);

        pthread_mutex_lock(&evtloop_mtx);
        notify_EvtLoop(proc_msg);
        pthread_mutex_unlock(&evtloop_mtx);
        break;
      } // case YY_PROCESS_MSG_WAKE_UP_MDMEmodem
      case YY_PROCESS_MSG_MDMEmodem_AT_CMD_REQ:
      {
        LOG("YY_PROCESS_MSG_MDMEmodem_AT_CMD_REQ got\n");

        if(msg.data.at_cmd_config.data[0] == '\0'){
          LOG("ERROR: AT cmd string not found in msg! Dump this req...\n");
          break;
        }

        memset(proc_msg_at_config.cmd_str, 0, MAX_PROC_MSG_AT_CMD_STR_LEN);
        strcpy(proc_msg_at_config.cmd_str, msg.data.at_cmd_config.data);
        msq_gen_at_cmd_id();
        pthread_mutex_lock(&evtloop_mtx);
        notify_EvtLoop(0x04);
        pthread_mutex_unlock(&evtloop_mtx);

        break;
      } // case YY_PROCESS_MSG_MDMEmodem_AT_CMD_REQ
#endif // FEATURE_ENABLE_MDMEmodem_CUSTOMED_AT_CMD
      case YY_PROCESS_MSG_NTP_RPT:
      {
	 isgetNptByYydaemon = true;
	 
#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

	 LOG("  ####################################################  \n");
 	 LOG("  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$  \n");
	 
	 LOG("   Get ntp time from yy_daemomn --> \"%s\"  \n", msg.data.ntp_time.ntp_time_yy);
	 MdmEmodemSetCCLK(msg.data.ntp_time.ntp_time_yy);

#endif
	 
	 LOG("Msq_qlinkdatanode.c :get NPT msg from yy_daemonisgetNptByYydaemon = %d.\n", isgetNptByYydaemon);
        print_cur_gm_time("   NTP rpt  ");
        break;
      } // case YY_PROCESS_MSG_NTP_RPT
      default:
      {
        LOG("ERROR: Wrong msg.id (%ld) found! Continue recving...\n", msg.id);
        break;
      }
      } // switch end
    }
    usleep(1000);
  }while(1);
}

static void *initMsgLoop(void *param)
{
  int ret1, ret2;

  ret1 = msg_queue_init(MSGQ_RCV);
  ret2 = msg_queue_init(MSGQ_SND);

#ifdef FEATURE_ENABLE_SEND_MSGTO_UPDATE
  int ret3;

  ret3 = msg_queue_init(MSGQ_SND_UPDATE);
  
#endif

  if(ret1 < 0 || ret2 < 0){
    *((int *)param) = -1;
  }else{
    *((int *)param) = 1;
    rcv_qid = ret1;
    snd_qid = ret2;
    ind_qid = ret2; 
#ifdef FEATURE_ENABLE_SEND_MSGTO_UPDATE
    snd_up_qid = ret3; 
#endif
  }

  pthread_mutex_init(&pwr_inf_qry_mtx, NULL);
  pthread_cond_init(&pwr_inf_qry_cnd, NULL);

  pthread_mutex_lock(&msgloop_mutex);
  msgloopStarted = 1;
  pthread_cond_broadcast(&msgloop_cond);
  pthread_mutex_unlock(&msgloop_mutex);


  if (*((int *)param) < 0) {
    LOG("msg_queue_init() failed. ret1=%d, ret2=%d.\n",
    ret1, ret2);
    return NULL;
  }

  MsgLoop(rcv_qid);

  LOG("ERROR: MsgLoop ended unexpectedly!\n");
  while(1){
    sleep(0x00ffffff);
  }
  return NULL;
}

int startMsgLoop(void)
{
  int ret;
  pthread_attr_t attr;
  int result;

  msgloopStarted = 0;
  pthread_mutex_lock(&msgloop_mutex);

  pthread_attr_init (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
#ifdef FEATURE_ENABLE_SYSTEM_RESTORATION
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

__MSGLOOP_PTHREAD_CREATE_AGAIN__:
  ret = pthread_create(&msgloop_tid, &attr, initMsgLoop, (void *)(&result));
  if(ret != 0){
    if(EAGAIN == errno){
      sleep(1);
      goto __MSGLOOP_PTHREAD_CREATE_AGAIN__;
    }

    LOG("ERROR: pthread_create() failed. errno=%d.\n", errno);
    return 0;
  }

  thread_id_array[THD_IDX_MSGLOOP] = (unsigned long)msgloop_tid;

  while (msgloopStarted == 0) {
    pthread_cond_wait(&msgloop_cond, &msgloop_mutex);
  }

  pthread_mutex_unlock(&msgloop_mutex);

  if (result < 0) {
    LOG("ERROR: initMsgLoop() error.\n");
    return 0;
  }

  return 1;
}
