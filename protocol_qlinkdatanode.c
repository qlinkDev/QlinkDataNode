#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>

#include <unistd.h>
#include <time.h>

#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

#include "protocol_qlinkdatanode.h"
#include "common_qlinkdatanode.h"
#include "feature_macro_qlinkdatanode.h"
#include "socket_qlinkdatanode.h"

#ifdef FEATURE_ENABLE_PRIVATE_PROTOCOL


/******************************External Functions*******************************/
extern void msq_send_qlinkdatanode_run_stat_ind(int data);
extern rsmp_protocol_type conv_flow_state_to_req_type(rsmp_state_s_type state);
extern int WriteFluValueToFile(long long ttvalue,unsigned char ucfile_mode);
extern int ReadFluValueFromFile(strflu_record *pflu_data,unsigned char ucfile_mode);
extern void notify_Sender(char opt);
extern int WriteAPNMessageToFile(char * apnBuf);


#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

	extern int WriteUserTrafficToFile(long long ttvalue);
	extern int ReadUserTrafficFromFile(strflu_record *pflu_data);
	extern int WriteEndDateFromServerToFile(unsigned int  ttvalue);
	extern int WriteModeToFile(long long  ttvalue);
	extern int ReadModeFromFile(unsigned int * unPRunMode);


#endif

/******************************External Variables*******************************/
extern Net_Info rsmp_net_info;
#ifdef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
extern Time_Zone_S timezone_qlinkdatanode;
#endif
extern bool g_qlinkdatanode_run_status_03;
extern rsmp_control_block_type flow_info;
extern Dev_Info rsmp_dev_info;

#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER
extern Flu_limit_control flucontrol_server;
#endif
extern int qlinkdatanode_run_stat;
extern unsigned char g_qlinkdatanode_run_status_01;
extern unsigned char g_qlinkdatanode_run_status_02;
extern bool g_disable_send_downloadSimcard_req;
extern unsigned char apnMessageBuff[200];

#ifdef FEATURE_ENABLE_EmodemUDP_LINK

extern bool g_EmodemUdp_flag;
extern unsigned int tcp_server_port;
extern char tcp_IP_addr[16];

extern unsigned int tcp_server_port_2;
extern char tcp_IP_addr_2[16];

extern unsigned int last_tcp_server_port1;
extern char last_tcp_IP_addr1[16];

extern unsigned int last_tcp_server_port2;
extern char last_tcp_IP_addr2[16];


extern bool g_enable_EmodemTCP_link;
extern unsigned char g_enable_TCP_IP01_linkcount;
extern unsigned char g_enable_TCP_IP02_linkcount;
extern unsigned char g_record_ID_timeout_conut ;


#endif

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206

	extern trafficFromServercontrol trafficDataServer;
	extern bool isBookMode;
	extern bool isCancelRecoveryNet;

#endif

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG


extern char  c_softwarecard_auth_buf[80];
extern unsigned int  c_softwarecard_auth_len;
extern bool b_enable_softwarecard_ok;

#endif

#ifdef  FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG


extern unsigned char g_SoftcardFlowState;
extern char keybuf[8];
extern unsigned int  g_randGetkey;
extern char keybuf_des[8];
extern char server_des_ki[16];
extern char server_des_opc[16];
//extern char server_des_ki_backup[16];



#endif

extern unsigned int tcp_server_port_update;
extern char tcp_IP_addr_update[16];


#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

extern rsp_cclk_time_type rsp_Emodemcclk_ime;

#endif

#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

extern bool isTestMode;
extern bool isPdpActive;

#endif

/******************************Local Variables*******************************/
#ifdef FEATURE_ENABLE_MSGQ
static bool isIdResultNotified = false;
static char pre_ID_result = 0xff;
#endif // FEATURE_ENABLE_MSGQ

//static bool isIdResult01 = false;
long long  changesimcard_netfluValue = 0;


char LAST_Imodem_CELLID[4] = {0x0,0x0,0x0,0x0}; //
 
/******************************Global Variables*******************************/
//Function:
//When calling add_node(), the flow_state of the time need to be saved into this variable.
//And before calling dequeue_and_send_req(), it needs to update flow_info.flow_state with this variable.
rsmp_state_s_type last_flow_state = FLOW_STATE_IDLE;

//Parameter:
//len: The length of memory which need to compute check byte.
char byte_xor_checkout(char *mem, const int len)
{
  char result;
  int cnt;
  int i = 1;

  if (len == 69){
 
	LOG5("**********byte_xor_checkout**********\n");
	for(; i<=70; i++){
		if(i%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(mem, i-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(mem, i-1) );
		
		if(i%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i-1)%20 != 0) // Avoid print redundant line break
		LOG3("\n");
	LOG4("***********byte_xor_checkout***************\n");
  }
  
  result = mem[0]^mem[1];
  for( cnt=2; cnt<len; cnt++){
    result ^= mem[cnt];
  }

  return result;
}

//parameter
//The pointer which pointed to wrapped data by private protocol.
void switch_endian(unsigned char *mem, const int length)
{
  int switch_time = length/2;
  int cnt = 0;
  int r_cnt = length-1;
  unsigned char tmp;

  for(; cnt<switch_time; cnt++, r_cnt--){
    tmp = *(mem+cnt);
    *(mem+cnt) = *(mem+r_cnt);
    *(mem+r_cnt) = tmp;
  }

  return;
}

//Description:
//	If input param "state" equals:
//		FLOW_STATE_ID_AUTH
//		FLOW_STATE_REMOTE_UIM_DATA_ERQ
//		FLOW_STATE_APDU_COMM
//		FLOW_STATE_STAT_UPLOAD
//		FLOW_STATE_PWR_DOWN_UPLOAD
//	This function will update flow_info.flow_state and flow_info.req_type.
//Return Values:
//	  1 : Success
//	-1 : Wrong param input.
//Modication Record:
//	2015-8-11: Create it. Move the update of flow_info.req_type from proto_encode_raw_data() to this function.
int proto_update_flow_state(rsmp_state_s_type state)
{
	if(state >= FLOW_STATE_MAX || state < FLOW_STATE_IDLE){
		LOG("ERROR: Wrong state (%d) found!\n", state);
		return -1;
	}
	
	//Update flow_info.req_type
	switch(state){
		case FLOW_STATE_ID_AUTH:
		case FLOW_STATE_REMOTE_UIM_DATA_REQ:
		case FLOW_STATE_APDU_COMM:
		case FLOW_STATE_STAT_UPLOAD:
		case FLOW_STATE_PWR_DOWN_UPLOAD:
			flow_info.req_type = conv_flow_state_to_req_type(state);break;
		default:
			break;
	}
	//Update flow_info.flow_state
	flow_info.flow_state = state;
	
	return 1;
}

//Return Values:
//	  1 : Success
//	-1 : Wrong param input.
//Modication Record:
//	2015-8-11: Create it.
int proto_update_flow_state_ext(rsmp_control_block_type *fi, rsmp_state_s_type state)
{
	if(fi == NULL){
		LOG("ERROR: Wrong fi found. fi=null.\n");
		return -1;
	}
	if(state >= FLOW_STATE_MAX || state < FLOW_STATE_IDLE){
		LOG("ERROR: Wrong state (%d) found!\n", state);
		return -1;
	}
	
	//Update flow_info.req_type
	switch(state){
		case FLOW_STATE_ID_AUTH:
		case FLOW_STATE_REMOTE_UIM_DATA_REQ:
		case FLOW_STATE_APDU_COMM:
		case FLOW_STATE_STAT_UPLOAD:
		case FLOW_STATE_PWR_DOWN_UPLOAD:
			fi->req_type = conv_flow_state_to_req_type(state);break;
		default:
			break;
	}
	//Update flow_info.flow_state
	fi->flow_state = state;
	
	return 1;
}

void int2char(char *cbuf ,unsigned int ivalue)
{
	char * ctempbuf;
	int i;

	ctempbuf = cbuf;

	*ctempbuf = ivalue >> 24;
	*(ctempbuf+1)= ivalue >> 16;
	*(ctempbuf+2)= ivalue >> 8 ;
	*(ctempbuf+3)= ivalue;

}
//Param:
//	option=0: read rx bytes.
//	option=1: read tx bytes.
//Return Values:
//	>0: The num of rx/tx bytes.
//	-1: Wrong param input.
//	-2: open() failed.
//	-3: read() failed.
static long long int Getnet_traffic_info(int option) 
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


/**********************************************************************
* SYMBOL	: yy_system_call(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/04/18
**********************************************************************/
void yy_system_call_d(const char *command, char *data)
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
}

/**********************************************************************
* SYMBOL	: Get_Devices_number(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/04/18
**********************************************************************/

int Get_Devices_number(void)
{
	char buffer[512];
	char *p = NULL;
	int cli = 0;
	memset(buffer,0,512);

	yy_system_call_d("hostapd_cli all_sta | grep \"sta_num=\"",buffer);
	p = strstr(buffer,"sta_num=");

	if (p != NULL)
	{
		
		p += strlen("sta_num=");
		cli = atoi(p);
		if (cli > 10)
		{
			cli = 10;
		}
		LOG("DEBUG: cli =%d.\n", cli);
	}
	else
	{
		LOG("DEBUG: cli =0 =%d.\n", cli);
		cli = 0;
	}

	return cli;
}

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG

void proto_encode_software_data(void *data, int *len)
{
    char msg_head = 0xaa;
    unsigned int msg_len;
    char *pck_ptr = NULL;
   
    memset (c_softwarecard_auth_buf, 0, sizeof(c_softwarecard_auth_buf));
    msg_len = 65;
    unsigned short APDU_len = *len;
    //msg_len = 10+*len; // packet length without msg_head and msg_len
    (*len) += 15; // packet length
     c_softwarecard_auth_len = (*len);
    LOG("DEBUG: c_softwarecard_auth_len = %d \n", c_softwarecard_auth_len);
    
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);
    pck_ptr[0] = msg_head;
    *((unsigned int *)(pck_ptr+1)) = msg_len;
    pck_ptr[5] = REQ_TYPE_APDU;
    pck_ptr[6] = DATA_TYPE_UEID;
    memcpy(pck_ptr+7, flow_info.UEID, 4);
    pck_ptr[11] = DATA_TYPE_APDU;
    memcpy((void *)(pck_ptr+14), data, APDU_len);
    switch_endian((unsigned char *)(&APDU_len), 2);
    *((unsigned short *)(pck_ptr+12)) = APDU_len;

    *len -= 1;
    LOG("Debug: *len = %d \n",  *len);
 
    msg_len = ( (*len+1) - 5);
    LOG("Debug: msg_len = 0x%02x \n",  msg_len);
    switch_endian((unsigned char *)(&msg_len), 4);
    *((unsigned int *)(pck_ptr+1)) = msg_len;

    pck_ptr[*len] = (char)  byte_xor_checkout(pck_ptr, *len);
    LOG("DEBUG: pck_ptr[*len] = %d \n", (pck_ptr[*len]));

	
    memcpy (c_softwarecard_auth_buf, pck_ptr, (*len+1));

    LOG("DEBUG: c_softwarecard_auth_len = %d \n", c_softwarecard_auth_len);
    LOG("DEBUG: (*len+1) = %d \n", (*len+1));
    return;

}

#endif


