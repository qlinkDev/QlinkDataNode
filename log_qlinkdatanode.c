#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h> // write
#include <fcntl.h>
#include <stddef.h> // ssize_t
#include <time.h> // time localtime
#include <errno.h>
#include <sys/stat.h> // stat
#include <sys/file.h> // flock
#include <syslog.h>
#include <pthread.h>
#include <sys/time.h> // gettimeofday

#include "msg.h"
#include "msgcfg.h"

#include "feature_macro_qlinkdatanode.h"
#include "common_qlinkdatanode.h"

#define LOG_FILE_PATH "/usr/bin/qlinkdatanode.log"
#define OLD_LOG_FILE_PATH "/usr/bin/qlinkdatanode_old.log"
#define MAX_LOG_FILE_SIZE (3*1024*1024/2) /* 1.5 MB */

/******************************External Functions*******************************/
extern int count_num_digits_v01(unsigned int num);


/******************************Global Variables*******************************/
int log_fd = -1;
pthread_mutex_t log_file_mtx;



void print_to_QXDM_no_pos(const char *fmt,...)
{
  char buf[DS_MAX_DIAG_LOG_MSG_SIZE] = {0};
  va_list ap;
  
  va_start(ap, fmt);
  vsnprintf(buf, DS_MAX_DIAG_LOG_MSG_SIZE, fmt, ap);
  va_end(ap);
  
  //Use MSG_LEGACY_FATAL to increase discriminability in QXDM.
  MSG_SPRINTF_1(MSG_SSID_LINUX_DATA, MSG_LEGACY_FATAL, "%s", buf);
  
  return;
}

void print_to_QXDM_internal(const char *log)
{
	if(NULL == log)
		return;
	
	//Use MSG_LEGACY_FATAL to increase discriminability in QXDM.
	MSG_SPRINTF_1(MSG_SSID_LINUX_DATA, MSG_LEGACY_FATAL, "%s", log);
	
	return;
}

#ifdef FEATURE_DEBUG_SYSLOG
void print_to_syslog(const char *fmt,...)
{
	#error code not present
	return;
}
#endif

//Description:
//	Print cur time mark to the beginning of ptr "buf".
//Return Values:
//	>=0 : The char bytes that have been copied into the position which is pointed by arg "buf".
int print_timestamp_to_buf(void *buf)
{
	#define MAX_PRTBUF_LEN (64)
	
	time_t tt;
	struct tm *tm;
	struct timeval tv;
	int prtbuf_offset = 0, zero_count = 0, max_zero_count = 0;
	char prtbuf[MAX_PRTBUF_LEN] = {0};
	
	if(NULL == buf)
		return 0;
	
	time(&tt);
	tm = localtime(&tt);
	gettimeofday(&tv, NULL);
	prtbuf_offset = snprintf(prtbuf, MAX_PRTBUF_LEN, "%02d/%02d %02d:%02d:%02d.", 
										tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	max_zero_count = 3-count_num_digits_v01((unsigned int)(tv.tv_usec/1000)); //If tv.tv_usec/1000 equals 0 is OK
	for(zero_count=0; zero_count<max_zero_count; zero_count++){
		snprintf(prtbuf+prtbuf_offset, MAX_PRTBUF_LEN-prtbuf_offset, "%c", 48);
		prtbuf_offset ++;
	}
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_PRTBUF_LEN-prtbuf_offset, "%ld ", tv.tv_usec/1000);
	
	memcpy(buf, (void *)prtbuf, prtbuf_offset);
	
	return prtbuf_offset;
}

/*
void print_to_shell_no_pos(const char *fmt,...)
{
	va_list args;
	
	va_start(args, fmt);
	
	printf(fmt, args);
	
	va_end(args);
	
	return;
}
*/

