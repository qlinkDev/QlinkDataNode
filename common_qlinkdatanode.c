#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h> //open()
#include <sys/stat.h> //open()
#include <fcntl.h> //open()
#include <errno.h> //errno

#include "feature_macro_qlinkdatanode.h"
#include "common_qlinkdatanode.h"

#ifdef FEATURE_ENABLE_CHANNEL_LOCK_LOG
#define CHAN_LCK_NUM_DEFINED
#endif
#ifdef FEATURE_ENABLE_ACQ_DATA_STATE_MTX_LOG
#define ACQ_DATA_STATE_MTX_NUM_DEFINED
#endif

#include "common_qlinkdatanode.h"
#include "qmi_parser_qlinkdatanode.h" // APDU_Setting
#include "protocol_qlinkdatanode.h" // flow_info

#ifndef FEATURE_ENABLE_COMMON_qlinkdatanode_C_LOG
#define LOG print_to_null
#endif


/******************************External Variables*******************************/
extern Acq_Data_State acq_data_state;
extern pthread_mutex_t acq_data_state_mtx;
extern pthread_mutex_t log_file_mtx;

/******************************Global Variables*******************************/
rsmp_recv_buffer_s_type g_rsmp_recv_buffer;

//Function:
//It will be only set true in add_node().
bool isQueued = false;

//Function:
//If it is the first time that start app, it will be set true. It will be set false when app managed to connect
//to server.
bool isProcOfInitApp = true;

int evtloop_pipe[2];
//Description:
//evtloop_mutex is for creating pthread "EvtLoop";
//evtloop_mtx is for syncing threads which utilize evtloop_pipe.
pthread_mutex_t evtloop_mtx = PTHREAD_MUTEX_INITIALIZER;

int chan_num_pipe[2];
int flow_state_pipe[2];
int thread_idx_pipe[2];

//function:
// -1: mdmEmodem and mdmImodem is unavailable for their network link. And mdmEmodem is being recovered.
//  0: mdmEmodem is available for its network link.
//  1: mdmImodem is available for its network link.
//  2: mdmEmodem and mdmImodem is available for their network link. And mdmEmodem is being suspended.
//  3: mdmEmodem and mdmImodem is available for their network link. And app is trying to transmit APDU via mdmEmodem.
//  4: The network link of mdmEmodem is broken, and app is trying to recover it.

//  Emodem   Imodem    ChannelNum
//  no(!)   no     -1
//  yes     no      0
//  no      yes     1
//  yes(!)  yes     2 //go suspend
//  yes     yes     3
//  yes(*)  yes     4 //go resume
int ChannelNum = -1;

#ifdef FEATURE_ENABLE_RWLOCK_CHAN_LCK
pthread_rwlock_t ChannelLock;
#else
pthread_mutex_t ChannelLock;
#endif

//Function:
//If mdmImodem has been set online mode, this var will be set true. Otherwise, it will be set false.
bool isOnlineMode = false;

rsmp_transmit_buffer_s_type g_rsmp_transmit_buffer[THD_IDX_MAX+1]; //Add 1 for that get_thread_idx() return THD_IDX_MAX
APDU_Setting APDU_setting = {false, 0x00, 0x00, {0}};

rsp_cclk_time_type rsp_Emodemcclk_ime;

rsmp_control_block_type flow_info = {{0}, FLOW_STATE_IDLE, REQ_TYPE_IDLE, false, 0xff, NULL, 0};

#ifdef FEATURE_ENABLE_CHANNEL_LOCK_LOG
int chan_lck_num = 0;
#endif
#ifdef FEATURE_ENABLE_ACQ_DATA_STATE_MTX_LOG
int acq_data_state_mtx_num = 0;
#endif

unsigned long thread_id_array[THD_IDX_MAX] = {0};