//Return value:
// The pointer which pointed to wrapped data by private protocol.
//Parameter
// *len: It is a parameter indicating the length of arg "data", and its return value indicating
// the length of data which is pointed by returned pointer. 
//Note:
// It will allocate a dynamic memory inside.
//Modification Record:
// 2015-8-11: Remove the arg "fi". Replace the input param arg "fi" with global var flow_info.
void *proto_encode_raw_data(void *data, int *len, rsmp_protocol_type type, Dev_Info *sim5360_di, Dev_Info *rsmp_di)
{
  char msg_head = 0xaa;
  unsigned int msg_len;
  char *pck_ptr = NULL;
  long long current_simcardfluvalue;
  long long   tempsimCradFludata;
  long long   reportsimCradFludata;
  strflu_record  pre_simCradFludata;
  unsigned int simcardbuf[2];

  char  devices_num = 0;

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
	int i = 0;
 	strflu_record userDataTraffic;
  	unsigned int userDataTrafficdbuf[2];

	 long long templong = 0;
	
#endif


  switch(type)
  {

  

  case REQ_TYPE_Emodem_UDP:

	msg_len = 24; // packet length without msg_head and msg_len
	*len = msg_len+5; // packet length
	 pck_ptr = (char *)malloc(*len);
    	memset(pck_ptr, 0, *len);
    	switch_endian((unsigned char *)(&msg_len), 4);
	 pck_ptr[0] = msg_head;
    	*((unsigned int *)(pck_ptr+1)) = msg_len;
    	pck_ptr[5] = REQ_TYPE_Emodem_UDP;
   	pck_ptr[6] = DATA_TYPE_IMEI;
    	memcpy(pck_ptr+7, sim5360_di->IMEI, 15);
    	pck_ptr[22] = DATA_TYPE_GCI;
    	memcpy(pck_ptr+23, sim5360_di->PLMN, 3);
    	memcpy(pck_ptr+26, sim5360_di->LAC, 2);
    	pck_ptr[28] = byte_xor_checkout(pck_ptr, 28);
	
	break;
	
#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

       case REQ_TYPE_SOFT_KEY:

	msg_len = 16; // packet length without msg_head and msg_len
	*len = msg_len+5; // packet length
       pck_ptr = (char *)malloc(*len);
    	memset(pck_ptr, 0, *len);
    	switch_endian((unsigned char *)(&msg_len), 4);
	pck_ptr[0] = msg_head;
    	*((unsigned int *)(pck_ptr+1)) = msg_len;
    	pck_ptr[5] = REQ_TYPE_SOFT_KEY;
	pck_ptr[6] = DATA_TYPE_UEID;
    	memcpy(pck_ptr+7, flow_info.UEID, 4);
	pck_ptr[11] = 0x30;

	memset(keybuf,0,sizeof(keybuf));
	g_randGetkey = (unsigned int) GenerateKeyUeVlaue(keybuf);
	LOG("Debug:  g_randGetkey = %d.\n", g_randGetkey);

	memcpy(pck_ptr+12, keybuf, sizeof(keybuf));
	pck_ptr[20] = byte_xor_checkout(pck_ptr, 20);


	break;

#endif


#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

       case REQ_TYPE_SOFT_KIOPC:

	msg_len = 7; // packet length without msg_head and msg_len
	*len = msg_len+5; // packet length
       pck_ptr = (char *)malloc(*len);
    	memset(pck_ptr, 0, *len);
    	switch_endian((unsigned char *)(&msg_len), 4);
	pck_ptr[0] = msg_head;
    	*((unsigned int *)(pck_ptr+1)) = msg_len;
    	pck_ptr[5] = REQ_TYPE_SOFT_KIOPC;
	pck_ptr[6] = DATA_TYPE_UEID;
    	memcpy(pck_ptr+7, flow_info.UEID, 4);
	pck_ptr[11] = byte_xor_checkout(pck_ptr, 11);
	


	break;

#endif


	
  case REQ_TYPE_ID:
  {
    if(sim5360_di == NULL)
    {
      return NULL;
    }
	
    msg_len = 34; // packet length without msg_head and msg_len
    *len = msg_len+5; // packet length
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);
    switch_endian((unsigned char *)(&msg_len), 4);

    pck_ptr[0] = msg_head;
    *((unsigned int *)(pck_ptr+1)) = msg_len;
    pck_ptr[5] = REQ_TYPE_ID;
    pck_ptr[6] = DATA_TYPE_IMEI;
    memcpy(pck_ptr+7, sim5360_di->IMEI, 15);
    pck_ptr[22] = DATA_TYPE_GCI;
    memcpy(pck_ptr+23, sim5360_di->PLMN, 3);
    memcpy(pck_ptr+26, sim5360_di->LAC, 2);
    pck_ptr[28] = DATA_TYPE_IMSI;
    memcpy(pck_ptr+29, rsmp_dev_info.IMSI, 9);
    pck_ptr[38] = byte_xor_checkout(pck_ptr, 38);
	
    break;
  }

  case REQ_TYPE_APDU:
  {
    unsigned short APDU_len = *len;
    msg_len = 10+*len; // packet length without msg_head and msg_len
    (*len) += 15; // packet length
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);

    pck_ptr[0] = msg_head;
    pck_ptr[5] = REQ_TYPE_APDU;
    pck_ptr[6] = DATA_TYPE_UEID;
    memcpy(pck_ptr+7, flow_info.UEID, 4);
    pck_ptr[11] = DATA_TYPE_APDU;
    memcpy((void *)(pck_ptr+14), data, APDU_len);
    switch_endian((unsigned char *)(&APDU_len), 2);
    *((unsigned short *)(pck_ptr+12)) = APDU_len;
    pck_ptr[*len-1] = byte_xor_checkout(pck_ptr, *len-1);

    switch_endian((unsigned char *)(&msg_len), 4);
    *((unsigned int *)(pck_ptr+1)) = msg_len;

    break;
  }
    
  case REQ_TYPE_STATUS:
  {
	
    if(rsmp_di == NULL || sim5360_di == NULL)
    {
      return NULL;
    }

    msg_len = 36 + 5+5+6+5+2+2+10+2+10; //xiaobin +5 for 0x23 ext | 0x24 5 | 0x25 6 | 0x26 5| 0x27 2|0x28 2|0x29+9
    *len = msg_len+5;
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);
    switch_endian((unsigned char *)(&msg_len), 4);

    pck_ptr[0] = msg_head;
    *((unsigned int *)(pck_ptr+1)) = msg_len;
    pck_ptr[5] = REQ_TYPE_STATUS;
    pck_ptr[6] = DATA_TYPE_UEID;
    memcpy(pck_ptr+7, flow_info.UEID, 4);
    pck_ptr[11] = DATA_TYPE_GCI;
    memcpy(pck_ptr+12, rsmp_di->PLMN, 3);
    memcpy(pck_ptr+15, rsmp_di->LAC, 2);
    pck_ptr[17] = DATA_TYPE_FLUX;
    pck_ptr[18] = DATA_LEN_FLUX;
    memcpy(pck_ptr+19, (unsigned char *)(rsmp_di->flux), DATA_LEN_FLUX);
    switch_endian((unsigned char *)pck_ptr+19, 8);
    pck_ptr[27] = DATA_TYPE_POWER;
    pck_ptr[28] = DATA_LEN_POWER;
    pck_ptr[29] = rsmp_di->power_lvl;
    pck_ptr[30] = DATA_TYPE_SIG;
    pck_ptr[31] = DATA_LEN_SIG;
    pck_ptr[32] = rsmp_di->sig_strength;
    
    pck_ptr[33] = DATA_TYPE_REG;
    pck_ptr[34] = DATA_LEN_REG;
    pck_ptr[35] = rsmp_net_info.basic_val;
	
#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

					LOG("Debug :   isTestMode = %d.\n", isTestMode);
					LOG("Debug :   isPdpActive = %d.\n", isPdpActive);

					if ((isTestMode) && ( isPdpActive)){

						pck_ptr[35] = 0x0E;

					}

#endif

    pck_ptr[36] = rsmp_net_info.cpin_val;
    pck_ptr[37] = rsmp_net_info.cnsmod_val;
    pck_ptr[38] = rsmp_net_info.cgreg_val;
    pck_ptr[39] = rsmp_net_info.cereg_val;
    pck_ptr[40] = rsmp_net_info.cregrej_val;//add
    pck_ptr[41] = 0x0;
    pck_ptr[42] = 0x0;
    pck_ptr[43] = 0x0;
    pck_ptr[44] = 0x0;

    //xiaobin add start
    pck_ptr[45] = 0x24; // 9615 cell id
    if(rsmp_di->CELLID[3] == 0 && rsmp_di->CELLID[2] == 0 && rsmp_di->CELLID[1] == 0 && rsmp_di->CELLID[0] == 0)
    {
      memcpy(pck_ptr+46, LAST_Imodem_CELLID, 4);
    }else
    {
      memcpy(pck_ptr+46, rsmp_di->CELLID, 4);
    }

    pck_ptr[50] = 0x25; // Emodem gci(plmn/lac)
    memcpy(pck_ptr+51, sim5360_di->PLMN, 3);
    memcpy(pck_ptr+54, sim5360_di->LAC, 2);

    pck_ptr[56] = 0x26; // Emodem cell id
    memcpy(pck_ptr+57, sim5360_di->CELLID, 4);
   
    pck_ptr[61] = 0x27; // Emodem csq
    pck_ptr[62] = sim5360_di->sig_strength;
    
    pck_ptr[63] = 0x28; // Emodem reg info in screen
    pck_ptr[64] = qlinkdatanode_run_stat & 0xf0;
    //xiaobin add end

   //add by jack 2016/03/30.
   //Only the ID check  return result is 01,get value from eth0.
   // LOG("DEBUG: isIdResult01 =%d.\n", isIdResult01);
    pck_ptr[65] = 0x29; // SIM card flu value.
    pck_ptr[66] = 0x08; 

    LOG("DEBUG: rsmp_di->flux[0] =%u.\n", rsmp_di->flux[0]);
    LOG("DEBUG: rsmp_di->flux[1] =%u.\n", rsmp_di->flux[1]);
    LOG("changesimcard_netfluValue =%lld.\n", changesimcard_netfluValue);


	

	if (ReadFluValueFromFile(&pre_simCradFludata,FILE_TYPE_SIMCARDFLUDATA) == STATUS_ERROR)
	{
		LOG("ERROR: ReadFluValueFromFile FILE_TYPE_SIMCARDFLUDATA \n");
		tempsimCradFludata = 0;
		if (WriteFluValueToFile(tempsimCradFludata,FILE_TYPE_SIMCARDFLUDATA) == STATUS_ERROR)
		{
			LOG("ERROR:  WriteFluValueToFile tempsimCradFludata FILE_TYPE_SIMCARDFLUDATA\n");
		}
		ReadFluValueFromFile(&pre_simCradFludata,FILE_TYPE_SIMCARDFLUDATA);
		LOG("pre_simCradFludata.flu_value =%lld.\n", pre_simCradFludata.flu_value);
	}
	LOG("pre_simCradFludata.flu_value =%lld.\n", pre_simCradFludata.flu_value);

	if ((rsmp_di->flux[0]) < changesimcard_netfluValue)
	{
		current_simcardfluvalue = 0;
	}
	else
	{
		current_simcardfluvalue = rsmp_di->flux[0] - changesimcard_netfluValue;
	}

	if (current_simcardfluvalue < pre_simCradFludata.flu_value)
	{
		pre_simCradFludata.flu_value = 0;
	}

	LOG("DEBUG:current_simcardfluvalue =%lld.\n", current_simcardfluvalue);

	reportsimCradFludata = (current_simcardfluvalue - pre_simCradFludata.flu_value);
	
	if (WriteFluValueToFile(current_simcardfluvalue,FILE_TYPE_SIMCARDFLUDATA) == STATUS_ERROR)
	{
			LOG("ERROR:  WriteFluValueToFile current_simcardfluvalue FILE_TYPE_SIMCARDFLUDATA\n");
	}
	LOG("DEBUG: reportsimCradFludata =%lld.\n", reportsimCradFludata);
	  

	simcardbuf[0] = (unsigned int)(reportsimCradFludata & 0x00000000ffffffff);
	simcardbuf[1] = (unsigned int)(reportsimCradFludata >> 32 & 0x00000000ffffffff);

	LOG("DEBUG: simcardbuf[0] =%u.\n", simcardbuf[0]);
	LOG("DEBUG: simcardbuf[1] =%u.\n", simcardbuf[1]);
	
	memset(pck_ptr+67,0,8);
	int2char(pck_ptr+67,simcardbuf[1]);
	int2char(pck_ptr+71,simcardbuf[0]);

	pck_ptr[75] = 0x30; // devices number.
	devices_num = Get_Devices_number();
	pck_ptr[76] = devices_num; // devices number.

	pck_ptr[77] = 0x31; //
	pck_ptr[78] = 0x08; //
	
#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
	
	ReadUserTrafficFromFile(&userDataTraffic);

	LOG("Debug:  userDataTraffic.flu_value = : %llu .\n", userDataTraffic.flu_value);

	
	userDataTrafficdbuf[0] = (unsigned int)(userDataTraffic.flu_value & 0x00000000ffffffff);
	userDataTrafficdbuf[1] = (unsigned int)(userDataTraffic.flu_value >> 32 & 0x00000000ffffffff);

	LOG("DEBUG: userDataTrafficdbuf[0] =%u.\n", userDataTrafficdbuf[0]);
	LOG("DEBUG: userDataTrafficdbuf[1] =%u.\n", userDataTrafficdbuf[1]);
	
	memset(pck_ptr+79,0,8);
	int2char(pck_ptr+79,userDataTrafficdbuf[1]);
	int2char(pck_ptr+83,userDataTrafficdbuf[0]);

	LOG2("TTTTTTTTTTTTTTTTTTT*************TTTTTTTTTTTTTTTTTTTTTTTTT: \n");
	i = 1;					
	for(; i<=8; i++){
		if(i%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(&pck_ptr[79], i-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(&pck_ptr[79], i-1) );
												
		if(i%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i-1)%20 != 0) // Avoid print redundant line break
	LOG3("\n");

	LOG("DEBUG: userDataTrafficdbuf[0] =%d.\n", userDataTrafficdbuf[0]);
#else

	memset(pck_ptr+79,0,8);
      

#endif

	 pck_ptr[87] = byte_xor_checkout(pck_ptr, *len-1);
  
    break;
  }
  
  case REQ_TYPE_REMOTE_UIM_DATA:
  {
    msg_len = 7; // packet length without msg_head and msg_len
    *len = msg_len+5; // packet length
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);
    switch_endian((unsigned char *)(&msg_len), 4);

    pck_ptr[0] = msg_head;
    *((unsigned int *)(pck_ptr+1)) = msg_len;
    pck_ptr[5] = REQ_TYPE_REMOTE_UIM_DATA;
    pck_ptr[6] = DATA_TYPE_UEID;
    memcpy(pck_ptr+7, flow_info.UEID, 4);
    pck_ptr[11] = byte_xor_checkout(pck_ptr, 11);
    break;
  }

  case REQ_TYPE_PWR_DOWN:
  {
    msg_len = 9; // packet length without msg_head and msg_len
    *len = msg_len+5; // packet length
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);
    switch_endian((unsigned char *)(&msg_len), 4);

    pck_ptr[0] = msg_head;
    *((unsigned int *)(pck_ptr+1)) = msg_len;
    pck_ptr[5] = REQ_TYPE_PWR_DOWN;
    pck_ptr[6] = DATA_TYPE_UEID;
    memcpy(pck_ptr+7, flow_info.UEID, 4);
	