void print_to_file(const char *file_name, 
									const unsigned int line_num, 
									const char *func_name,
									const char *log)
{
	unsigned long file_size =0;
	struct stat statbuf;
	char prtbuf[MAX_LOG_BUF_LEN] = {0};
	int orig_errno, prtbuf_len = 0, rm_flag = 0;
	time_t tt;
	struct tm *tm;
	struct timeval tv;
	int prtbuf_offset = 0, zero_count = 0, max_zero_count = 0;
	ssize_t written_bytes = 0;
	
	pthread_mutex_lock(&log_file_mtx);
	
	orig_errno = errno;
	
	time(&tt);
	tm = localtime(&tt);
	gettimeofday(&tv, NULL);
	prtbuf_offset = snprintf(prtbuf, MAX_LOG_BUF_LEN, "%02d/%02d %02d:%02d:%02d.", 
													tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);	
	max_zero_count = 3-count_num_digits_v01((unsigned int)(tv.tv_usec/1000)); //If tv.tv_usec/1000 equals 0 is OK
	for(zero_count=0; zero_count<max_zero_count; zero_count++){
		snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%c", 48);
		prtbuf_offset ++;
	}
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%ld ", tv.tv_usec/1000);
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%s(%d)%s%s: ", 
											(file_name==NULL?"null":file_name), 
											line_num, 
											(func_name==NULL?"":"-"), 
											(func_name==NULL?"":func_name)
											);
	snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%s", log);
	
	if(flock(log_fd, LOCK_EX) == 0){        
		if(stat(LOG_FILE_PATH, &statbuf) >= 0){
			file_size = statbuf.st_size;
		}else{
			//The possible cases are that errno equals EACCESS or ENOMEM.
			rm_flag = 1;
		}
        

		if(file_size >= MAX_LOG_FILE_SIZE){
			rm_flag = 1;
			
			//Fixed by rjf at 20151010 
			//	Can't miss the log content of this time when file_size is not smaller than MAX_LOG_FILE_SIZE.
//			goto __EXIT_OF_PRINT_TO_LOGFILE__;
		}
		
		prtbuf_len = strlen(prtbuf);
    __WRITE_LOG_AGAIN__:
		written_bytes = write(log_fd, prtbuf, prtbuf_len);
		if(written_bytes != prtbuf_len){
			if((written_bytes == -1 && errno == EINTR) || written_bytes == 0){
				goto __WRITE_LOG_AGAIN__;
			}else if(written_bytes == -1){
//    		printf("ERROR: write() failed. errno=%d.\n", errno);
			}else{
//    		printf("ERROR: Some log missed. written_bytes=%d, prtbuf_len=%d.",
//             	written_bytes, prtbuf_len);
			}
		}
	}

