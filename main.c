#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include "feature_macro_qlinkdatanode.h"
#include "common_qlinkdatanode.h"

#ifdef FEATURE_ENABLE_APP_DAEMON
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#endif

#ifndef FEATURE_ENABLE_MAIN_C_LOG
	#define LOG print_to_null
#endif

#define LOCKFILE "/usr/bin/qlinkdatanode_daemon_lock_file"
//Description:
//	Failure rsp:
//		"ls: XXXXX: No such file or directory"
#define SYS_CMD_LS_FAILURE_RSP_FIXED_LEN (31)


/******************************External Functions*******************************/
extern int startEvtLoopThread(void);
extern int startSerialComm(int argc, char **argv);
extern int startQueue(void);
extern int startQMIReceiverThread(void);
extern int startQMIParserThread(void);
extern int startQMISender(void);
extern int startQMIUimSender(void);
extern int startQMINasSender(void);
extern int startQMIDmsSender(void);
extern int startUrcMonitorThread(void);
extern int startMsgLoop(void);
extern int startSockListenerLoop(void);
extern void startRstMonitor(void);
extern int write_file(const char *path, unsigned int offset, char *data);

#ifdef FEATURE_ENABLE_STATUS_REPORT_10_MIN

extern int startStatusReportThread();

#endif





/******************************External Variables*******************************/
extern int evtloop_pipe[2];
extern int chan_num_pipe[2];
extern int flow_state_pipe[2];
extern int thread_idx_pipe[2];
extern rsmp_recv_buffer_s_type g_rsmp_recv_buffer;
extern rsmp_transmit_buffer_s_type g_rsmp_transmit_buffer[THD_IDX_MAX+1];
#ifdef FEATURE_DEBUG_LOG_FILE
extern int log_fd;
extern pthread_mutex_t log_file_mtx;
#endif


/******************************Global Variables*******************************/
Dev_Info sim5360_dev_info;
Dev_Info rsmp_dev_info;

char *getcopslmn = NULL;

int startup_log_fd = -1;
char err_str[512] = {0};



#ifdef FEATURE_ENABLE_APP_DAEMON
//return value:
//	1:   Success.
//	-1: First fork() failed.
//	-2: Fail to set SIGHUP handler.
//	-3: Second fork() failed.
//	-4: Fail to "link" 0,1,2 to /dev/null.
int config_daemon(void)
{
	pid_t pid;
	struct sigaction sa;
	int fd_of_dev_null, fd0, fd1, fd2;
	
	umask(0);

	if((pid = fork()) < 0){
		LOG("First fork() failed.\n");
		return -1;
	}else if(pid != 0){
		exit(0);
	}
	setsid();

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP, &sa, NULL) < 0){
		LOG("Fail to ignore SIGHUP.\n");
		return -2;
	}
	if((pid = fork()) < 0){
		LOG("Second fork() failed.\n");
		return -3;
	}else if(pid != 0){
		exit(0);
	}

	close(0);
	close(1);
	close(2);
	fd_of_dev_null = open("/dev/null", O_RDWR);
	fd0 = dup2(fd_of_dev_null, 0);
	fd1 = dup2(fd_of_dev_null, 1);
	fd2 = dup2(fd_of_dev_null, 2);
	if(fd0!=0 || fd1!=1 ||fd2!=2){
		LOG("Fail to link file descriptor 0,1,2 to /dev/null.\n");
		return -4;
	}
	
	LOG("Succeed to daemonize the app.\n");
	return 0;
}

int isAlreadyRunning(void){
	int fd;
	int ret;
	struct flock lock_cfg;
	
	lock_cfg.l_type = F_WRLCK;
	lock_cfg.l_start = 0;
	lock_cfg.l_whence = SEEK_SET;
	lock_cfg.l_len = 0;
	lock_cfg.l_pid = getpid();
	
	fd = open(LOCKFILE, O_RDWR|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fd < 0){
		LOG("Unable to open %s. error=%s.", LOCKFILE, strerror(errno));
		return 0;
	}
	
	ret = fcntl(fd, F_SETLK, &lock_cfg);
	if(ret == -1){
		LOG("Unable to lock %s. error=%s.", LOCKFILE, strerror(errno));
		close(fd);
		return 0;
	}
	
	return 1;
}
#endif // FEATURE_ENABLE_APP_DAEMON