#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810

    pck_ptr[11] = 0x23;

    LOG("DEBUG: g_qlinkdatanode_run_status_01 =0x%02x.\n", g_qlinkdatanode_run_status_01);
    LOG("DEBUG: g_qlinkdatanode_run_status_02 =0x%02x.\n", g_qlinkdatanode_run_status_02);
    LOG("DEBUG: rsmp_net_info.basic_val = 0x%02x.\n", rsmp_net_info.basic_val);

    if( (g_qlinkdatanode_run_status_01 > 0) && (g_qlinkdatanode_run_status_01 < CPIN_READY_OK))
    {
	 pck_ptr[12] = 0x07; // F1 ,F2
	 g_disable_send_downloadSimcard_req = true;
	 LOG("DEBUG: g_disable_send_downloadSimcard_req =0x%02x.\n", g_disable_send_downloadSimcard_req);
	 LOG("DEBUG: Delete simcard data before power off.\n");
        notify_Sender(0x05);
    }
    else if ((g_qlinkdatanode_run_status_01 >= CPIN_READY_OK) && (g_qlinkdatanode_run_status_02 == REGIST_NET_ERROR))
    {
	pck_ptr[12] = 0x08; // F7 OPEN WEB FAIL
	LOG("DEBUG08: rsmp_net_info.basic_val =0x%02x.\n", rsmp_net_info.basic_val);
	LOG("DEBUG08: g_qlinkdatanode_run_status_03 =0x%02x.\n", g_qlinkdatanode_run_status_03);
	if (g_qlinkdatanode_run_status_03){

		pck_ptr[12] = 0x0C; // 

	}
	if (rsmp_net_info.basic_val == 0x04) // 20160829 add by jackli
	{
		pck_ptr[12] = rsmp_net_info.basic_val;
	}
    }
    else if ((g_qlinkdatanode_run_status_01 >= CPIN_READY_OK_AFTER) && (g_qlinkdatanode_run_status_02 == REGIST_NET_OK))
    {
	//pck_ptr[12] = 0x0A; // F7 NET FAIL
	if( rsmp_net_info.basic_val == 0x02)
	{
		pck_ptr[12] = 0x0A; // F7 NET FAIL
	}
	else 
	{
		pck_ptr[12] = rsmp_net_info.basic_val;
	}
    }
    else if ((g_qlinkdatanode_run_status_01 == 0xff) || (g_qlinkdatanode_run_status_02 == 0xff))
    {
	pck_ptr[12] = 0x0B; // factory reset
    }
    else if ((g_qlinkdatanode_run_status_01 == CPIN_READY_OK) && (g_qlinkdatanode_run_status_02 == REGIST_NET_OK))
    {// 20160829 add by jackli
	
	LOG("DEBUG01: rsmp_net_info.basic_val =0x%02x.\n", rsmp_net_info.basic_val);
	if (rsmp_net_info.basic_val == 0x01)
	{
		pck_ptr[12] = 0x04;
	}
    }
    else
    {
	pck_ptr[12] = rsmp_net_info.basic_val;
    }

#endif
    pck_ptr[13] = byte_xor_checkout(pck_ptr, 13);
    break;
  }
  
  case REQ_TYPE_REG:
  {
    msg_len = 7; // packet length without msg_head and msg_len
    *len = msg_len+5; // packet length
    pck_ptr = (char *)malloc(*len);
    memset(pck_ptr, 0, *len);
    switch_endian((unsigned char *)(&msg_len), 4);

    pck_ptr[0] = msg_head;
    *((unsigned int *)(pck_ptr+1)) = msg_len;
    pck_ptr[5] = REQ_TYPE_REG;
    pck_ptr[6] = DATA_TYPE_UEID;
    memcpy(pck_ptr+7, flow_info.UEID, 4);
    pck_ptr[11] = byte_xor_checkout(pck_ptr, 11);
    break;
  }
  default:
    break;
  }

  return (void *)pck_ptr;
}

/**********************************************************************
* SYMBOL	: SaveAPN4GFile_Opition
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/26
**********************************************************************/

