#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include <pthread.h>
#include <errno.h>

#include "sock_listener_qlinkdatanode.h"
#include "common_qlinkdatanode.h"
#include "socket_qlinkdatanode.h"


/******************************External Functions*******************************/
extern void clr_idle_timer(void);
extern void clr_pdp_check_act_routine_timer(void);


/******************************External Variables*******************************/
extern bool isFTMPowerOffInd;
extern Suspend_Mdm_State suspend_state;
extern int SockListener_2_Listener_pipe[2];
extern unsigned long thread_id_array[THD_IDX_MAX];

#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

extern bool isPdpActive;

#endif


/******************************Local Variables*******************************/
static int listen_fd = -1;
static int socklistenerloopStarted = 0;
static pthread_t socklistenerloop_tid;
static pthread_mutex_t socklistenerloop_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t socklistenerloop_cond = PTHREAD_COND_INITIALIZER;

static bool isDialingFailed = false;


/******************************Global Variables*******************************/
//Description:
//Once PDP act msg is recved by sock listener, this var will be set true. And when net_act_evt_hdlr()
//completes, this var will be set false.
//This var is used for avoiding the collision between pdp act msg and pdp check by idle timer.
bool isSockListenerRecvPdpActMsg = false;

//Param:
//sig = 0x01: PDP activated.
//sig = 0x00: PDP not activated.
static void notify_ListenerLoop(const char sig)
{
	int n = -1;
	
	LOG6("~~~~ notify_ListenerLoop start (%d) ~~~~\n", sig);
	
	if(sig != 0x01 && sig != 0x00){
		LOG("Wrong sig found. sig=%d.\n", sig);
		goto __EXIT_OF_NOTIFY_LISTENERLOOP__;
	}
	
	do{
		if(n == 0)
			LOG("Write() failed. n=%d.\n", n);
		n = write(SockListener_2_Listener_pipe[1], &sig, 1);
	}while ((n<0 && errno==EINTR) || n==0);
	
	if(n<0){
		LOG("ERROR: Write() erro. n=%d, errno=%d.\n", n, errno);
	}

	__EXIT_OF_NOTIFY_LISTENERLOOP__:
	LOG6("~~~~ notify_ListenerLoop end (%d)~~~~\n", sig);
	return;
}

//Note:
//	No precaution for input param.
static int parse_dialup_msg(char *recv_buf)
{
  int ret = -1;

  if(*recv_buf == 0x00){
    ret = 0;
  }else{
    ret = 1;
  }

  return ret;
}

static void SockListenerLoop(void)
{
  struct sockaddr_un client_addr;
  int client_fd = -1, accept_len = -1;
  fd_set rfd;
  int ret = -1;
  
  accept_len = sizeof(client_addr);

	int read_bytes = -1;
	char recv_buf[2] = {0};

	if((client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&accept_len)) < 0){
	  LOG("accept() failed. errno=%d. Continue.\n", errno);
	  
	}

	accept_len -= offsetof(struct sockaddr_un, sun_path);
	client_addr.sun_path[accept_len] = 0x00;
	memset(recv_buf, 0, sizeof(recv_buf));

	FD_ZERO(&rfd);
  FD_SET(client_fd, &rfd);
while(1){
	ret = select(client_fd+1, &rfd, NULL, NULL, NULL);

	if(ret > 0)
	{
    do{
      read_bytes = read(client_fd, recv_buf, 1);
    }while(read_bytes == 0 || (read_bytes == -1 && errno == EINTR));
	
    if(read_bytes < 0){
      LOG("ERROR: read() error. errno=%s(%d). Continue.\n", strerror(errno), errno);
      continue;
    }else{
      int ret = -1;

      print_cur_gm_time("   Dialing Report   ");
	  
      if(isFTMPowerOffInd)
      {
        LOG("NOTICE: isFTMPowerOffInd = 1, so ignore PDP activation msg.\n");
        break; // Terminate the while loop
      }

      ret = parse_dialup_msg(recv_buf);
      if(ret == 1)
      {
        LOG("NOTICE: PDP has been activated.\n");
	 msq_send_qlinkdatanode_run_stat_ind(30); // 2016/12/07 by jack li
        isDialingFailed = false;
        isSockListenerRecvPdpActMsg = true;
	isPdpActive = true;

        clr_idle_timer(); // For uploading status info
        clr_pdp_check_act_routine_timer();
        notify_ListenerLoop(0x01);
      }else{
        LOG("NOTICE: PDP activation failed.\n");
	 msq_send_qlinkdatanode_run_stat_ind(29); // 2016/12/07 by jack li
        if(isDialingFailed){
          continue; // Redundant msg of activation failure
        }

        isDialingFailed = true;

        clr_idle_timer(); // For uploading status info
        clr_pdp_check_act_routine_timer();
        notify_ListenerLoop(0x00);
    }

    } // read_bytes > 0
  }
}

  return;
}

