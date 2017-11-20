#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <pthread.h> // pthread_cancel
#include <string.h>

#include "common_qlinkdatanode.h"

#define THD_ERR LOG("Rst Thd Ends Here!!!\n");

extern int mdmEmodem_check_if_running(void);
extern int mdmEmodem_check_if_standby_switch_available(void);
extern int mdmEmodem_set_awake(void);
extern void mdmEmodem_shutdown(void);
extern void msq_send_sys_rst_req(int msg);
extern void Qmi_receive_release(void);

extern bool isFTMPowerOffInd;
extern unsigned long thread_id_array[THD_IDX_MAX];
extern Chk_Net_State chk_net_state;
extern bool isUimPwrDwnMssage;

#ifdef FEATURE_ENABLE_FLU_LIMIT
extern long long flu_net0Value;
extern int WriteFluValueToFile(long long ttvalue,unsigned char ucfile_mode);
#endif



int rst_com_pipe[2];

//Parameter:
//	msg:
//		0x00: Virtual USIM has been powered down.
//		0x01: Virtual USIM has been powered up.

void notify_RstMonitor(char msg)
{
	int wr_fd = rst_com_pipe[1], written_bytes = -1;

	LOG6("~~~~ notify_RstMonitor (%02x) ~~~~~\n", msg);
	
	do{
		written_bytes = write(wr_fd, (void *)(&msg), 1);
	}while(written_bytes == 0 || (written_bytes < 0 && errno == EINTR));
	if(written_bytes < 0){
		LOG("ERROR: write() failed. errno=%d.\n", errno);
	}

	return;
}