int SaveAPN4GFile_Opition(void)
{
	int status;
	

       status  = system("cd /var/ && cp apns-conf-4G.xml apns-conf-4G_old.xml && chmod 777 apns-conf-4G_old.xml");

       if (-1 == status)  
       {  
            LOG("ERROR:system call  apns-conf-4G_old.xml file command  error!\n");  
	     return STATUS_ERROR;
       }  
       else  
       {  
            LOG("system call apns-conf-4G_old.xml file command exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                    LOG("system call  apns-conf-4G_old.xml file command successfully.\n");  
		      return STATUS_OK;
                }  
                else  
                {
                    LOG("ERROR: system call apns-conf-4G_old.xml file command script exit code: %d\n", WEXITSTATUS(status)); 
		       return STATUS_ERROR;
                }  
            }  
            else  
            {  
                LOG("system call qlinkdatanode file command exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
}


/**********************************************************************
* SYMBOL	: SaveAPN3GFile_Opition
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/26
**********************************************************************/

int SaveAPN3GFile_Opition(void)
{
	int status;
	

       status  = system("cd /var/ && cp apns-conf-3G.xml apns-conf-3G_old.xml && chmod 777 apns-conf-3G_old.xml");

       if (-1 == status)  
       {  
            LOG("ERROR:system call  apns-conf-3G_old.xml file command  error!\n");  
	     return STATUS_ERROR;
       }  
       else  
       {  
            LOG("system call apns-conf-3G_old.xml file command exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                    LOG("system call  apns-conf-3G_old.xml file command successfully.\n");  
		      return STATUS_OK;
                }  
                else  
                {
                    LOG("ERROR: system call apns-conf-3G_old.xml file command script exit code: %d\n", WEXITSTATUS(status)); 
		       return STATUS_ERROR;
                }  
            }  
            else  
            {  
                LOG("system call qlinkdatanode file command exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
}


/**********************************************************************
* SYMBOL	: DeleteAPN4GFile_Opition
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/26
**********************************************************************/

int DeleteAPN4GFile_Opition(void)
{
	int status;
	

       status  = system("cd /var/ && rm -r apns-conf-4G.xml");

       if (-1 == status)  
       {  
            LOG("ERROR:system call  apns-conf-4G.xml file command  error!\n");  
	     return STATUS_ERROR;
       }  
       else  
       {  
            LOG("system call apns-conf-4G.xml file command exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                    LOG("system call  apns-conf-4G.xml file command successfully.\n");  
		      return STATUS_OK;
                }  
                else  
                {
                    LOG("ERROR: system call apns-conf-4G.xml file command script exit code: %d\n", WEXITSTATUS(status)); 
		       return STATUS_ERROR;
                }  
            }  
            else  
            {  
                LOG("system call qlinkdatanode file command exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
}

/**********************************************************************
* SYMBOL	: DeleteAPN3GFile_Opition
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/26
**********************************************************************/

int DeleteAPN3GFile_Opition(void)
{
	int status;
	

       status  = system("cd /var/ && rm -r apns-conf-3G.xml");

       if (-1 == status)  
       {  
            LOG("ERROR:system call  apns-conf-3G.xml file command  error!\n");  
	     return STATUS_ERROR;
       }  
       else  
       {  
            LOG("system call apns-conf-3G.xml file command exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                    LOG("system call  apns-conf-3G.xml file command successfully.\n");  
		      return STATUS_OK;
                }  
                else  
                {
                    LOG("ERROR: system call apns-conf-3G.xml file command script exit code: %d\n", WEXITSTATUS(status)); 
		       return STATUS_ERROR;
                }  
            }  
            else  
            {  
                LOG("system call qlinkdatanode file command exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
}

/**********************************************************************
* SYMBOL	: RecoverAPN4GFile_Opition
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/26
**********************************************************************/

int RecoverAPN4GFile_Opition(void)
{
	int status;
	

	status  = system("cd /var/ && cp apns-conf-4G_old.xml apns-conf-4G.xml && chmod 777 apns-conf-4G.xml");

       if (-1 == status)  
       {  
            LOG("ERROR:system call  apns-conf-4G.xml file command  error!\n");  
	     return STATUS_ERROR;
       }  
       else  
       {  
            LOG("system call apns-conf-4G.xml file command exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                    LOG("system call  apns-conf-4G.xml file command successfully.\n");  
		      return STATUS_OK;
                }  
                else  
                {
                    LOG("ERROR: system call apns-conf-4G.xml file command script exit code: %d\n", WEXITSTATUS(status)); 
		       return STATUS_ERROR;
                }  
            }  
            else  
            {  
                LOG("system call qlinkdatanode file command exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
}

/**********************************************************************
* SYMBOL	: RecoverAPN3GFile_Opition
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/05/26
**********************************************************************/

int RecoverAPN3GFile_Opition(void)
{
	int status;
	

	status  = system("cd /var/ && cp apns-conf-3G_old.xml apns-conf-3G.xml && chmod 777 apns-conf-3G.xml");

       if (-1 == status)  
       {  
            LOG("ERROR:system call  apns-conf-3G.xml file command  error!\n");  
	     return STATUS_ERROR;
       }  
       else  
       {  
            LOG("system call apns-conf-3G.xml file command exit status value = [0x%x]\n", status);  
      
            if (WIFEXITED(status))  
            {  
                if (0 == WEXITSTATUS(status))  
                {  	
                    LOG("system call  apns-conf-3G.xml file command successfully.\n");  
		      return STATUS_OK;
                }  
                else  
                {
                    LOG("ERROR: system call apns-conf-3G.xml file command script exit code: %d\n", WEXITSTATUS(status)); 
		       return STATUS_ERROR;
                }  
            }  
            else  
            {  
                LOG("system call qlinkdatanode file command exit status = [%d]\n", WEXITSTATUS(status));  
            }  
        }  
}

int Creat_New3GAPNFile(void)
{
		
	 system("cd /var/ && cp apns-conf-4G.xml apns-conf-3G.xml");
}

int Creat_New4GAPNFile(void)
{
	 system("cd /var/ && cp apns-conf-4G_temp.xml apns-conf-4G.xml && rm -r apns-conf-4G_temp.xml");
}
#if 1
int Set_APNMessageToFile(char * newmcc,char * newmnc,char * newapn,char * newuser,char * newpassword,char Apnfiletype)
{

	FILE *fp;
	int status;
	char exp[100] = "";
	char *carrier_str = "YY-Mobile";
	char *mcc_str = "mcc";
	char *mnc_str = "mnc";
	char *apn_str = "apn";
	char *user_str = "user";
	char *password_str = "password";
	char *type_str = "type";
	char *frist_ad;
	char *pck_ptr = NULL;
	char *pdata = NULL;
	char *pwritedata = NULL;
	unsigned short num = 0;
	struct stat statbuf;
	int fd_3G = -1;
	int fd_4G = -1;
	int fd_new = -1;
	int ret;
	int strlen_t;
	int strapnlen_t = 0;


	char *apn_new_mcc = NULL;
	char *apn_new_mnc = NULL;
	char *apn_new_strapn =NULL;
	char *apn_new_struser = NULL;
	char *apn_new_strpassword = NULL;
	char *apn_new_strtype= "default,supl";

	
	apn_new_mcc = newmcc;
	apn_new_mnc = newmnc;
	apn_new_strapn = newapn;
	apn_new_struser = newuser;
	apn_new_strpassword = newpassword;

	strapnlen_t = strlen(apn_new_mcc);
	//LOG(" strapnlen_t =  %d.\n", strapnlen_t);
	strapnlen_t += strlen(apn_new_mnc);
	//LOG(" strapnlen_t =  %d.\n", strapnlen_t);
	strapnlen_t += strlen(apn_new_strapn);
	//LOG(" strapnlen_t =  %d.\n", strapnlen_t);
	strapnlen_t += strlen(apn_new_struser);
	//LOG(" strapnlen_t =  %d.\n", strapnlen_t);
	strapnlen_t += strlen(apn_new_strpassword);
	//LOG(" strapnlen_t =  %d.\n", strapnlen_t);
	strapnlen_t += strlen(apn_new_strtype);
	//LOG(" strapnlen_t =  %d.\n", strapnlen_t);

	LOG(" apn_new_mcc =  %s.\n", apn_new_mcc);
	LOG(" apn_new_mnc =  %s.\n", apn_new_mnc);
	LOG(" apn_new_strapn =  %s.\n", apn_new_strapn);
	LOG(" apn_new_struser =  %s.\n", apn_new_struser);
	LOG(" apn_new_strpassword =  %s.\n", apn_new_strpassword);

	

	if (Apnfiletype == 0)
	{
		SaveAPN4GFile_Opition();
		sprintf(exp,"sed '28,$d' /var/apns-conf-4G_old.xml > /var/apns-conf-4G_temp.xml");
       	system(exp);

		sprintf(exp,"sed -i '$a </apns>' /var/apns-conf-4G_temp.xml");
       	system(exp);

		ret = stat("/var/apns-conf-4G_temp.xml", &statbuf);
	}
	else
	{
		SaveAPN3GFile_Opition();
		sprintf(exp,"sed '28,$d' /var/apns-conf-3G_old.xml > /var/apns-conf-3G_temp.xml");
      	 	system(exp);

		sprintf(exp,"sed -i '$a </apns>' /var/apns-conf-3G_temp.xml");
      		system(exp);

		ret = stat("/var/apns-conf-3G_temp.xml", &statbuf);
	}
	
	//LOG("------ret::: %d------------------\r\n",ret);
	//LOG("------statbuf.st_size:::::::::::: %d------------------\r\n",statbuf.st_size);

	pdata = (char*)malloc(statbuf.st_size+strapnlen_t);

	if (pdata == NULL)
	{
		LOG("ERROR:Write_VersionToFile pdata !\r\n");
		if (Apnfiletype == 0)
		{
			RecoverAPN4GFile_Opition();
		}
		else
		{
			RecoverAPN3GFile_Opition();
		}
		
		free(pdata);
		return STATUS_ERROR;
	}

	if (Apnfiletype == 0)
	{
		if ((fd_4G = open("/var/apns-conf-4G_temp.xml",O_RDWR)) == (-1))
		{
		    	printf("ERROR: open() failed. errno=%d.\n", errno);
			close(fd_4G);
			free(pdata);
			RecoverAPN4GFile_Opition();
		    	return STATUS_ERROR;
		}
		else
		{
			LOG(" Open 4G file fd:::: %d------------------\r\n",fd_4G);
		}
	}
	else
	{
		if ((fd_3G = open("/var/apns-conf-3G_temp.xml",O_RDWR)) == (-1))
		{
		    	printf("ERROR: open() failed. errno=%d.\n", errno);
			close(fd_3G);
			free(pdata);
			RecoverAPN3GFile_Opition();
		    	return STATUS_ERROR;
		}
		else
		{
			LOG(" Open 3G file fd:::: %d------------------\r\n",fd_3G);
		}
	}

	if (Apnfiletype == 0)
	{
		status = read(fd_4G,pdata,statbuf.st_size);
	}
	else
	{
		status = read(fd_3G,pdata,statbuf.st_size);
	}

	

	//LOG("------status:::::::::::   %d------------------\r\n",status);

/////////////////////////////////////////Set carrier ///////////////////////////////////////////////////////
	frist_ad = pdata;

	for(num = 0; num < statbuf.st_size;num++)
	{
		if ((*(pdata + num) == 'c' ) && (*(pdata + num+1) == 'a') && (*(pdata + num+2) == 'r') && (*(pdata + num+3) == 'r') && (*(pdata + num+4) == 'i') )
		{
			
			//LOG("Write_VersionToFile:  frist_ad = 0x%02x.\n",  frist_ad );
			//LOG("Write_VersionToFile:  num = 0x%02x.\n",  num );
			frist_ad += num;
			//LOG("Write_VersionToFile:  frist_ad = 0x%02x.\n",  frist_ad );
			
			strlen_t = (strlen("carrier=") + 1);
			
			//LOG("Write_VersionToFile:  strlen_t=0x%02x.\n",  strlen_t);
			 
			//LOG("config new carrier !\r\n");

			while(*carrier_str != '\0')
			{
				*(frist_ad + strlen_t )= *carrier_str;
				strlen_t++;
				carrier_str++;
			}

			*(frist_ad + strlen_t ) = '\"';
			*(frist_ad + strlen_t+1 ) = ' ';
/////////////////////////////////////////config mcc start ///////////////////////////////////////////////////////
			//LOG("config new mcc !\r\n");
			
			while(*mcc_str != '\0')
			{
				*(frist_ad + strlen_t+2 )= *mcc_str;
				strlen_t++;
				mcc_str++;
			}
			*(frist_ad + strlen_t+2 ) = '=';
			*(frist_ad + strlen_t+2+1) = '\"';

			while(*apn_new_mcc != '\0')
			{
				*(frist_ad + strlen_t+2+1+1 ) = *apn_new_mcc;
				strlen_t++;
				apn_new_mcc++;
			}
			*(frist_ad + strlen_t+2+1+1 ) = '\"';
			*(frist_ad + strlen_t+2+1+1+1) = ' ';

/////////////////////////////////////////config mcc end ///////////////////////////////////////////////////////			
/////////////////////////////////////////config mnc start ///////////////////////////////////////////////////////
			//LOG("config new mnc !\r\n");

			while(*mnc_str != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1 ) = *mnc_str;
				strlen_t++;
				mnc_str++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1) = '=';
			*(frist_ad + strlen_t+2+1+1+1+1+1) = '\"';
 
			while(*apn_new_mnc != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1)= *apn_new_mnc;
				strlen_t++;
				apn_new_mnc++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1 ) = '\"';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1) = ' ';

/////////////////////////////////////////config mnc end ///////////////////////////////////////////////////////

/////////////////////////////////////////config apn start ///////////////////////////////////////////////////////
			//LOG("config new apn !\r\n");

			while(*apn_str != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1 ) = *apn_str;
				strlen_t++;
				apn_str++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1 ) = '=';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1 ) = '\"';
 
			while(*apn_new_strapn != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1)= *apn_new_strapn;
				strlen_t++;
				apn_new_strapn++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1 ) = '\"';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1) = ' ';

/////////////////////////////////////////config apn end ///////////////////////////////////////////////////////

/////////////////////////////////////////config user start ///////////////////////////////////////////////////////
			//LOG("config new user !\r\n");

			while(*user_str != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1) = *user_str;
				strlen_t++;
				user_str++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1 ) = '=';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1 ) = '\"';
 
			while(*apn_new_struser != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1)= *apn_new_struser;
				strlen_t++;
				apn_new_struser++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = '\"';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = ' ';

/////////////////////////////////////////config user end ///////////////////////////////////////////////////////

/////////////////////////////////////////config password start ///////////////////////////////////////////////////////
			//LOG("config new password !\r\n");

			while(*password_str != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = *password_str;
				strlen_t++;
				password_str++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = '=';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1 ) = '\"';
 
			while(*apn_new_strpassword != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1)= *apn_new_strpassword;
				strlen_t++;
				apn_new_strpassword++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = '\"';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = ' ';

/////////////////////////////////////////config password end ///////////////////////////////////////////////////////

/////////////////////////////////////////config type start ///////////////////////////////////////////////////////

			//LOG("config new type !\r\n");

			while(*type_str != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = *type_str;
				strlen_t++;
				type_str++;
			}
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = '=';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = '\"';
 
			while(*apn_new_strtype != '\0')
			{
				*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1)= *apn_new_strtype;
				strlen_t++;
				apn_new_strtype++;
			}
			
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) = '\"';
			*(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1) =  ' '; 

			//memset(frist_ad + strlen_t+2+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1,0,strapnlen_t);
			break;
/////////////////////////////////////////config type end ///////////////////////////////////////////////////////

			
		}
	}
	
	
	
	pwritedata = (char*)malloc(statbuf.st_size+strapnlen_t);

	if (pwritedata == NULL)
	{
		LOG("ERROR:Write_VersionToFile pwritedata !\r\n");
		if (fd_3G >= 0)
		{
			fd_3G = -1;
			close(fd_3G);
		}

		if (fd_4G >= 0)
		{
			fd_4G = -1;
			close(fd_4G);
		}
		
		
		free(pdata);
		free(pwritedata);
	
		
		if (Apnfiletype == 0)
		{
			RecoverAPN4GFile_Opition();
		}
		else
		{
			RecoverAPN3GFile_Opition();
		}
		
		return STATUS_ERROR;
	}

	memcpy((char *)pwritedata, (char *)pdata, statbuf.st_size+strapnlen_t);

	if (Apnfiletype == 0)
	{
		if ((fd_new = open("/var/apns-conf-4G_temptemp.xml",O_RDWR|O_CREAT)) == (-1))
		{
		    	printf("ERROR: open() apns-conf-4G_temptemp.xml failed. errno=%d.\n", errno);
			
			if (fd_4G >= 0)
			{
				fd_4G = -1;
				close(fd_4G);
			}
			free(pdata);
			free(pwritedata);
			RecoverAPN4GFile_Opition();
		    	return STATUS_ERROR;
		 }
		else
		{
			LOG("open fd_new:::: %d------------------\r\n",fd_new);
		}
	}
	else
	{
		if ((fd_new = open("/var/apns-conf-3G_temptemp.xml",O_RDWR|O_CREAT)) == (-1))
		{
		    	printf("ERROR: open() apns-conf-3G_temptemp.xml failed. errno=%d.\n", errno);
			if (fd_3G >= 0)
			{
				fd_3G = -1;
				close(fd_3G);
			}

			free(pdata);
			free(pwritedata);
			RecoverAPN3GFile_Opition();
		    	return STATUS_ERROR;
		 }
		else
		{
			LOG("open fd_new:::: %d------------------\r\n",fd_new);
		}
	}
	
	
	
	strlen_t = write(fd_new,pwritedata,statbuf.st_size+strapnlen_t);

	//LOG("Write_VersionToFile  write strlen_t:  %d------------------\r\n",strlen_t);
	
		if (fd_3G >= 0)
		{
			fd_3G = -1;
			close(fd_3G);
		}

		if (fd_4G >= 0)
		{
			fd_4G = -1;
			close(fd_4G);
		}

		if (fd_new >= 0)
		{
			fd_new = -1;
			close(fd_new);
		}
	
	
	if (Apnfiletype == 0)
	{
		sprintf(exp,"sed -i '$a />' /var/apns-conf-4G_temptemp.xml");
      		system(exp);
		sprintf(exp,"sed -i '$a  </apns>' /var/apns-conf-4G_temptemp.xml");
      		system(exp);
		//Creat_New4GAPNFile();

		sprintf(exp,"sed '29,$d' /var/apns-conf-4G_temptemp.xml > /var/apns-conf-4G_finish.xml");
      	 	system(exp);

		system("cd /var/ && cp apns-conf-4G_finish.xml apns-conf-4G.xml && rm -r apns-conf-4G_temp.xml && rm -r apns-conf-4G_temptemp.xml && rm -r apns-conf-4G_finish.xml && rm -r apns-conf-4G_old.xml");
	}
	else
	{
		sprintf(exp,"sed -i '$a />' /var/apns-conf-3G_temptemp.xml");
      		system(exp);
		sprintf(exp,"sed -i '$a  </apns>' /var/apns-conf-3G_temptemp.xml");
      		system(exp);

		sprintf(exp,"sed '29,$d' /var/apns-conf-3G_temptemp.xml > /var/apns-conf-3G_finish.xml");
      	 	system(exp);

		system("cd /var/ && cp apns-conf-3G_finish.xml apns-conf-3G.xml && rm -r apns-conf-3G_temp.xml && rm -r apns-conf-3G_temptemp.xml && rm -r apns-conf-3G_finish.xml && rm -r apns-conf-3G_old.xml");
		
	}

	free(pdata);
	free(pwritedata);
 
	return STATUS_OK;
}

unsigned char Chgbyte(unsigned char chdata)
{
	return (chdata << 4)+(chdata >> 4);
}