static void *initSockListener(void *param)
{
	struct sockaddr_un srv_addr;
	int bind_len = -1, *result = NULL;
	char *servname = NULL;
	
	if(param == NULL){
		LOG("ERROR: Wrong param input. param=null.\n");
		*result = -1;
		goto __NOTIFY_MAIN_THREAD__;
	}
	if( ((Input_Param_t *)param)->serv_name[0] == 0x00){
		LOG("ERROR: Wrong param input. serv_name is improper.\n");
		*result = -1;
		goto __NOTIFY_MAIN_THREAD__;
	}
	servname = ((Input_Param_t *)param)->serv_name;
	result = &((Input_Param_t *)param)->result;
	
	memset(&srv_addr, 0, sizeof(struct sockaddr_un));
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, servname);
	
	unlink(servname);
	
	if((listen_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		LOG("socket() failed. errno=%d.\n", errno);
		*result = -1;
		goto __NOTIFY_MAIN_THREAD__;
	}
	
	bind_len = offsetof(struct sockaddr_un, sun_path) + strlen(srv_addr.sun_path);
	if(bind(listen_fd, (struct sockaddr*)&srv_addr, bind_len) < 0)
	{
		LOG("bind() failed. errno=%d.\n", errno);
		*result = -1;
		goto __NOTIFY_MAIN_THREAD__;
	}
	
	if(listen(listen_fd, 1) < 0)
	{
		LOG("listen() failed. errno=%d.\n", errno);
		*result = -1;
		goto __NOTIFY_MAIN_THREAD__;
	}
	
	__NOTIFY_MAIN_THREAD__:
	pthread_mutex_lock(&socklistenerloop_mutex);
	socklistenerloopStarted = 1;
	pthread_cond_broadcast(&socklistenerloop_cond);
	pthread_mutex_unlock(&socklistenerloop_mutex);
	
	if(*result < 0){
		return NULL;
	}
	
	SockListenerLoop();
	
	LOG("ERROR: SockListenerLoop() ended unexpectedly!\n");
	while(1){
		sleep(0x00ffffff);
	}
	return NULL;
}

int startSockListenerLoop(void)
{
	int ret;
	pthread_attr_t attr;
	Input_Param_t param;

	socklistenerloopStarted = 0;
	param.result = 0;
	memcpy(param.serv_name, UNIX_SERV_NAME, sizeof(UNIX_SERV_NAME)); // Include '\0'
	
	pthread_mutex_lock(&socklistenerloop_mutex);

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	#ifdef FEATURE_ENABLE_SYSTEM_RESTORATION
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	#endif
	
	__SOCK_LISTENER_PTHREAD_CREATE_AGAIN__:
	ret = pthread_create(&socklistenerloop_tid, &attr, initSockListener, (void *)(&param));
	if(ret != 0){
		if(EAGAIN == errno){
			sleep(1);
			goto __SOCK_LISTENER_PTHREAD_CREATE_AGAIN__;
		}
		
		LOG("ERROR: pthread_create() failed. errno=%d.\n", errno);
		return 0;
	}
	
	#ifdef FEATURE_ENABLE_PRINT_TID
		LOG("NOTICE: slloop_tid = %lu.\n", (unsigned long)socklistenerloop_tid);
	#endif
	thread_id_array[THD_IDX_SOCKLISTENERLOOP] = (unsigned long)socklistenerloop_tid;
	
	while(socklistenerloopStarted == 0){
		pthread_cond_wait(&socklistenerloop_cond, &socklistenerloop_mutex);
	}

	pthread_mutex_unlock(&socklistenerloop_mutex);
	
	if(param.result < 0){
		LOG("ERROR: initSockListener() error.\n");
		return 0;
	}
	
	return 1;
}