//Note:
//	Arg num must be less than 10000. (<= 9999)
int count_num_digits_v01(unsigned int num)
{
	int digit = 0;
	
	if(num/1000 != 0)
		digit = 4;
	else if(num/100 != 0)
		digit = 3;
	else if(num/10 != 0)
		digit = 2;
	else // Including num == 0
		digit = 1;
	
	return digit;
}

unsigned char get_byte_of_void_mem(void *ptr, unsigned int idx)
{
  return *((unsigned char *)ptr+idx);
}

//return value:
// 0: error
// 1: success
int check_IP_addr(const char *str)
{
	int field[4] = {0, 0, 0, 0};
	int len = strlen(str);
	int count = 0;
	int field_count = 0;
	int digit_count = 0;

	LOG(" . IP address = %s.\n", str);

	if(len>15 || len<7){
		LOG("ERROR: Wrong IP address format. Excessive address string length.\n");
		return 0;
	}
	
	for(; count<len&&field_count<4&&digit_count<3; count++){
		if( (str[count]<'0' || str[count]>'9') && str[count]!='.' ){
			LOG("ERROR: Wrong IP address format. Unexpected char found. field=%d, position=%d.\n", field_count, count);
			return 0;
		}else if(str[count] == '.'){
			continue;
		}
		
		if(digit_count <= 2){
			field[field_count] *= 10;
			field[field_count] += str[count] - '0';
			digit_count++;
			
		}else{
			LOG("ERROR: Wrong IP address format. Excessive digits of IP field. field=%d.\n", field_count);
			return 0;
		}

		if(digit_count == 3){
			if(field[field_count] > 255){
				LOG("ERROR: Wrong IP address format. Excessive value of IP field. field=%d.\n", field_count);
				return 0;
			}else{
				field_count++;
				digit_count=0;
				
				continue;
			}
		}else if(str[count+1] == '.'){
			field_count++;
			digit_count=0;
			
			continue;
		}
	}

	LOG("field_count = %d .\n", field_count);
	
	
	//field_count start at 0, and exit for loop with field_count=4
	if(field_count > 4 ){
		LOG("ERROR: Wrong IP address format. Inadequate fields of IP address.\n");
		return 0;
	}

	LOG("IP address is valid. IP address=%s.\n", str);
	return 1;
}

//return value:
// 0: error
// 1: success
int check_port(const int server_port)
{
  if(server_port > 65535 || server_port < 0){
    LOG("ERROR: Wrong parameter input.\n");
    return 0;
  }

  return 1;
}

//function:
// Only seperate IP address and port
//return value:
// 0: error
// 1: success
int parse_serv_addr(const char *str, char *IP, int *port)
{
	int count = 0;
	int len = strlen(str);

	if(str==NULL){
		LOG("ERROR: Wrong parameter input.\n");
		return 0;
	}

	for(; count<len&&str[count]!=':'; count++);
	if(count == len){
		LOG("No ':' found.\n");
		return 0;
	}else if(count == len-1){
		LOG("No port field found.\n");
		return 0;
	}else if(count == 0){
		LOG("No IP address field found.\n");
		return 0;
	}

	if(sscanf(str, "%[^:]:%d", IP, port) != 2){
		LOG("sscanf() error.\n");
		return 0;
	}
//	LOG("IP addr=%s, port=%d.\n", IP, *port);
	return 1;
}

//return value:
// 0: error
// 1: success
int check_dev_addr(const char *str)
{
	int count = 0;
	int len = strlen(str);
	
	for(; count<len; count++){
		if( !(str[count]<='z' ||str[count]>='a') &&
			!(str[count]<='Z' ||str[count]>='A') && 
			str[count]!='/' )
		{
			LOG("ERROR: Wrong device file address found.\n");
			return 0;
		}
	}
	
	return 1;
}