void  ChPlmnFormat(unsigned char *pdata,char *ucpmcc,char *ucpmnc)
{
	unsigned char *pPlmn_former;
	unsigned char ucPlmn_last[10];
	unsigned char ucPlmntemp = 0;
	unsigned int tmcc;
	unsigned int tmnc;
	char pmcc[3];
	char pmnc[2];

	char ptemp;

	pPlmn_former = pdata;
	
	/*LOG(":  ucPlmn_former[0]=0x%02x.\n",  pPlmn_former[0]);
	LOG(":  ucPlmn_former[1]=0x%02x.\n",  pPlmn_former[1]);
	LOG(":  ucPlmn_former[2]=0x%02x.\n",  pPlmn_former[2]);*/

	ucPlmntemp = pPlmn_former[0];
	pPlmn_former[0]  = Chgbyte(ucPlmntemp);
	//LOG(":  pPlmn_former[0]=0x%02x.\n",  pPlmn_former[0]);

	ucPlmntemp = pPlmn_former[1];
	ucPlmntemp = ucPlmntemp & 0x0f;
	pPlmn_former[1]  = ucPlmntemp;
	//LOG(":  pPlmn_former[1]=0x%02x.\n",  pPlmn_former[1]);

	ucPlmntemp = pPlmn_former[2];
	pPlmn_former[2]  = Chgbyte(ucPlmntemp);
	//LOG(":  pPlmn_former[2]=0x%02x.\n",  pPlmn_former[2]);

	ucPlmn_last[0] = ((pPlmn_former[0] >> 4) & 0x0f) + '0';
	ucPlmn_last[1] = (pPlmn_former[0] & 0x0f) + '0';
	  
       ucPlmn_last[2] = (pPlmn_former[1] & 0x0f) + '0';
       ucPlmn_last[3] = ((pPlmn_former[2] >> 4) & 0x0f) + '0';
	ucPlmn_last[4] = (pPlmn_former[2] & 0x0f) + '0';

	/*LOG(" ucPlmn_last[0]=0x%02x.\n",  ucPlmn_last[0]);
	LOG(" ucPlmn_last[1]=0x%02x.\n",  ucPlmn_last[1]);
	LOG(" ucPlmn_last[2]=0x%02x.\n",  ucPlmn_last[2]);
	LOG(" ucPlmn_last[3]=0x%02x.\n",  ucPlmn_last[3]);
	LOG(" ucPlmn_last[4]=0x%02x.\n",  ucPlmn_last[4]);*/

	tmcc = Ascii_To_int(&ucPlmn_last[0],3);

	tmnc = Ascii_To_int(&ucPlmn_last[3],2);

	//LOG(" tmcc =%d .\n",  tmcc);
	//LOG(" tmnc =%d .\n",  tmnc);

	sprintf(pmcc,"%d",tmcc);
	sprintf(pmnc,"%d",tmnc);

	//LOG(" 00 mcc --> \"%s\"  \n", pmcc);
	// LOG("00  mnc --> \"%s\"  \n", pmnc);

	if ((ucPlmn_last[3] == 0x30) && (ucPlmn_last[4] != 0x30))
	{
		 ptemp = pmnc[0];
		 pmnc[1] =ptemp;
	 	 pmnc[0] =  '0';
	}
	else if ((ucPlmn_last[3] == 0x30) && (ucPlmn_last[4] == 0x30))
	{
		pmnc[0] =   '0';
		pmnc[1] =   '0';
	}

	// LOG(" pmcc --> \"%s\"  \n", pmcc);
	// LOG(" pmnc --> \"%s\"  \n", pmnc);

	ucpmcc[0] =  pmcc[0];
	ucpmcc[1] =  pmcc[1];
	ucpmcc[2] =  pmcc[2];
	
	ucpmnc[0] =  pmnc[0];
	ucpmnc[1] =  pmnc[1];

	//LOG(" ucpmcc --> \"%s\"  \n", ucpmcc);
      // LOG(" ucpmnc --> \"%s\"  \n", ucpmnc);

	if ((tmcc == 0) && (tmnc == 0) )
	{
		ucpmcc[0] =   '0';
		ucpmcc[1] =   '0';
		ucpmcc[2] =   '0';
	
		ucpmnc[0] =   '0';
		ucpmnc[1] =   '0';
		//LOG(" tmcc and tmnc = 0 .\n");
	}
	else if ((tmcc != 0) && (tmnc == 0) )
	{
		ucpmnc[0] =   '0';
		ucpmnc[1] =   '0';
		//LOG(" tmcc != 0) && (tmnc == 0 .\n");
	}
	

	 LOG(" mcc --> \"%s\"  \n", ucpmcc);
	 LOG(" mnc --> \"%s\"  \n", ucpmnc);
	
}




#endif

//Remote USIM data macro
#define USIM_DATA_PTR_INCL_DATA_TYPE_AND_LEN (cur+PROTOCOL_HEADER_LEN_OF_REMOTE_UIM_DATA_PCK-FIELD_LEN_OF_DATA_TYPE_AND_DATA_LEN)
#define USIM_DATA_LEN_INCL_DATA_TYPE_AND_LEN (USIM_len+FIELD_LEN_OF_DATA_TYPE_AND_DATA_LEN)
#define USIM_DATA_PTR (cur+PROTOCOL_HEADER_LEN_OF_REMOTE_UIM_DATA_PCK)
#define USIM_DATA_LEN (USIM_len)

//parameters:
//	err_num:   0: rsp is correct.
//					 1: Wrong parameter input.
//					 2: Wrong message head received. Need to re-send request.
//					 3: Wrong check byte found. Need to re-send request.
//					 4: ID authentication failed. Flow ends.
//					 5: Wrong UEID of response found. Need to re-send request.
//					 6: Wrong req_type found. The req_type of response and request should be identical.
//					 7: ID authentication rejected. Retry it later.
//					 8: Rsp of power-down msg is correct.
//					 9: Rsp of registration msg is correct.
//					 10: ID authentication failed. Flow ends.  // add  by jack  li 2016/02/18
//					 11: the device doesn't find at the server list.  // add  by jack  li 2016/02/18
//					 12: debug 98 62 .  // add  by jack  li 2016/02/29
//Note:
//		The proto_decode_wrapped_data() generates YY_PROCESS_MSG_MDM9215_STARTUP_INF_IND msg. And, as a decoder, this 
//	function is not necessary and redundant.

typedef struct APN3G4G_config_type_s
{
  unsigned char plmntag;
  unsigned char plmn[3];
  unsigned char apn_3G_tag;
  unsigned char apn_3G_len;
  unsigned char apn_3GName_tag;
  unsigned char apn_3GName_len;
  unsigned char apn_3GName[30];
  unsigned char apn_3GUser_tag;
  unsigned char apn_3GUser_len;
  unsigned char apn_3GUser[30];
  unsigned char apn_3GPassword_tag;
  unsigned char apn_3GPassword_len;
  unsigned char apn_3GPassword[30];

  unsigned char apn_4G_tag;
  unsigned char apn_4G_len;
  unsigned char apn_4GName_tag;
  unsigned char apn_4GName_len;
  unsigned char apn_4GName[30];
  unsigned char apn_4GUser_tag;
  unsigned char apn_4GUser_len;
  unsigned char apn_4GUser[30];
  unsigned char apn_4GPassword_tag;
  unsigned char apn_4GPassword_len;
  unsigned char apn_4GPassword[30];

}APN3G4G_config_type;

 #ifdef FEATURE_ENABLE_IDCHECK_C7_C8

/**********************************************************************
* SYMBOL	: WriteUpdateFlagToFile
* FUNCTION	: 
* PARAMETER	: WriteUpdateFlagToFile
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/11/10
**********************************************************************/

static pthread_mutex_t fileoption_mutex = PTHREAD_MUTEX_INITIALIZER;

int WriteUploadlogFlagToFile(unsigned long ttvalue)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char buf[100];
	int itest;
	//static unsigned long last_value = 0;
	
	memset(buf, 0 ,sizeof(buf));

	pthread_mutex_lock(&fileoption_mutex);

	if ( (file = fopen("/usr/bin/save_uploadlog.txt", "wb+")) < 0 )
	{
		LOG(" ERROR : Socket_qlinkdatanode.c: ~~~~status_Loop: fopen() error. errno=%s(%d).\n", strerror(errno), errno);
	}

       time(&t);
	time_write = localtime(&t); 

	sprintf(buf, "save_uploadlog: %d, %d-%d-%d, %d:%d:%d\n", ttvalue, time_write->tm_year+1900, time_write->tm_mon+1, time_write->tm_mday, time_write->tm_hour, time_write->tm_min, time_write->tm_sec);
	itest = fwrite(buf, sizeof(char), strlen(buf), file);
	fflush(file);

	fclose(file);

	/*LOG(" Rw_txt :fwrite fwrite  time_write->tm_year =%d.\n", time_write->tm_year+1900);
	LOG(" Rw_txt :fwrite fwrite  time_write->tm_mon =%d.\n", time_write->tm_mon+1);
	LOG(" Rw_txt :fwrite fwrite  time_write->tm_mday =%d.\n", time_write->tm_mday);
	LOG(" Rw_txt :fwrite fwrite  time_write->tm_hour =%d.\n", time_write->tm_hour);
	LOG(" Rw_txt :fwrite fwrite  time_write->tm_min =%d.\n", time_write->tm_min);
	LOG(" Rw_txt :fwrite fwrite  time_write->tm_sec =%d.\n", time_write->tm_sec);*/

	LOG(" Rw_txt :fwrite fwrite  itest =%d.\n", itest);

	pthread_mutex_unlock(&fileoption_mutex);

	return STATUS_OK;
	
}
#endif




/**********************************************************************
* SYMBOL	: WriteIPaddressMessageToFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/01/19
**********************************************************************/

int WriteIPaddressMessageToFile(char * apnBuf, int lenth)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char *tempbuf;
	int itest;
	
	tempbuf = apnBuf;

	if ( (file = fopen("/var/addressMessage.txt", "wb+")) < 0 )
	{
		LOG(" ERROR :  addressMessage  fopen() error. errno=%s(%d).\n", strerror(errno), errno);
		return STATUS_ERROR;
	}

	itest = fwrite(tempbuf, sizeof(char), lenth, file);
	fflush(file);
	fclose(file);

	return STATUS_OK;
	
}


/**********************************************************************
* SYMBOL	: ReadIPaddressMessageToFile
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 1
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2017/01/19
**********************************************************************/

int ReadIPaddressMessageToFile(char * apnBuf, int lenth)
{
	FILE * file;
	struct tm *time_write;
	time_t  t;
	char *tempbuf;
	int itest;
	
	tempbuf = apnBuf;

	if ( (file = fopen("/var/addressMessage.txt", "rb+")) < 0 )
	{
		LOG(" ERROR :  addressMessage  fopen() error. errno=%s(%d).\n", strerror(errno), errno);
		return STATUS_ERROR;
	}

	itest = fread(tempbuf, sizeof(char), lenth, file);
	fclose(file);

	return STATUS_OK;
	
}