//Param:
//	rsp: The pointer which pointed to the buffer storing the cmd rsp.
//		   If the rsp = null, sys_call() will not acq cmd rsp.
//		   Now, sys_call() only acq 1 line of rsp. Other lines will be abandoned.
//Return Values:
//	1: Succeed in executing cmd.
//	0: Fail to execute cmd.
//  -1: sys_call() error.
//Note:
//	Please make sure that the buffer *rsp has enough space for storing cmd rsp.
static int sys_call(const char *cmd, char *rsp)
{
	FILE *stream = NULL;
	char tmp_buf[256] = {0};
	bool isNeedRsp = false;
	
	if(cmd == NULL){
		LOG("ERROR: Wrong param input. cmd=null.\n");
		return -1;
	}
	if(rsp != NULL){
		isNeedRsp = true;
	}
	
	stream = popen(cmd, "r");
	if(NULL != stream){
		if(isNeedRsp){
			// No echo str!
			memset(tmp_buf, 0, sizeof(tmp_buf));
			if(fgets(tmp_buf, sizeof(tmp_buf), stream) != NULL){
				memcpy(rsp, tmp_buf, strlen(tmp_buf));
			}else{
				LOG("ERROR: Error in acqing rsp! errno=%s(%d) \n",strerror(errno), errno);
				return -1;
			}
		}
		pclose(stream);
	}else{
		LOG("ERROR: popen() failed. errno=%s(%d).\n",
				strerror(errno), errno);
		return 0;
	}
	
	return 1;
}

//Description:
//	Check if the file or dir of path is existed.
//Return Values:
//	1 : File or dir existed.
//	0 : No such file or dir.
//   -1: check_if_file_exist() failed.
static int check_if_file_exist(const char *path)
{
	int cmd_len = qlinkdatanode_MAX_PATH_NAME_LENGTH+sizeof("ls ")-1;
	int rsp_len = qlinkdatanode_MAX_PATH_NAME_LENGTH+SYS_CMD_LS_FAILURE_RSP_FIXED_LEN;
	char cmd_str[cmd_len], rsp_buf[rsp_len], failure_str[] = "No such file or directory";
	int ret = -1, result = -1;
	
	if(path == NULL){
		LOG("ERROR: Wrong param input. path=null.\n");
		return -1;
	}
	if(*path == '\0'){
		LOG("ERROR: No cmd found in buffer *path.\n");
		return -1;
	}
	
	memset(cmd_str, 0, sizeof(cmd_str));
	memset(rsp_buf, 0, sizeof(rsp_buf));
	
    strncpy(cmd_str, "ls ", sizeof("ls ")-1);
	strncat(cmd_str, path, strlen(path));
	ret = sys_call(cmd_str, rsp_buf);
	if(ret < 0 || ret == 0){
		return -1;
	}else{
		if(strstr(rsp_buf, failure_str) != NULL){
			result = 0;
		}else{
			result = 1;
		}
	}
	
	return result;
}

void clr_dial_msg_client_file(void)
{
	int ret = -1;
	
	ret = check_if_file_exist("/usr/bin/qlinkdatanode_unix_client*");
	if(ret == 1){
		sys_call("rm /usr/bin/qlinkdatanode_unix_client*", NULL);
	}
	
	return;
}

void clr_history(void)
{
	clr_dial_msg_client_file();
	
	return;
}