//TIPS:
//Use it when you need to fill buffer_config. No need to use it when you send out the data of buffer_config.
void reset_rsmp_recv_buffer(void)
{
  g_rsmp_recv_buffer.size = 0;
  if(g_rsmp_recv_buffer.data != NULL)
  {
    free(g_rsmp_recv_buffer.data);
  }
  g_rsmp_recv_buffer.data = NULL;
  return;
}

void reset_rsmp_transmit_buffer(rsmp_transmit_buffer_s_type *p_cfg)
{
  p_cfg->size = -1;
  if(p_cfg->data != NULL)
    free(p_cfg->data);
  p_cfg->data = NULL;
  return;
}

#ifdef FEATURE_ENABLE_TIMESTAMP_LOG
#include <time.h>
#include <sys/time.h>
void print_cur_gm_time(const char *i_str)
{
	struct timeval t;
	char time_buf[10];
	gettimeofday(&t, NULL);
	strftime(time_buf, 10, "%X", localtime(&t.tv_sec));
	#ifdef FEATURE_DEBUG_LOG_FILE
		pthread_mutex_lock(&log_file_mtx);
		LOG5("=======================================================\n");
		LOG4("%sCurrent time: %s.%ld\n", i_str, time_buf, t.tv_usec/1000); //ms level
		LOG4("=======================================================\n");
		pthread_mutex_unlock(&log_file_mtx);
	#else
		LOG2("=======================================================\n");
		LOG2("%sCurrent time: %s.%ld\n", i_str, time_buf, t.tv_usec/1000); //ms level
		LOG2("=======================================================\n");
	#endif
	return;
}
#endif

//Return Value:
//The ptr which points to the beginning pos of identical part of mem "src".
void *find_sub_content(const void *src, void *tgt, unsigned int len_src, unsigned int len_tgt)
{
	const unsigned char *_src = src;
	unsigned char *_tgt = tgt;
	int idx = 0, start_idx = 0, len_eql = 0;
	
	for(; idx<len_src; idx++){
		if(_tgt[len_eql] == _src[idx]){
			if(0 == len_eql)
				start_idx = idx;
			
			len_eql ++;
			
			if(len_eql == len_tgt)
				return (void *)(_src+idx-len_eql+1);
		}else{
			//Fixed by rjf at 20151023
			if(0 != len_eql){
				idx = start_idx; //Fixed by rjf at 20151029 (The idx will be increased by 1 in the end of this cycle.)
				start_idx = 0; //Not necessary
				len_eql = 0;
			}
		}
	} // for
	
	return NULL;
}

//return value:
//-1: wrong parameter input.
//param range:
//0~9, A~F and a~f (char)
//function:
//Translate one-digit hex char to decimal integer.
int one_digit_hex_to_dec(const char digit)
{
  int value;

  if(digit >= 'A' && digit <= 'F')
    value = digit - 'A' + 10;
  else if(digit >= 'a' && digit <= 'f')
    value = digit - 'a' + 10;
  else if(digit >= '0' && digit <= '9')
    value = digit - '0';
  else
    return -1;

  return value;
}

void init_dev_info(Dev_Info *di)
{
  if(di == NULL){
    return;
  }
  memset((void *)di, 0, sizeof(Dev_Info));
  return;
}

//function:
//Convert 4-byte big-endian memory content to (4-byte) integer.
unsigned int conv_4_byte_be_mem_to_val(void *mem)
{
  unsigned char *c = (unsigned char *)mem;
  return ((c[0]<<24) + (c[1]<<16) + (c[2]<<8) + c[3]);
}

void set_ChannelNum(const int val)
{
#ifdef FEATURE_ENABLE_CHANNEL_LOCK_LOG
  LOG6("~~~~ set_ChannelNum ~~~~ %d\n", val);
#endif

  ACQ_CHANNEL_LOCK_WR;
  ChannelNum = val;
  RLS_CHANNEL_LOCK;
}