//	__EXIT_OF_PRINT_TO_LOGFILE__:
	flock(log_fd, LOCK_UN);
	
	if(rm_flag == 1){
		unlink(OLD_LOG_FILE_PATH);
		
		close(log_fd);
		rename(LOG_FILE_PATH, OLD_LOG_FILE_PATH);
		
		log_fd = open(LOG_FILE_PATH, 
								O_WRONLY|O_CREAT|O_APPEND, 
								S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
    
	errno = orig_errno;
	
	pthread_mutex_unlock(&log_file_mtx);
	
	return;
}

void print_to_file_no_lck(const char *file_name, 
													const unsigned int line_num, 
													const char *func_name,
													const char *log)
{
	unsigned long file_size =0;
    struct stat statbuf;
	char prtbuf[MAX_LOG_BUF_LEN] = {0};
	int orig_errno, prtbuf_len = 0, rm_flag = 0;
	time_t tt;
	struct tm *tm;
	struct timeval tv;
	int prtbuf_offset = 0, zero_count = 0, max_zero_count = 0;
	ssize_t written_bytes = 0;
	
	orig_errno = errno;
	
	time(&tt);
	tm = localtime(&tt);
	gettimeofday(&tv, NULL);
	prtbuf_offset = snprintf(prtbuf, MAX_LOG_BUF_LEN, "%02d/%02d %02d:%02d:%02d.", 
													tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	max_zero_count = 3-count_num_digits_v01((unsigned int)(tv.tv_usec/1000)); //If tv.tv_usec/1000 equals 0 is OK
	for(zero_count=0; zero_count<max_zero_count; zero_count++){
		snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%c", 48);
		prtbuf_offset ++;
	}
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%ld ", tv.tv_usec/1000);
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%s(%d)%s%s: ", 
											(file_name==NULL?"null":file_name), 
											line_num, 
											(func_name==NULL?"":"-"), 
											(func_name==NULL?"":func_name)
											);
	snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%s", log);
	
	if(flock(log_fd, LOCK_EX) == 0){
		if(stat(LOG_FILE_PATH, &statbuf) >= 0){
			file_size = statbuf.st_size;
		}else{
			//The possible cases are that errno equals EACCESS or ENOMEM.
			rm_flag = 1;
		}


		if(file_size >= MAX_LOG_FILE_SIZE){
			rm_flag = 1;
			
			//Fixed by rjf at 20151010 
			//	Can't miss the log content of this time when file_size is not smaller than MAX_LOG_FILE_SIZE.
//			goto __EXIT_OF_PRINT_TO_LOGFILE__;
		}
		
		prtbuf_len = strlen(prtbuf);
		__WRITE_LOG_AGAIN__:
		written_bytes = write(log_fd, prtbuf, prtbuf_len);
		if(written_bytes != prtbuf_len){
			if((written_bytes == -1 && errno == EINTR) || written_bytes == 0){
				goto __WRITE_LOG_AGAIN__;
			}else if(written_bytes == -1){
//    		printf("ERROR: write() failed. errno=%d.\n", errno);
			}else{
//    		printf("ERROR: Some log missed. written_bytes=%d, prtbuf_len=%d.",
//             	written_bytes, prtbuf_len);
			}
		}
	}

//	__EXIT_OF_PRINT_TO_LOGFILE__:
	flock(log_fd, LOCK_UN);
	
	if(rm_flag == 1){
		unlink(OLD_LOG_FILE_PATH);
		
		close(log_fd);
		rename(LOG_FILE_PATH, OLD_LOG_FILE_PATH);
		
		log_fd = open(LOG_FILE_PATH, 
								O_WRONLY|O_CREAT|O_APPEND, 
								S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
    
	errno = orig_errno;
	
	return;
}

//Applicable Situation:
//	Print seperator line.
void print_to_file_no_pos_n_lck(const char *fmt,...)
{
	va_list args;
	unsigned long file_size =0;
    struct stat statbuf;
	char prtbuf[MAX_LOG_BUF_LEN] = {0};
	int orig_errno, prtbuf_len = 0, rm_flag = 0;
	time_t tt;
	struct tm *tm;
	struct timeval tv;
	int prtbuf_offset = 0, zero_count = 0, max_zero_count = 0;
	ssize_t written_bytes = 0;
	
	orig_errno = errno;
	memset(prtbuf, 0, MAX_LOG_BUF_LEN);
        	
	va_start(args, fmt);
	
	time(&tt);
	tm = localtime(&tt);
	gettimeofday(&tv, NULL);
	prtbuf_offset = snprintf(prtbuf, MAX_LOG_BUF_LEN, "%02d/%02d %02d:%02d:%02d.", 
													tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	max_zero_count = 3-count_num_digits_v01((unsigned int)(tv.tv_usec/1000)); //If tv.tv_usec/1000 equals 0 is OK
	for(zero_count=0; zero_count<max_zero_count; zero_count++){
		snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%c", 48);
		prtbuf_offset ++;
	}
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%ld ", tv.tv_usec/1000);
	vsnprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, fmt, args);
	
	va_end(args);
	
	if(flock(log_fd, LOCK_EX) == 0){		
		if(stat(LOG_FILE_PATH, &statbuf) >= 0){
			file_size = statbuf.st_size;
		}else{
			//The possible cases are that errno equals EACCESS or ENOMEM.
			rm_flag = 1;
		}

		if(file_size >= MAX_LOG_FILE_SIZE){
			rm_flag = 1;
			
			//Fixed by rjf at 20151010 
			//	Can't miss the log content of this time when file_size is not smaller than MAX_LOG_FILE_SIZE.
//			goto __EXIT_OF_PRINT_TO_LOGFILE__;
		}
		
		prtbuf_len = strlen(prtbuf);
    __WRITE_LOG_AGAIN__:
		written_bytes = write(log_fd, prtbuf, prtbuf_len);
		if(written_bytes != prtbuf_len){
			if((written_bytes == -1 && errno == EINTR) || written_bytes == 0){
				goto __WRITE_LOG_AGAIN__;
			}else if(written_bytes == -1){
//    		printf("ERROR: write() failed. errno=%d.\n", errno);
			}else{
//    		printf("ERROR: Some log missed. written_bytes=%d, prtbuf_len=%d.",
//             	written_bytes, prtbuf_len);
			}
		}
	}

//	__EXIT_OF_PRINT_TO_LOGFILE__:
	flock(log_fd, LOCK_UN);
	
	if(rm_flag == 1){
		unlink(OLD_LOG_FILE_PATH);
		
		close(log_fd);
		rename(LOG_FILE_PATH, OLD_LOG_FILE_PATH);
		
		log_fd = open(LOG_FILE_PATH, 
								O_WRONLY|O_CREAT|O_APPEND, 
								S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
    
	errno = orig_errno;
	
	return;
}

//Applicable Situation:
//	Print each byte of data in buffer.
void print_to_file_no_pos_n_lck_n_time(const char *fmt,...)
{
	va_list args;
	unsigned long file_size =0;
    struct stat statbuf;
	char prtbuf[MAX_LOG_BUF_LEN] = {0};
	int orig_errno, prtbuf_len = 0, rm_flag = 0;
	ssize_t written_bytes = 0;
	
	orig_errno = errno;
	memset(prtbuf, 0, MAX_LOG_BUF_LEN);
        	
	va_start(args, fmt);
	
	vsnprintf(prtbuf, MAX_LOG_BUF_LEN, fmt, args);
	
	va_end(args);
	
	if(flock(log_fd, LOCK_EX) == 0){
		if(stat(LOG_FILE_PATH, &statbuf) >= 0){
			file_size = statbuf.st_size;
		}else{
			//The possible cases are that errno equals EACCESS or ENOMEM.
			rm_flag = 1;
		}

		if(file_size >= MAX_LOG_FILE_SIZE){
			rm_flag = 1;
			
			//Fixed by rjf at 20151010 
			//	Can't miss the log content of this time when file_size is not smaller than MAX_LOG_FILE_SIZE.
//			goto __EXIT_OF_PRINT_TO_LOGFILE__;
		}
		
		prtbuf_len = strlen(prtbuf);
    __WRITE_LOG_AGAIN__:
		written_bytes = write(log_fd, prtbuf, prtbuf_len);
		if(written_bytes != prtbuf_len){
			if((written_bytes == -1 && errno == EINTR) || written_bytes == 0){
				goto __WRITE_LOG_AGAIN__;
			}else if(written_bytes == -1){
//    		printf("ERROR: write() failed. errno=%d.\n", errno);
			}else{
//    		printf("ERROR: Some log missed. written_bytes=%d, prtbuf_len=%d.",
//             	written_bytes, prtbuf_len);
			}
		}
	}

//	__EXIT_OF_PRINT_TO_LOGFILE__:
	flock(log_fd, LOCK_UN);
	
	if(rm_flag == 1){
		unlink(OLD_LOG_FILE_PATH);
		
		close(log_fd);
		rename(LOG_FILE_PATH, OLD_LOG_FILE_PATH);
		
		log_fd = open(LOG_FILE_PATH, 
								O_WRONLY|O_CREAT|O_APPEND, 
								S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
    
	errno = orig_errno;
	
	return;
}

//Applicable Situation:
//	Print first-byte data in each line of data log on buffer. 
void print_to_file_no_pos_n_lck_but_space(const char *fmt,...)
{
	va_list args;
	unsigned long file_size =0;
    struct stat statbuf;
	char prtbuf[MAX_LOG_BUF_LEN] = {0};
	int orig_errno, prtbuf_len = 0, rm_flag = 0;
	time_t tt;
	struct tm *tm;
	struct timeval tv;
	int prtbuf_offset = 0, zero_count = 0, max_zero_count = 0;
	ssize_t written_bytes = 0;
	
	orig_errno = errno;
	memset(prtbuf, 0, MAX_LOG_BUF_LEN);
        	
	va_start(args, fmt);
	
	time(&tt);
	tm = localtime(&tt);
	gettimeofday(&tv, NULL);
	prtbuf_offset = snprintf(prtbuf, MAX_LOG_BUF_LEN, "%02d/%02d %02d:%02d:%02d.", 
													tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	max_zero_count = 3-count_num_digits_v01((unsigned int)(tv.tv_usec/1000)); //If tv.tv_usec/1000 equals 0 is OK
	for(zero_count=0; zero_count<max_zero_count; zero_count++){
		snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%c", 48);
		prtbuf_offset ++;
	}
	prtbuf_offset += snprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, "%ld ", tv.tv_usec/1000);
	memset(prtbuf, 32, prtbuf_offset);
	vsnprintf(prtbuf+prtbuf_offset, MAX_LOG_BUF_LEN-prtbuf_offset, fmt, args);
	
	va_end(args);
	
	if(flock(log_fd, LOCK_EX) == 0){
		if(stat(LOG_FILE_PATH, &statbuf) >= 0){
			file_size = statbuf.st_size;
		}else{
			//The possible cases are that errno equals EACCESS or ENOMEM.
			rm_flag = 1;
		}

		if(file_size >= MAX_LOG_FILE_SIZE){
			rm_flag = 1;
			
			//Fixed by rjf at 20151010 
			//	Can't miss the log content of this time when file_size is not smaller than MAX_LOG_FILE_SIZE.
//			goto __EXIT_OF_PRINT_TO_LOGFILE__;
		}
		
		prtbuf_len = strlen(prtbuf);
    __WRITE_LOG_AGAIN__:
		written_bytes = write(log_fd, prtbuf, prtbuf_len);
		if(written_bytes != prtbuf_len){
			if((written_bytes == -1 && errno == EINTR) || written_bytes == 0){
				goto __WRITE_LOG_AGAIN__;
			}else if(written_bytes == -1){
//    		printf("ERROR: write() failed. errno=%d.\n", errno);
			}else{
//    		printf("ERROR: Some log missed. written_bytes=%d, prtbuf_len=%d.",
//             	written_bytes, prtbuf_len);
			}
		}
	}

//	__EXIT_OF_PRINT_TO_LOGFILE__:
	flock(log_fd, LOCK_UN);
	
	if(rm_flag == 1){
		unlink(OLD_LOG_FILE_PATH);
		
		close(log_fd);
		rename(LOG_FILE_PATH, OLD_LOG_FILE_PATH);
		
		log_fd = open(LOG_FILE_PATH, 
								O_WRONLY|O_CREAT|O_APPEND, 
								S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }
    
	errno = orig_errno;
	
	return;
}

void print_to_null(const char *fmt,...){}

#ifdef FEATURE_ENABLE_FMT_PRINT_DATA_LOG
//It is for QXDM log.
void print_fmt_data(char *str)
{
	int line_num = strlen(str)/3/25+((strlen(str)/3%25>0)?1:0), line_len = 25*3, i = 0;
	char prtbuf[line_len+1];
	
	if(str == NULL)	return;
	
	for(; i<line_num; i++){
		memset(prtbuf, 0, sizeof(prtbuf));
		memcpy(prtbuf, str+i*line_len, line_len);
		print_to_QXDM_no_pos("   %s", prtbuf);
	}
	
	return;
}
#endif

#if 0
#define YY_CFG_PATH  "/etc/yy_cfg"
#define YY_CFG_NAME_MAX_LEN  40 
#define YY_CFG_VALUE_MAX_LEN  20 
#define YY_LOG_TO_QXDM 0 
#define YY_LOG_TO_FILE  1

int yy_get_log_mode()
{
    int mode;
    char config_buff[YY_CFG_VALUE_MAX_LEN];

    memset(config_buff,0,YY_CFG_VALUE_MAX_LEN);
 
    YyReadConfig(YY_CFG_PATH,"YyWriteLogToQxdmOrFile",config_buff);
    mode = atoi(config_buff);
    return mode;
}

YyReadConfig(char *conf_path,char *conf_name,char *config_buff)
{
    char config_linebuf[256];
    char line_name[40];
    char exchange_buf[256];
    char *config_sign = "=";
    char *leave_line;
    FILE *f;
    int leave_num;
 
    f = fopen(conf_path,"r");
    if(f == NULL){
        printf("OPEN CONFIG FALID\n");
        return 0;
    }
    fseek(f,0,SEEK_SET); 
    while(fgets(config_linebuf,256,f) != NULL)
    {   
        if(strlen(config_linebuf) < 3){
            continue;
        }
        if (config_linebuf[strlen(config_linebuf)-1] == 10) {  
            memset(exchange_buf,0,sizeof(exchange_buf));
            strncpy(exchange_buf,config_linebuf,strlen(config_linebuf)-1);
            memset(config_linebuf,0,sizeof(config_linebuf));
            strcpy(config_linebuf,exchange_buf);
        }
        memset(line_name,0,sizeof(line_name));
        leave_line = strstr(config_linebuf,config_sign);
        if(leave_line == NULL){
            continue;
        }
        leave_num = leave_line - config_linebuf;
        strncpy(line_name,config_linebuf,leave_num);
        if(strcmp(line_name,conf_name) ==0){
            strncpy(config_buff,config_linebuf+(leave_num+1),strlen(config_linebuf)-leave_num-1);
            break;
        }
        if(fgetc(f)==EOF){
            break;  
        }
        fseek(f,-1,SEEK_CUR);
        memset(config_linebuf,0,sizeof(config_linebuf));
    }
    fclose(f);
}
#endif