//cmd format:
// BIN_NAME -d DEVICE_ADDRESS -s SERVER_ADDRESS
//feature macro:
//	FEATURE_WAIT_FOR_FIRST_REQUEST-----Not to acquire data initiatively.
//params:
// argv[0]: mdmEmodem device file
// argv[1]: server IP address
int main(int argc, char **argv)
{
  int ret, i;
  int evtloop_pipe_fd[2], chan_num_pipe_fd[2], flow_state_pipe_fd[2], thread_idx_pipe_fd[2];
  
#ifdef FEATURE_ENABLE_APP_DAEMON
  char *cmd = NULL;
  struct sigaction sa;
#endif

#ifdef FEATURE_DEBUG_LOG_FILE
  int open_flag = O_WRONLY|O_CREAT|O_APPEND;
  int file_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
  pthread_mutexattr_t mtx_attr;
#endif

#ifdef FEATURE_DEBUG_LOG_FILE
  log_fd = open(LOG_FILE_PATH, open_flag, file_mode);
  if(log_fd == -1){
    printf("ERROR: open() failed. errno=%d.\n", errno);
    return -1;
  }

  pthread_mutexattr_init(&mtx_attr);  
  pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&log_file_mtx, &mtx_attr);
  pthread_mutexattr_destroy(&mtx_attr); 
#endif

#ifdef FEATURE_DEBUG_LOG_FILE
  LOG3("*************************************************************************\n");
  LOG3("*****Official Version: %s*****Compile Date: %s %s*****\n", OFFICIAL_VER, __DATE__, __TIME__);
  LOG3("*************************************************************************\n");
#else
  LOG2("*************************************************************************\n");
  LOG2("*****Official Version: %s*****Compile Date: %s %s*****\n", OFFICIAL_VER, __DATE__, __TIME__);
  LOG2("*************************************************************************\n");
#endif

#ifdef FEATURE_VER_QRY
	{
		FILE *ver_file_strm = NULL;
		
		ver_file_strm = fopen(VER_FILE_PATH, "w+");
		if(ver_file_strm == NULL){
			LOG("ERROR: fopen(\"%s\") failed. errno=%d.\n", VER_FILE_PATH, errno);
			truncate(VER_FILE_PATH, 0);
			goto __EXIT_OF_PRINT_VER_TO_FILE__;
		}

		fprintf(ver_file_strm, 
						"*****Official Version: %s*****Compile Date: %s %s*****\n", 
						OFFICIAL_VER,
						__DATE__, 
						__TIME__
					 );
		
	__EXIT_OF_PRINT_VER_TO_FILE__:
		fclose(ver_file_strm);
	}
	#endif
	
	#ifdef FEATURE_MDMEmodem_BOOT_STAT_QRY
		//Avoid that abnormal shutdown or reboot caused inproper boot status in related file.
		write_file(MDMEmodem_BOOT_STAT_FILE_PATH, 0, "0");
	#endif
	

  if(argc != 5 && argc != 6){
    LOG("ERROR: Need more arguments. argc=%d.\n", argc);
    return -1;
  }

#ifdef FEATURE_ENABLE_APP_DAEMON
	ret = config_daemon();
	if(ret != 0){
		return -1;
	}

  if((cmd = strrchr(argv[0], '/')) == NULL)
    cmd = argv[0];
  else
    cmd++;
  
  openlog(cmd, LOG_CONS, LOG_DAEMON);

  if(isAlreadyRunning() != 1){
    LOG("ERROR: Reduplicate daemon app found.\n");
    return -1;
  }

	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction(SIGHUP, &sa, NULL) < 0){
		LOG("ERROR: Unable to restore SIGHUP default setting.\n");
		return -1;
	}
	//Enable all signals. When app is in debug mode, SIGTERM is needed to terminate the app by user.