int get_ChannelNum(void)
{
  int ret = -99;

  ACQ_CHANNEL_LOCK_RD;
  ret = ChannelNum;
  RLS_CHANNEL_LOCK;
  
#ifdef FEATURE_ENABLE_CHANNEL_LOCK_LOG
  LOG6("~~~~ get_ChannelNum ~~~~ %d \n", ret);
#endif

  return ret;
}

int check_acq_data_state(Acq_Data_State state)
{
  int ret = 0;
  ACQ_ACQ_DATA_STATE_MTX;
  if(state == acq_data_state)
    ret = 1;
  else
    ret = 0;
  RLS_ACQ_DATA_STATE_MTX;
  return ret;
}

void set_acq_data_state(Acq_Data_State state)
{
  ACQ_ACQ_DATA_STATE_MTX;
  acq_data_state = state;
  RLS_ACQ_DATA_STATE_MTX;
}

Acq_Data_State get_acq_data_state(void)
{
  Acq_Data_State ret = -1;
  ACQ_ACQ_DATA_STATE_MTX;
  ret = acq_data_state;
  RLS_ACQ_DATA_STATE_MTX;
  return ret;
}

rsmp_protocol_type conv_flow_state_to_req_type(rsmp_state_s_type state)
{
	rsmp_protocol_type req;
	
	switch(state){
		case FLOW_STATE_APDU_COMM:
			req = REQ_TYPE_APDU;break;
		case FLOW_STATE_ID_AUTH:
			req = REQ_TYPE_ID;break;
		case FLOW_STATE_PWR_DOWN_UPLOAD:
			req = REQ_TYPE_PWR_DOWN;break;
		case FLOW_STATE_REMOTE_UIM_DATA_REQ:
			req = REQ_TYPE_REMOTE_UIM_DATA;break;
		case FLOW_STATE_STAT_UPLOAD:
			req = REQ_TYPE_STATUS;break;
		default:{
			LOG("ERROR: Wrong flow_state (%d) found!\n", state);
			req = REQ_TYPE_IDLE;
			break;
		}
	} // switch end
	
	return req;
}

//Description:
//Set the No."idx" bit of "*p_src_val" to "bit_val".
//Pamameters:
//idx: Range 1~32.
//Return Values:
// 0 : Success.
// -1 : Error.
int bit_set(const int idx, int bit_val, int *p_src_val)
{
	int src_val = *p_src_val, set_val = 0;
	
	if(idx > 32 || idx <= 0){
		LOG("ERROR: Wrong idx (%d) found!\n", idx);
		return -1;
	}
	if(NULL == p_src_val){
		LOG("ERROR: Wrong p_src_val found! p_src_val=null.\n");
		return -1;
	}
	if(bit_val > 1 || bit_val < 0){
		LOG("ERROR: Wrong set_val (%d) found. Now, change set_val to 1.\n", bit_val);
		bit_val = 1;
	}
	
	if(1 == bit_val){
		set_val = bit_val << (idx-1);
		src_val |= set_val;
	}else{
		set_val = 1 << (idx-1);
		set_val = ~set_val;
		src_val &= set_val;
	}
	*p_src_val = src_val;
	
	return 0;
}

//Parameter:
//	offset : The offset from the beginning of file.
//Return Values:
//	 0  : Success
//	-1 : Internal error.
int write_file(const char *path, unsigned int offset, char *data)
{
	int fd = -1, lseek_result = -1, written_bytes = -1;
	
	if(NULL == path){
		LOG("ERROR: Wrong param input. path=null.\n");
		return -1;
	}
	if(NULL == data){
		LOG("ERROR: Wrong param input. data=null.\n");
		return -1;
	}
	
	fd = open(path, O_CREAT|O_WRONLY);
	if(fd < 0){
		LOG("ERROR: open() failed. errno=%d.\n", errno);
		return -1;
	}
	
	lseek_result = lseek(fd, offset, SEEK_SET);
	if(lseek_result < 0){
		//The errno EINVAL represents that offset is beyond the end of file.
		LOG("ERROR: lseek() failed. errno=%d.\n", errno);
		return -1;
	}
	
	__WRITE_FILE_WRITE_AGAIN__:
	do{
		written_bytes = write(fd, data, strlen(data));
	}while(written_bytes == 0);
	if(written_bytes < 0){
		if(errno == EAGAIN || errno == EINTR)
			goto __WRITE_FILE_WRITE_AGAIN__;
		
		LOG("ERROR: write() failed. errno=%d.\n", errno);
		return -1;
	}
	
	close(fd);
	return 0;
}