void startRstMonitor(void)
{
  int rst_com_pipe_fd[2];
  struct timeval tv, *ptv;
  int ret = -1;
  
  if(pipe(rst_com_pipe_fd) < 0){
    LOG("ERROR: pipe(rst_com_pipe_fd) error. errno=%d.\n", errno);
    THD_ERR;
    return;
  }
  
  rst_com_pipe[0] = rst_com_pipe_fd[0];
  rst_com_pipe[1] = rst_com_pipe_fd[1];
  fcntl(rst_com_pipe[0], F_SETFL, O_NONBLOCK);

  while(1)
  {
    int select_result = -1, rd_fd = rst_com_pipe[0], maxfdp1 = rd_fd+1;
    fd_set rfd;

    memset(&tv, 0, sizeof(tv));
    ptv = NULL;

__RST_MONITOR_SELECT_AGAIN__:
    FD_ZERO(&rfd);
    FD_SET(rd_fd, &rfd);
    
    select_result = select(maxfdp1, &rfd, NULL, NULL, ptv);
    
    if(select_result  < 0){
      LOG("ERROR: select() failed. errno=%d.\n", errno);
      THD_ERR;
      return;
    }else if(select_result > 0)
    {
      int read_bytes = -1;
      char recv_buf = 0xff;

      do{
        read_bytes = read(rd_fd, &recv_buf, 1);
      }while(read_bytes == 0 || (read_bytes < 0 && (errno == EAGAIN || errno == EINTR)));
      
      if(read_bytes < 0){
        LOG("ERROR: read() failed. errno=%d.\n", errno);
        goto __RST_MONITOR_SELECT_AGAIN__;
      }

      if(isFTMPowerOffInd){
        LOG("NOTICE: isPwrDwnReqAcq = 1. Cancel the system restoration.\n");
        break;
      }

      LOG("NOTICE: Restoration Monitor received request! recv_buf = 0x%02x.\n", recv_buf);
      switch(recv_buf){
      case 0x00:{
        //Virtual USIM has been powered down
        tv.tv_sec = 3*60;  // by jack li 20160224
        tv.tv_usec = 0;
        ptv = &tv;
        goto __RST_MONITOR_SELECT_AGAIN__;
      }
      case 0x01:{
        //Virtual USIM has been powered up
        ptv = NULL;
	 isUimPwrDwnMssage = false;
	 LOG("~~~~ start monitor 04 isUimPwrDwnMssage :=%d\n",isUimPwrDwnMssage);
        goto __RST_MONITOR_SELECT_AGAIN__;
      }
	
      default:
        LOG("ERROR: Wrong request message received. recv_buf = 0x%02x.\n", recv_buf);
        break;
      }
    }else
    {//select() returns 0

      /*int i = 0;

      if(CHK_NET_STATE_MAX != chk_net_state)
      {
        //Not restore system when environment signal is weak.
        LOG("NOTICE: The environment signal is weak, so cancel the system restoration.\n");
        tv.tv_sec = 3*60;
        tv.tv_usec = 0;
        ptv = &tv;
        goto __RST_MONITOR_SELECT_AGAIN__;
      }*/

      LOG("NOTICE: System Restoration begins!\n");

#ifdef FEATURE_ENABLE_FLU_LIMIT
	// fix qlinkdatanode restart issue
	if (WriteFluValueToFile(flu_net0Value,1) == (-1))
	{
		LOG("ERROR: startRstMonitor.c: ~~ffffff~~ WriteFluValueToFile pre_net0_value \n");
	}
#endif

	LOG("~~~~ start monitor 03 isUimPwrDwnMssage :=%d\n",isUimPwrDwnMssage);

	if (false == isUimPwrDwnMssage)
	{
		LOG("~~~~ start monitor 04  reboot \n");
		//system("reboot");
		yy_popen_call("reboot -f");
		while(1)
		{
			sleep(1);
		}
	}
   

     //Terminate the threads except main thread (i.e. Sys rst thread)
     //Because the action of shutting down mdmEmodem takes a relatively long time, terminating useless threads for no interference
     //is necessary.
     //Not move this block of codes casually. mdmEmodem_check_if_standby_switch_available() is designed according that EvtLoop 
      //has been terminated before calling itself.
      LOG("Terminate the threads...\n");
      //Stop powerup at first. It may cause an acceptable error below.
     
     
      Qmi_receive_release();
      ret = pthread_cancel((pthread_t)thread_id_array[THD_IDX_RECVER]);
      if(ret == -1)
      {
         LOG("cancel 3thread erro %d ...\n", errno);
      }	
	  
      ret = pthread_cancel((pthread_t)thread_id_array[THD_IDX_SENDER]);
      if(ret == -1)
      {
         LOG("cancel 1thread erro %d ...\n", errno);
      }

      ret = pthread_cancel((pthread_t)thread_id_array[THD_IDX_EVTLOOP]);
      if(ret == -1)
      {
         LOG("cancel 2thread erro %d ...\n", errno);
      }
      //Stop standby switch at second. mdmEmodem_set_awake() is robust enough to handle wrong gpio power level.
#if 0
      for( i=THD_IDX_EVTLOOP; i<THD_IDX_MAX; i++){
        pthread_cancel((pthread_t)thread_id_array[i]);
      }
#endif

      //Notify yy_daemon that system restoration begins
      msq_send_sys_rst_req(0);

      //Set mdmEmodem awake if needed
      if(!mdmEmodem_check_if_running()){
        LOG("MdmEmodem is on standby mode. Check the standby switch ctrl...\n");
        if(mdmEmodem_check_if_standby_switch_available() == -1){
          LOG("ERROR: mdmEmodem_check_if_standby_switch_available() returns -1! Force mdmEmodem to be waken up.\n");
        }

        LOG("Waking up mdmEmodem...\n");
        mdmEmodem_set_awake();
        LOG("MdmEmodem has been waken up.\n");
      }

      //Shut down mdmEmodem
      LOG("Shutting down mdmEmodem...\n");
      mdmEmodem_shutdown();
      LOG("MdmEmodem has been shut down.\n");

      //Notify yy_daemon that system restoration completes. And request to kill itself.
      LOG("Sending notification to yy_daemon...\n");
      msq_send_sys_rst_req(1);

      break;
    }
  }

  return;
}