#endif // FEATURE_ENABLE_APP_DAEMON

  clr_history();

  ret = pipe(evtloop_pipe_fd);
  if (ret < 0) {
    LOG("ERROR: pipe(evtloop_pipe_fd) error. errno=%d.\n", errno);
    return -1;
  }
  evtloop_pipe[0] = evtloop_pipe_fd[0];
  evtloop_pipe[1] = evtloop_pipe_fd[1];
  fcntl(evtloop_pipe[0], F_SETFL, O_NONBLOCK);

  ret = pipe(chan_num_pipe_fd);
  if (ret < 0) {
    LOG("ERROR: pipe(chan_num_pipe_fd) error. errno=%d.\n", errno);
    return -1;
  }
  chan_num_pipe[0] = chan_num_pipe_fd[0];
  chan_num_pipe[1] = chan_num_pipe_fd[1];
  fcntl(chan_num_pipe[0], F_SETFL, O_NONBLOCK);

  ret = pipe(flow_state_pipe_fd);
  if (ret < 0) {
    LOG("ERROR: pipe(flow_state_pipe_fd) error. errno=%d.\n", errno);
    return -1;
  }
  flow_state_pipe[0] = flow_state_pipe_fd[0];
  flow_state_pipe[1] = flow_state_pipe_fd[1];
  fcntl(flow_state_pipe[0], F_SETFL, O_NONBLOCK);

  ret = pipe(thread_idx_pipe_fd);
  if (ret < 0) {
    LOG("ERROR: pipe(thread_idx_pipe_fd) error. errno=%d.\n", errno);
    return -1;
  }
  thread_idx_pipe[0] = thread_idx_pipe_fd[0];
  thread_idx_pipe[1] = thread_idx_pipe_fd[1];
  fcntl(thread_idx_pipe[0], F_SETFL, O_NONBLOCK);


  //Init dev_info and dev_info_s
  memset(&sim5360_dev_info, 0, sizeof(sim5360_dev_info));
  memset(&rsmp_dev_info, 0, sizeof(rsmp_dev_info));
  //Init req_config_array
  for( i=0; i<(int)THD_IDX_MAX; i++){
    g_rsmp_transmit_buffer[i].size = -1;
    g_rsmp_transmit_buffer[i].data = NULL;
    g_rsmp_transmit_buffer[i].StatInfMsgSn = 0;
  }

#ifdef FEATURE_ENABLE_MSGQ
  ret = startMsgLoop();
  if(ret != 1){
    LOG("ERROR: startMsgLoop() error.\n");
    return -1;
  }
#endif

  ret = startEvtLoopThread();
  if(ret != 1){
    LOG("ERROR: startEvtLoop() error.\n");
    return -1;
  }

#ifdef FEATURE_CHECK_STARTUP_OF_MDMEmodem
  ret = uart_check_mdmEmodem();
  if(ret == 0){
    LOG("ERROR: mdmEmodem is not started!\n");
    return -1;
  }
#endif

#ifdef FEATURE_ENABLE_STATUS_REPORT_10_MIN
			 ret = startStatusReportThread();
			  if(ret != 1){
				  LOG("ERROR: startStatusReportThread thread is fali.\n");
				  return -1;
			  }
#endif




  ret = startSerialComm(argc, argv);
  if(ret != 1){
    LOG("ERROR: startSerialComm() error.\n");
    return -1;
  }

  startQueue();

  g_rsmp_recv_buffer.size = 0;
  g_rsmp_recv_buffer.data = NULL;

  ret = startQMIParserThread();
  if(ret != 1){
    LOG("ERROR: startQMIParser() error.\n");
    return -1;
  }

  ret = startQMIReceiverThread();
  if(ret != 1){
    LOG("ERROR: startQMIReceiverThread() error.\n");
    return -1;
  }

  ret = startQMISender();
  if(ret != 1){
    LOG("ERROR: startQMISender() error.\n");
    return -1;
  }

  ret = startQMIUimSender(); 
  if(ret != 1){
    LOG("ERROR: startQMIUimSender() error.\n");
    return -1;
  }

  ret = startQMINasSender();
  if(ret != 1){
    LOG("ERROR: startQMINasSender() error.\n");
    return -1;
  }

  ret = startQMIDmsSender();
  if(ret != 1){
    LOG("ERROR: startQMIDmsSender() error.\n");
    return -1;
  }

  ret = startUrcMonitorThread();
  if(ret != 1){
    LOG("ERROR: startUrcMonitorThread() error.\n");
    return -1;
  }

  ret = startSockListenerLoop();
  if(ret != 1){
    LOG("ERROR: startSockListenerLoop() error.\n");
    return -1;
  }

#ifdef FEATURE_ENABLE_SYSTEM_RESTORATION
  startRstMonitor();
#endif

  for(;;){
    sleep(0x00ffffff);
  }

  LOG("ERROR: Main thread ended unexpectedly.\n");
  return -1;
}