Thread_Idx_E_Type get_thread_idx(void)
{
	int i;
	
	for( i=0; i<(int)THD_IDX_MAX; i++){
		if(pthread_equal(thread_id_array[i], pthread_self())){
			break;
		}
	}
	
	return (Thread_Idx_E_Type)i;
}


#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

int  IsLeapYear(int n)
{
        if((((n%4)==0)&&(n%100)!=0)||(n%400==0))
            return 1;
        else 
            return 0;
}



void UTCDate(rsp_cclk_time_type *strSetTime,  char unTimeZone)
{       
		
		unsigned char ucaDays[]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		
		LOG("Debug: strSetTime->year =%d.\n", strSetTime->year);
		LOG("Debug: strSetTime->month =%d.\n", strSetTime->month);
		LOG("Debug: strSetTime->mday =%d.\n", strSetTime->mday);
		LOG("Debug: strSetTime->hour =%d.\n", strSetTime->hour);
		LOG("Debug: strSetTime->min =%d.\n", strSetTime->min);
		LOG("Debug: strSetTime->sec =%d.\n", strSetTime->sec);

		LOG("Debug: unTimeZone =%d.\n", unTimeZone);

		if ( unTimeZone > 0)
		{
			strSetTime->hour += unTimeZone;
			if (strSetTime->hour > 23)
			{
				strSetTime->hour = strSetTime->hour - 24;
				strSetTime->mday++;

				
				if (IsLeapYear(strSetTime->year))
				{
					ucaDays[1] = 29;
				}
				else
				{
					ucaDays[1] = 28;
				}

				if (strSetTime->mday > ucaDays[strSetTime->month - 1])
				{
					strSetTime->mday = 1;

					strSetTime->month++;

					if (strSetTime->month > 12)
					{
					       strSetTime->month=1;
						strSetTime->year++;
					}
				}
			}

		}
		else if (unTimeZone < 0)
		{
			int iHour;

			iHour = strSetTime->hour;
			iHour += unTimeZone;

			if (iHour < 0)
			{
				strSetTime->hour = 24 + iHour;

				if (strSetTime->mday == 1)
				{
					if (strSetTime->month == 1)
					{
						strSetTime->year--;
						strSetTime->month = 12;
						strSetTime->mday = 31;
					}
					else
					{
						if (strSetTime->month == 3)
						{
							strSetTime->month = 2;

							
							if (IsLeapYear(strSetTime->year))
							{
								strSetTime->mday = 29;
							}
							else
							{
								strSetTime->mday = 28;
							}
						}
						else
						{
							strSetTime->month--;

							strSetTime->mday = ucaDays[strSetTime->month];
						}
					}
				}
				else
				{
					strSetTime->mday--;
				}
			}
			else
			{
				strSetTime->hour = iHour;
			}
		}


		LOG("Debug: strSetTime->year =%d.\n", strSetTime->year);
		LOG("Debug: strSetTime->month =%d.\n", strSetTime->month);
		LOG("Debug: strSetTime->mday =%d.\n", strSetTime->mday);
		LOG("Debug: strSetTime->hour =%d.\n", strSetTime->hour);
		LOG("Debug: strSetTime->min =%d.\n", strSetTime->min);
		LOG("Debug: strSetTime->sec =%d.\n", strSetTime->sec);

}

#endif