void *proto_decode_wrapped_data(void *data, int *err_num, rsmp_control_block_type *fi)
{
  char *cur = NULL;
 
  unsigned int pck_len = 0;
  char req_t;
  void *ret = NULL;
  int checkout_len = 0;
  int i = 1 ;
  char  xor_result = 0;

  char  key_temp[8] = {0};
  char  server_des_backup[16] = {0};

  unsigned char unProgrameFlowMode = 0;
  unsigned char unFluOrBookModeAndLanguage = 0;

  unsigned int userTimrforServer = 0;
  long long userTotalTrafficforServer = 0;
  long long userTrafficfromServer = 0;
  long long userTrafficfromFile = 0;
  long long modeTemp = 0;
  unsigned int  userModeTemp = 0;

#ifdef FEATURE_ENABLE_MESSAGE_TO_YY_DAEMON_CANCEL_APPLE0302

  unsigned int  flucontrol_data1_temp = 0;
  unsigned int  flucontrol_data2_temp = 0;
  
 #endif

#ifdef FEATURE_ENABLE_APNCONFIG_FROM_SERVER

  char  apnname[30];
  char  apnuser[30];
  char  apnpassword[30];
  APN3G4G_config_type *stp_APNConfig = NULL;
  char ptem_mcc[3];
  char ptem_mnc[2];

  char *pdefault_str="3gnet";
  char *pdefault_plmnMCC="460";
  char *pdefault_plmnMNC="01";

  char *pdefault_apn=" ";
  char *pdefault_user=" ";
  char *pdefault_password=" ";

  unsigned char  getPlmn[3];

  int templen=0;
  unsigned int tcp_ip_temp = 0;
  unsigned int tcp_ip_len = 0;
  unsigned int tcp_ip_count = 0;
 
  
#endif

  strflu_record userTrafficTemp;
  long long int tx,rx;

  unsigned int switch_simcard_fluValue = 0;
  unsigned int ld_usedTimeByUserMintes = 0;

  char exp[50] = "";

   #ifdef FEATURE_ENABLE_EmodemUDP_LINK

  // static bool b_copy_onlyonce = true;

   #endif

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG
   char temp_keyServerbuf[8];
   unsigned long long temp_key_server = 0;
   int i_temp,ii_temp;
#endif  
 
  if(data == NULL)
  {
    LOG("ERROR: Wrong parameter input. data=null.\n");
    *err_num = 1;
    return NULL;
  }else
  {
    cur = (char *)data;
  }

  if(cur[0] != 0xaa){
    LOG("ERROR: Wrong msg head found. Msg head=%c.\n", cur[0]);
    *err_num = 2;
    return NULL;
  }

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

  LOG(" Debug: b_enable_softwarecard_ok = %d \n",b_enable_softwarecard_ok);

  if (b_enable_softwarecard_ok == true){
	  if((cur[5] == REQ_TYPE_APDU) && (cur[14] == 0x98) && (cur[15] == 0x62) && (cur[16] == 0x00)){
	    LOG("Debug: Soft card auth is invalid.\n");
	    *err_num = 6;
	    return NULL;
	  }
  }

 #endif

  pck_len = *((unsigned int *)(cur+1));
  switch_endian((unsigned char *)(&pck_len), 4);
  checkout_len = pck_len+FIELD_LEN_OF_MSG_HEAD_AND_MSG_LEN-1;
  xor_result = (char) byte_xor_checkout(cur, checkout_len);

  LOG("Debug: checkout_len = %d.\n", checkout_len);
  LOG("Debug: xor_result = 0x%02x.\n", xor_result);
  LOG("Debug: cur[checkout_len] = 0x%02x.\n", cur[checkout_len]);
  
  if( xor_result != cur[checkout_len] ){
  LOG("ERROR: Wrong check byte found. Last byte of pck(index: %d): 0x%02x. Theoretical val: 0x%02x.\n",
              checkout_len, cur[checkout_len], byte_xor_checkout(cur, checkout_len));

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

   if (b_enable_softwarecard_ok == true){

		 LOG("Debug: Soft card apdu data xor error.\n");
		 *err_num = 6;
   }else{

	*err_num = 3;
   }

#else

   *err_num = 3;

#endif


  return NULL;
  }

  req_t = cur[5];
  if(req_t != fi->req_type)
  {

#ifdef   FEATURE_ENABLE_EmodemUDP_LINK
   if((req_t == 0x10) && (fi->req_type == 0x60)){
	 LOG("NOTICE:UDP link .\n");
   }
   else if ( (req_t == 0x70) || (req_t == 0x71)){

		LOG("NOTICE:Softcard flow  .\n");
   }
   
   else
   {
	 LOG("NOTICE: Wrong req_type found. req_type of rsp=0x%02x, req_type of req=0x%02x.\n", req_t, fi->req_type);
        *err_num = 6;
       return NULL;
   }

#else
    LOG("NOTICE: Wrong req_type found. req_type of rsp=0x%02x, req_type of req=0x%02x.\n", req_t, fi->req_type);
    *err_num = 6;
    return NULL;

#endif
  }
  LOG("Rsp of req_type %02x found.\n", req_t);
  switch((rsmp_protocol_type)req_t)
  {

 #ifdef FEATURE_ENABLE_EmodemUDP_LINK
 case REQ_TYPE_Emodem_UDP:

	

	 LOG("Debug:REQ_TYPE_Emodem_UDP %02x found.\n", REQ_TYPE_Emodem_UDP);
	 
	 g_EmodemUdp_flag = false;

	 LOG("Debug:g_EmodemUdp_flag %d .\n", g_EmodemUdp_flag);
	 
	
	 tcp_server_port = hex_to_int(&cur[11],2);
	 if((tcp_server_port <= 0) ||(tcp_server_port > 9999)){
		LOG("ERROR:The IP port is invaild .\n");
		//system("reboot");
		yy_popen_call("reboot -f");
		*err_num = 0xbb;
		return NULL;
	 }
	 memset(tcp_IP_addr,0,16);

	  for(tcp_ip_count = 7; tcp_ip_count <= 10; tcp_ip_count++){

		tcp_ip_temp = (unsigned int) cur[tcp_ip_count]; 
		//LOG("Debug:  tcp_ip_temp %d .\n", tcp_ip_temp);
		
	       if((tcp_ip_temp <= 0) ||(tcp_ip_temp >= 255)){
		LOG("ERROR:The IP ADDR is invaild .\n");
		//system("reboot");
		yy_popen_call("reboot -f");
		*err_num = 0xbb;
		return NULL;
		}
		   
		 if(tcp_ip_count == 10){
			sprintf(&tcp_IP_addr[templen],"%d",tcp_ip_temp);
		 }else{
			 sprintf(&tcp_IP_addr[templen],"%d",tcp_ip_temp);
			 tcp_ip_len =  strlen(&tcp_IP_addr[templen]);
			// LOG("Debug:  tcp_ip_len %d .\n", tcp_ip_len);
			 strcpy(&tcp_IP_addr[tcp_ip_len+templen],".");
			 templen = tcp_ip_len+templen+1;
			// LOG("Debug:  templen %d .\n", templen);
		 }

	  }

	  LOG("Debug:  tcp_server_port %d .\n", tcp_server_port);
	  LOG("Debug:  tcp_IP_addr --> \"%s\"  \n", tcp_IP_addr);


	 templen = 0;
	
	 tcp_server_port_2 = hex_to_int(&cur[18],2);
	 if((tcp_server_port_2 <= 0) ||(tcp_server_port_2 > 9999)){
		LOG("ERROR:The IP port is invaild tcp_server_port_2 .\n");
		//system("reboot");
		yy_popen_call("reboot -f");
		*err_num = 0xbb;
		return NULL;
	 }
	 memset(tcp_IP_addr_2,0,16);

	  for(tcp_ip_count = 14; tcp_ip_count <= 17; tcp_ip_count++){

		tcp_ip_temp = (unsigned int) cur[tcp_ip_count]; 
		//LOG("Debug: 00  tcp_ip_temp %d .\n", tcp_ip_temp);
	       if((tcp_ip_temp <= 0) ||(tcp_ip_temp >= 255)){
		LOG("ERROR:The IP ADDR is invaild .\n");
		//system("reboot");
		yy_popen_call("reboot -f");
		*err_num = 0xbb;
		return NULL;
		}
		   
		 if(tcp_ip_count == 17){
			sprintf(&tcp_IP_addr_2[templen],"%d",tcp_ip_temp);
		 }else{
			 sprintf(&tcp_IP_addr_2[templen],"%d",tcp_ip_temp);
			 tcp_ip_len =  strlen(&tcp_IP_addr_2[templen]);
			// LOG("Debug: 00  tcp_ip_len %d .\n", tcp_ip_len);
			 strcpy(&tcp_IP_addr_2[tcp_ip_len+templen],".");
			 templen = tcp_ip_len+templen+1;
			// LOG("Debug: 00  templen %d .\n", templen);
		 }
		 

	  }

	  LOG("Debug:  tcp_server_port_2 %d .\n", tcp_server_port_2);
	  LOG("Debug:  tcp_IP_addr_2 --> \"%s\"  \n", tcp_IP_addr_2);

#ifdef FEATURE_ENABLE_UPDATE_IP3_DEBUG

	 templen = 0;
	
	 tcp_server_port_update = hex_to_int(&cur[25],2);
	
	 memset(tcp_IP_addr_update,0,16);

	 for(tcp_ip_count = 21; tcp_ip_count <= 24; tcp_ip_count++){

		tcp_ip_temp = (unsigned int) cur[tcp_ip_count]; 
		//LOG("Debug:  tcp_ip_temp %d .\n", tcp_ip_temp);
	       if((tcp_ip_temp <= 0) ||(tcp_ip_temp >= 255)){
		LOG("ERROR:The IP ADDR is invaild .\n");
		}
		   
		 if(tcp_ip_count == 24){
			sprintf(&tcp_IP_addr_update[templen],"%d",tcp_ip_temp);
		 }else{
			 sprintf(&tcp_IP_addr_update[templen],"%d",tcp_ip_temp);
			 tcp_ip_len =  strlen(&tcp_IP_addr_update[templen]);
			// LOG("Debug:  tcp_ip_len %d .\n", tcp_ip_len);
			 strcpy(&tcp_IP_addr_update[tcp_ip_len+templen],".");
			 templen = tcp_ip_len+templen+1;
			// LOG("Debug:  templen %d .\n", templen);
		 }
		 

	  }

	  LOG("Debug:  tcp_server_port_update = %d .\n", tcp_server_port_update);
	  LOG("Debug:  tcp_IP_addr_update --> \"%s\"  \n", tcp_IP_addr_update);

	  WriteIPaddressMessageToFile(&cur[21],6);
	#endif
	  
	  LOG("Debug:  g_record_ID_timeout_conut = %d.\n", g_record_ID_timeout_conut);
	  g_record_ID_timeout_conut = 0;
	
	 *err_num = 0xaa;
	 
	 break;
#endif

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG

       case REQ_TYPE_SOFT_KEY:

	LOG("Debug:  g_randGetkey = %d.\n", g_randGetkey);
	LOG("Debug:  g_SoftcardFlowState = %d.\n", g_SoftcardFlowState);

	memset(temp_keyServerbuf,0,sizeof(temp_keyServerbuf));
	memcpy(temp_keyServerbuf, cur+7, sizeof(temp_keyServerbuf));

	LOG("Debug:  g_randGetkey = %d.\n", g_randGetkey);

	memset(keybuf_des,0,sizeof(keybuf_des));
	temp_key_server = GetKeyVlaueFromServerProcess(temp_keyServerbuf,g_randGetkey,keybuf_des);

	LOG5("**********Get key from server after des**********\n");
	i = 1;
	
       for(; i<= sizeof(keybuf_des); i++){
		if(i%20 == 1)
			LOG4(" %02x", get_byte_of_void_mem(keybuf_des, i-1) );
		else
			LOG3(" %02x", get_byte_of_void_mem(keybuf_des, i-1) );
		
		if(i%20 == 0) //Default val of i is 1
			LOG3("\n");
	}
	if((i-1)%20 != 0) // Avoid print redundant line break
		LOG3("\n");
	LOG4("**********Get key from server after des********** \n");

	
	LOG("Debug:  temp_key_server = %lld.\n", temp_key_server);
	LOG("Debug:  g_record_ID_timeout_conut = %d.\n", g_record_ID_timeout_conut);
	g_record_ID_timeout_conut = 0;
	g_SoftcardFlowState++;
	*err_num = 0;
	
	break;


	case REQ_TYPE_SOFT_KIOPC:

		memset(server_des_ki,0,sizeof(server_des_ki));
		memset(server_des_opc,0,sizeof(server_des_opc));
		memset(key_temp,0,sizeof(key_temp));
		memset(server_des_backup,0,sizeof(server_des_backup));
		memcpy(key_temp,keybuf_des,sizeof(key_temp));  
		
		DES_Decrypt(cur+7,sizeof(server_des_backup),key_temp,server_des_backup);
		memcpy(server_des_ki,server_des_backup,sizeof(server_des_backup));  

		memset(server_des_backup,0,sizeof(server_des_backup));
		memset(key_temp,0,sizeof(key_temp));
		memcpy(key_temp,keybuf_des,sizeof(key_temp));  
	
		DES_Decrypt(cur+24,sizeof(server_des_backup),key_temp,server_des_backup);

		memcpy(server_des_opc,server_des_backup,sizeof(server_des_backup));  
		memset(server_des_backup,0,sizeof(server_des_backup));

		LOG5("**********Get KI from server after des**********\n");
		i = 1;

		   for(; i<= sizeof(server_des_ki); i++){
			if(i%20 == 1)
				LOG4(" %02x", get_byte_of_void_mem(server_des_ki, i-1) );
			else
				LOG3(" %02x", get_byte_of_void_mem(server_des_ki, i-1) );
			
			if(i%20 == 0) //Default val of i is 1
				LOG3("\n");
		}
		if((i-1)%20 != 0) // Avoid print redundant line break
			LOG3("\n");
		LOG4("***********Get KI from server after des***********\n");

		LOG5("**********Get OPC from server after des**********\n");
		i = 1;

		   for(; i<= sizeof(server_des_opc); i++){
			if(i%20 == 1)
				LOG4(" %02x", get_byte_of_void_mem(server_des_opc, i-1) );
			else
				LOG3(" %02x", get_byte_of_void_mem(server_des_opc, i-1) );
			
			if(i%20 == 0) //Default val of i is 1
				LOG3("\n");
		}
		if((i-1)%20 != 0) // Avoid print redundant line break
			LOG3("\n");
		LOG4("****************************************\n");

		g_SoftcardFlowState++;

		LOG("Debug:  g_record_ID_timeout_conut = %d.\n", g_record_ID_timeout_conut);
	 	 g_record_ID_timeout_conut = 0;
		*err_num = 0;
	
	break;

#endif

  
  case REQ_TYPE_ID:
  {
    char ID_result;
    

 #ifdef FEATURE_ENABLE_IDCHECK_C7_C8

       char LogUpload_result = 0;
       
	unFluOrBookModeAndLanguage = cur[235] & 0x0f;
	LogUpload_result = cur[237] & 0x0f;

	LOG("Debug:unFluOrBookModeAndLanguage =  %d .\n", unFluOrBookModeAndLanguage);
	LOG("Debug:LogUpload_result = %d .\n", LogUpload_result);

	msq_send_Lcdversion_to_yydaemon(unFluOrBookModeAndLanguage);
	WriteUploadlogFlagToFile(LogUpload_result);

	unFluOrBookModeAndLanguage = cur[235] & 0xf0;
	LOG("Debug:unFluOrBookModeAndLanguage =  %d .\n", unFluOrBookModeAndLanguage);

#ifdef FEATURE_ENABLE_STSRT_TEST_MODE

	LogUpload_result = (cur[237] >> 4 ) & 0x0f;
       LOG("Debug:is test mode (1:test mode  0: not test mode ) = 0x%02x .\n", LogUpload_result);

	if (LogUpload_result == 1){

		isTestMode = true;
	}else{

		isTestMode = false;
	}
	
	LOG("Debug:isTestMode =  %d .\n", isTestMode);

#endif

	if (unFluOrBookModeAndLanguage != 0){

		isBookMode= true;
		modeTemp = 0xAA;
		WriteModeToFile(modeTemp);
			
	}else{
		
		isBookMode= false;
		
		if (ReadModeFromFile(&userModeTemp) == STATUS_ERROR ){

				LOG(" Debug: Read file is error or This is first times use for the deceive in unbook mode \n");
				modeTemp = 0;
				WriteModeToFile(modeTemp);
				ReadModeFromFile(&userModeTemp);
				WriteUserTrafficToFile(0);
				//WriteFluValueToFile(0,0);
				//WriteFluValueToFile(0,1);
				//WriteFluValueToFile(0,FILE_TYPE_SIMCARDFLUDATA);
		}
		
		LOG(" Debug: userModeTemp = 0x%02x. \n", userModeTemp);
		
		if (userModeTemp == 0xAA){
			
			LOG(" Debug: Last mode is BOOK mode  \n");
			modeTemp = 0;
			WriteModeToFile(modeTemp);
			WriteUserTrafficToFile(0);
			WriteFluValueToFile(0,0);
			WriteFluValueToFile(0,1);
			WriteFluValueToFile(0,FILE_TYPE_SIMCARDFLUDATA);
		}
	}

	LOG(" Debug: isBookMode = %d. \n", isBookMode);

 #endif

 #ifdef FEATURE_ENABLE_EmodemUDP_LINK
    g_EmodemUdp_flag = true;
    g_enable_EmodemTCP_link = false;

    LOG("Debug:g_EmodemUdp_flag %d .\n", g_EmodemUdp_flag);
    LOG("Debug:g_enable_EmodemTCP_link %d .\n", g_enable_EmodemTCP_link);
    LOG("Debug:g_enable_TCP_IP01_linkcount %d .\n", g_enable_TCP_IP01_linkcount);
    LOG("Debug:g_enable_TCP_IP02_linkcount %d .\n", g_enable_TCP_IP02_linkcount);
#endif

    LOG("Debug:  g_record_ID_timeout_conut = %d.\n", g_record_ID_timeout_conut);
    g_record_ID_timeout_conut = 0;

    memcpy(fi->UEID, cur+7, 4);

    ID_result = cur[12];
    
#ifdef FEATURE_ENABLE_MSGQ
    if(false == isIdResultNotified || pre_ID_result != ID_result){

	#ifdef FEATURE_ENABLE_STATUS_FOR_CHANGE_SIMCARD    // add by jackli 20160810
		if (ID_result == 0x01 )
		{
			g_qlinkdatanode_run_status_01 = 0x01;
		}
		else if (ID_result == 0x02)
		{
			g_qlinkdatanode_run_status_01 = 0x02;
		}
		else
			g_qlinkdatanode_run_status_01 = 0x00;

		 LOG("g_qlinkdatanode_run_status_01: 0x%02x.\n", g_qlinkdatanode_run_status_01);
	#endif

#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
	 LOG("Debug:ID_result =  0x%02x.\n", ID_result);
	if (ID_result == 0x08){
		
		msq_send_qlinkdatanode_run_stat_ind(40);
		
	}else{

		msq_send_qlinkdatanode_run_stat_ind((int)ID_result);

	}
#else

       msq_send_qlinkdatanode_run_stat_ind((int)ID_result);

#endif
      
	 
         isIdResultNotified = true;
    }
    pre_ID_result = ID_result;
#endif
    if(ID_result == 0x01){
      fi->isNeedtoReqRemoteUSIM = true;
      fi->flag = 0x00;

    //add by jack 2016/03/30.
   //Only the ID check  return result is 01,get value from eth0.
  //  isIdResult01 = true;
    rx = Getnet_traffic_info(0);
    tx = Getnet_traffic_info(1);
  
    LOG("rx : %lld, tx: %lld.\n", rx, tx);
    changesimcard_netfluValue = (rx + tx);
    LOG("changesimcard_netfluValue: %lld.\n", changesimcard_netfluValue);
	  
    }else if(ID_result == 0x03){
      LOG("Your ID authentication is rejected. ID result=%d. Retry it later.\n", ID_result);
       //isIdResult01 = false;
      *err_num = 7;
      changesimcard_netfluValue = 0;
      return NULL;
    }else if(ID_result == 0x04){
      fi->isNeedtoReqRemoteUSIM = true;
      fi->flag = 0x01;
     // isIdResult01 = false;
      changesimcard_netfluValue = 0;
    }
    else if (ID_result == 0x05)
    {
      LOG("Your ID authentication is refused. ID result=%d.\n", ID_result);
      *err_num = 10;
     // isIdResult01 = false;
      changesimcard_netfluValue = 0;
      return NULL;
    }
     else if (ID_result == 0x06)
    {
      LOG("the device doesn't find at the server list. ID result=%d.\n", ID_result);
      *err_num = 11;
    //  isIdResult01 = false;
     changesimcard_netfluValue = 0;
      return NULL;
    }
    else if ((ID_result != 0x02) && (ID_result != 0x08))
    {
 
#ifdef FEATURE_ENABLE_IDCHECK_7
      if (ID_result == 0x07){

		 LOG("Neet to reboot system.\n");
		// system("reboot");
		 yy_popen_call("reboot -f");
      }
#endif

	  
      LOG("Your ID authentication is refused. ID result=%d.\n", ID_result);
      *err_num = 4;
	//isIdResult01 = false;
	 changesimcard_netfluValue = 0;
      return NULL;
    }


#ifdef FEATURE_ENABLE_MSGQ
    isIdResultNotified = false; // Added by rjf at 20150915
#endif

#ifdef FEATURE_ENABLE_TIME_ZONE_IN_ID_CHECK
    memset(timezone_qlinkdatanode.val, 0, sizeof(timezone_qlinkdatanode.val));
    memcpy(timezone_qlinkdatanode.val, cur+14, sizeof(timezone_qlinkdatanode.val));
#endif

#ifdef FEATURE_ENABLE_Emodem_SET_TIME_DEBUG

    LOG("Debug :  ID CHECK RSP Get cclk time year :=  %d.\n", rsp_Emodemcclk_ime.year);
    LOG("Debug :  ID CHECK RSP Get cclk time month :=  %d.\n", rsp_Emodemcclk_ime.month);
    LOG("Debug :  ID CHECK RSP Get cclk time mday :=  %d.\n", rsp_Emodemcclk_ime.mday);
    LOG("Debug :  ID CHECK RSP Get cclk time hour :=  %d.\n", rsp_Emodemcclk_ime.hour);
    LOG("Debug :  ID CHECK RSP Get cclk time min :=  %d.\n", rsp_Emodemcclk_ime.min);
    LOG("Debug :  ID CHECK RSP Get cclk time sec :=  %d.\n", rsp_Emodemcclk_ime.sec);

    UTCDate(&rsp_Emodemcclk_ime,timezone_qlinkdatanode.val[0]);

    LOG("Debug :  ID CHECK RSP Get cclk time year :=  %d.\n", rsp_Emodemcclk_ime.year);
    LOG("Debug :  ID CHECK RSP Get cclk time month :=  %d.\n", rsp_Emodemcclk_ime.month);
    LOG("Debug :  ID CHECK RSP Get cclk time mday :=  %d.\n", rsp_Emodemcclk_ime.mday);
    LOG("Debug :  ID CHECK RSP Get cclk time hour :=  %d.\n", rsp_Emodemcclk_ime.hour);
    LOG("Debug :  ID CHECK RSP Get cclk time min :=  %d.\n", rsp_Emodemcclk_ime.min);
    LOG("Debug :  ID CHECK RSP Get cclk time sec :=  %d.\n", rsp_Emodemcclk_ime.sec);

    sprintf(exp, "date -s \"%d-%d-%d %d:%d:%d\"", rsp_Emodemcclk_ime.year,rsp_Emodemcclk_ime.month,rsp_Emodemcclk_ime.mday,rsp_Emodemcclk_ime.hour,rsp_Emodemcclk_ime.min,rsp_Emodemcclk_ime.sec);
    system(exp);
    LOG("Time updated.\n");

	
#endif



#ifdef FEATURE_ENABLE_LIMITFLU_FROM_SERVER

#ifdef  FEATURE_ENABLE_CANCEL_LOG_OUTPUT  //20160708
	 LOG("Protocol_qlinkdatanode.c :  ---------test----------- pck_len+5 = %d.\n", pck_len+5);
#endif

 LOG("Debug : Receive pck_len+5 = %d.\n", pck_len+5);

if (pck_len !=  0)
{
    LOG("Receive limit data from server : len != 0 .\n");
    memset(flucontrol_server.flu_limit_data_level1, 0, sizeof(flucontrol_server.flu_limit_data_level1));
    memcpy(flucontrol_server.flu_limit_data_level1, cur+18, sizeof(flucontrol_server.flu_limit_data_level1));

    memset(flucontrol_server.flu_limit_speed_level1, 0, sizeof(flucontrol_server.flu_limit_speed_level1));
    memcpy(flucontrol_server.flu_limit_speed_level1, cur+22, sizeof(flucontrol_server.flu_limit_speed_level1));

    memset(flucontrol_server.flu_limit_data_level2, 0, sizeof(flucontrol_server.flu_limit_data_level2));
    memcpy(flucontrol_server.flu_limit_data_level2, cur+26, sizeof(flucontrol_server.flu_limit_data_level2));

    memset(flucontrol_server.flu_limit_speed_level2, 0, sizeof(flucontrol_server.flu_limit_speed_level2));
    memcpy(flucontrol_server.flu_limit_speed_level2, cur+30, sizeof(flucontrol_server.flu_limit_speed_level2));	
}
else
{
    LOG("Receive limit data from server : len == 0 .\n");
    memset(flucontrol_server.flu_limit_data_level1, 0, sizeof(flucontrol_server.flu_limit_data_level1));
    memset(flucontrol_server.flu_limit_speed_level1, 0, sizeof(flucontrol_server.flu_limit_speed_level1));
    memset(flucontrol_server.flu_limit_data_level2, 0, sizeof(flucontrol_server.flu_limit_data_level2));
    memset(flucontrol_server.flu_limit_speed_level2, 0, sizeof(flucontrol_server.flu_limit_speed_level2));
}

#endif

#ifdef FEATURE_ENABLE_APNCONFIG_FROM_SERVER

 	 memset(apnMessageBuff,0,APN_MESSAGE_LEN);
	 memcpy(apnMessageBuff,&cur[34],APN_MESSAGE_LEN);
	 msq_send_APN_message_to_yydaemon();

        stp_APNConfig = cur+30+4;

	 memset(apnname,0,sizeof(apnname));
	 memset(apnuser,0,sizeof(apnuser));
        memset(apnpassword,0,sizeof(apnpassword));
		
	 memcpy(apnname,stp_APNConfig->apn_3GName,stp_APNConfig->apn_3GName_len);
	 LOG(" Get apnName of 3G from server --> \"%s\"  \n", apnname);
	 memcpy(apnuser,stp_APNConfig->apn_3GUser,stp_APNConfig->apn_3GUser_len);
	 LOG(" Get apnUser of 3G from server --> \"%s\"  \n", apnuser);
	 memcpy(apnpassword,stp_APNConfig->apn_3GPassword,stp_APNConfig->apn_3GPassword_len);
	 LOG(" Get apnPassword of 3G from server --> \"%s\"  \n", apnpassword);

	 memcpy(apnname,stp_APNConfig->apn_4GName,stp_APNConfig->apn_4GName_len);
	 LOG(" Get apnName of 4G from server --> \"%s\"  \n", apnname);
	 memcpy(apnuser,stp_APNConfig->apn_4GUser,stp_APNConfig->apn_4GUser_len);
	 LOG(" Get apnUser of 4G from server --> \"%s\"  \n", apnuser);
	 memcpy(apnpassword,stp_APNConfig->apn_4GPassword,stp_APNConfig->apn_4GPassword_len);
	 LOG(" Get apnPassword of 4G from server --> \"%s\"  \n", apnpassword);
		
#endif

#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG
if ((pck_len+5) > 239){

	unProgrameFlowMode = (cur[239] & 0x0f);
	LOG(" Debug: unProgrameFlowMode = 0x%02x  \n", unProgrameFlowMode);
	if (unProgrameFlowMode == 0x01){


		LOG(" Debug: Should be softcard flow  \n");
		LOG(" Debug: g_SoftcardFlowState = %d \n",g_SoftcardFlowState);
		LOG(" Debug: b_enable_softwarecard_ok = %d \n",b_enable_softwarecard_ok);

		b_enable_softwarecard_ok = true;
		//zhijie 20170330,begin
		if (true == b_enable_softwarecard_ok)
			msq_send_SoftSim_mode_message_to_yydaemon();  
		//zhijie 20170330,end

		if (g_SoftcardFlowState < SOFT_CARD_STATE_FLOW_MAX ){

			g_SoftcardFlowState = SOFT_CARD_STATE_FLOW_GETKEY_REG;
			
		}else{

			g_SoftcardFlowState = SOFT_CARD_STATE_FLOW_DEFAULT;
		}
			

	}else{

		LOG(" Debug: Don't should be softcard flow  \n");
		g_SoftcardFlowState = SOFT_CARD_STATE_FLOW_DEFAULT;
		b_enable_softwarecard_ok = false;
	}


	unProgrameFlowMode = (cur[239] & 0xf0);
	LOG(" Debug: unProgrameFlowMode = 0x%02x  \n", unProgrameFlowMode);
	if(unProgrameFlowMode == 0x10){

		LOG(" Debug: Jump window for customer  \n");
		msqSendToTrafficAndDateYydaemon(NOTE_JUMP_WINDOWN,0ULL);
	}
}

#endif


#ifdef FEATURE_ENABLE_MESSAGE_TO_YY_DAEMON_CANCEL_APPLE0302

       flucontrol_data1_temp = hex_to_int(&flucontrol_server.flu_limit_data_level1[0],4);
	LOG(" Debug: flucontrol_data1_temp = %d \n",flucontrol_data1_temp);

	flucontrol_data2_temp = hex_to_int(&flucontrol_server.flu_limit_data_level2[0],4);
	LOG(" Debug: flucontrol_data2_temp = %d \n",flucontrol_data2_temp);

	
	if( (flucontrol_data1_temp == 0) || (flucontrol_data1_temp == 0)){

		LOG(" Debug: Disable iptale for apple  \n");
		msqSendToTrafficAndDateYydaemon(NOTE_CANCEL_APPLE,2ULL);

	}else {

		LOG(" Debug: Enable iptale for apple  \n");
		msqSendToTrafficAndDateYydaemon(NOTE_CANCEL_APPLE,1ULL);
	}
	
#endif


#ifdef FEATURE_ENABLE_OUT_OF_TRAFFIC_AND_DATE_0206
if ((pck_len+5)  > 240){
	if (isBookMode == true){

		LOG(" Debug: Count flu data start.  \n");
		memset(trafficDataServer.trafficBuyTotal, 0, sizeof(trafficDataServer.trafficBuyTotal));
    		memcpy(trafficDataServer.trafficBuyTotal, cur+241, sizeof(trafficDataServer.trafficBuyTotal));

		memset(trafficDataServer.usedTrafficByUser, 0, sizeof(trafficDataServer.usedTrafficByUser));
    		memcpy(trafficDataServer.usedTrafficByUser, cur+250, sizeof(trafficDataServer.usedTrafficByUser));

		
		memset(trafficDataServer.usedTimeByUser, 0, sizeof(trafficDataServer.usedTimeByUser));
    		memcpy(trafficDataServer.usedTimeByUser, cur+259, sizeof(trafficDataServer.usedTimeByUser));

		
			
		if((trafficDataServer.trafficBuyTotal[0] != 0) || (trafficDataServer.trafficBuyTotal[1] != 0) || (trafficDataServer.trafficBuyTotal[2] != 0)|| (trafficDataServer.trafficBuyTotal[3] != 0) || (trafficDataServer.trafficBuyTotal[4] != 0) || (trafficDataServer.trafficBuyTotal[5] != 0) || (trafficDataServer.trafficBuyTotal[6] != 0) || (trafficDataServer.trafficBuyTotal[7] != 0)){

			userTotalTrafficforServer = hex_to_long(&trafficDataServer.trafficBuyTotal[0],sizeof(trafficDataServer.trafficBuyTotal));
			LOG("Debug: This is getting from server at first.   userTotalTrafficforServer =  %lld .\n", userTotalTrafficforServer);
			
			if (userTotalTrafficforServer >= 0){

				msqSendToTrafficAndDateYydaemon(NOTE_SHOW_BOOK,userTotalTrafficforServer);
			}
			
			

			if ((trafficDataServer.usedTrafficByUser[0] == 0) && (trafficDataServer.usedTrafficByUser[1] == 0) && (trafficDataServer.usedTrafficByUser[2] == 0)&& (trafficDataServer.usedTrafficByUser[3] == 0) && (trafficDataServer.usedTrafficByUser[4] == 0) && (trafficDataServer.usedTrafficByUser[5] == 0) && (trafficDataServer.usedTrafficByUser[6] == 0) && (trafficDataServer.usedTrafficByUser[7] == 0)){

			
				LOG(" Debug: This is first start deceive in BOOKMODE.  \n");
				WriteUserTrafficToFile(0);
				WriteFluValueToFile(0,0);
				WriteFluValueToFile(0,1);
				WriteFluValueToFile(0,FILE_TYPE_SIMCARDFLUDATA);
			
				LOG("Debug:  trafficDataServer.usedTimeByUser[0] =  0x%02x .\n", trafficDataServer.usedTimeByUser[0]);
				LOG("Debug:  trafficDataServer.usedTimeByUser[1] =  0x%02x .\n", trafficDataServer.usedTimeByUser[1]);
				LOG("Debug:  trafficDataServer.usedTimeByUser[2] =  0x%02x .\n", trafficDataServer.usedTimeByUser[2]);
				LOG("Debug:  trafficDataServer.usedTimeByUser[3] =  0x%02x .\n", trafficDataServer.usedTimeByUser[3]);

				userTimrforServer = hex_to_int(&trafficDataServer.usedTimeByUser[0],sizeof(trafficDataServer.usedTimeByUser));
				LOG("Debug: This is getting from server at first.   userTimrforServer =  %d .\n", userTimrforServer);
				WriteEndDateFromServerToFile(userTimrforServer);
				msqSendToTrafficAndDateYydaemon(NOTE_SHOW_MIN,(long long)userTimrforServer);
					
			}else{

			userTrafficfromServer = hex_to_long(&trafficDataServer.usedTrafficByUser[0],sizeof(trafficDataServer.usedTrafficByUser));
			LOG("Debug: This is getting from server in BOOK mode.   userTrafficfromServer =  %lld .\n", userTrafficfromServer);

			
			if (ReadUserTrafficFromFile(&userTrafficTemp) == STATUS_ERROR ){

				WriteUserTrafficToFile(0);
				ReadUserTrafficFromFile(&userTrafficTemp);
			}

			LOG("Debug:  userTrafficTemp.flu_value = : %lld .\n", userTrafficTemp.flu_value);
			userTrafficfromFile = userTrafficTemp.flu_value;
			LOG("Debug: Get the traffice by user from file after reboot.   userTrafficfromFile =  %lld .\n", userTrafficfromFile);
			
			if (userTrafficfromServer <= userTrafficfromFile){
				
				LOG(" Debug: The traffic for use is right form file.  \n");
				msq_send_flu_to_yydaemon(userTrafficfromFile,0xaa);

			}else{
				LOG(" Debug: The traffic for use is not right  form file.  \n");
				WriteUserTrafficToFile(userTrafficfromServer);
				msq_send_flu_to_yydaemon(userTrafficfromServer,0xaa);
			}

			
			if (userTrafficfromFile != 0){

				userTimrforServer = hex_to_int(&trafficDataServer.usedTimeByUser[0],sizeof(trafficDataServer.usedTimeByUser));
			       LOG("Debug: time is 0 ?  userTimrforServer =  %d .\n", userTimrforServer);
				WriteEndDateFromServerToFile(userTimrforServer);
				   
				if(userTimrforServer == 0){

					msqSendToTrafficAndDateYydaemon(NOTE_DATE_OUTOF,0ULL);
				}else{

					if (ReadEndDateFromServerFile(&ld_usedTimeByUserMintes) == STATUS_ERROR ){

						WriteEndDateFromServerToFile(0);
						ReadEndDateFromServerFile(&ld_usedTimeByUserMintes);
					}

					LOG("Debug:   ld_usedTimeByUserMintes =  %d .\n", ld_usedTimeByUserMintes);

					msqSendToTrafficAndDateYydaemon(NOTE_SHOW_MIN,(long long)ld_usedTimeByUserMintes);


				}

			}

			}



		}

		
		
	}else{

		LOG(" Debug: Don't count flu data start.  \n");
    		memset(trafficDataServer.trafficBuyTotal, 0, sizeof(trafficDataServer.trafficBuyTotal));
		memset(trafficDataServer.usedTrafficByUser, 0, sizeof(trafficDataServer.usedTrafficByUser));
		memset(trafficDataServer.usedTimeByUser, 0, sizeof(trafficDataServer.usedTimeByUser));
		WriteUserTrafficToFile(0);
		WriteEndDateFromServerToFile(0);
	}
}
#endif

      LOG("Debug:ID_result =  0x%02x.\n", ID_result);

	if (ID_result == 0x08){
		
		isCancelRecoveryNet = true;
		LOG("Debug:This is out of traffic.\n");
		LOG("Debug:   isCancelRecoveryNet = %d \n",isCancelRecoveryNet);
		msq_send_qlinkdatanode_run_stat_ind(40);
		*err_num = 12;
	       changesimcard_netfluValue = 0;
     	       return NULL;
	}

    *err_num = 0;
    break;
  } // case REQ_TYPE_ID

  case REQ_TYPE_APDU:
  {
    unsigned short APDU_len = 0;

    if(strncmp(fi->UEID, (char *)(cur+7), 4) != 0){
      *err_num = 5;
      return NULL;
    }

    APDU_len = cur[13]+256*cur[12]; //APDU_len is big-endian data.

    fi->data_len = APDU_len;

    // jack li 20160229
    if ((cur[14] == 0x98 ) && (cur[15] == 0x62))
    {
	 LOG("receive data from server cur[14]=0x%02x, cur[15]=0x%02x \n", cur[14],cur[15]);
	// msq_send_qlinkdatanode_run_stat_ind(20);
     }
    *err_num = 0;
	
    break;
  } // case REQ_TYPE_APDU

  case REQ_TYPE_REMOTE_UIM_DATA:
  {
    unsigned int USIM_len = 0;

    msq_send_qlinkdatanode_run_stat_ind(4);

    if(strncmp(fi->UEID, (char *)(cur+7), 4) != 0){
      *err_num = 5;
      return NULL;
    }

    USIM_len = *((unsigned int *)(cur+12));
    switch_endian((unsigned char *)(&USIM_len), 4);
    fi->data_len = USIM_DATA_LEN_INCL_DATA_TYPE_AND_LEN; //Sending data length

    *err_num = 0;
    ret = USIM_DATA_PTR_INCL_DATA_TYPE_AND_LEN;
    break;
  } // case REQ_TYPE_REMOTE_UIM_DATA

  case REQ_TYPE_PWR_DOWN:
  {
    if(strncmp(fi->UEID, (char *)(cur+7), 4) != 0){
      *err_num = 5;
      return NULL;
    }

    *err_num = 8;
    break;
  }

  case REQ_TYPE_REG:
  {
    if(strncmp(fi->UEID, (char *)(cur+7), 4) != 0){
      *err_num = 5;
      return NULL;
    }

    *err_num = 9;
    break;
  }

  case REQ_TYPE_STATUS:
  {
    LOG("ERROR: Wrong req_type found. req_type=%d.\n", REQ_TYPE_STATUS);
    break;
  } // case REQ_TYPE_STATUS

  default:
    break; // Not possible to reach here.
  }

  return ret;
}

rsmp_protocol_type proto_get_req_cfg_data_type(rsmp_transmit_buffer_s_type *p_req_cfg)
{
  if(NULL == p_req_cfg){
    LOG("ERROR: Wrong param input. p_req_cfg=null.\n");
    return REQ_TYPE_IDLE;
  }

  if(NULL == p_req_cfg->data){
    LOG("The member \"data\" of req config struct is empty!\n");
    return REQ_TYPE_IDLE;
  }

  return (rsmp_protocol_type)(get_byte_of_void_mem(p_req_cfg->data, PROTO_REQ_TYPE_IDX));
}

#endif // FEATURE_ENABLE_PRIVATE_PROTOCOL
