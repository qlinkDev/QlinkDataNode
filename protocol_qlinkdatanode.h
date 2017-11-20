#ifndef PROTOCOL_qlinkdatanode_H_INCLUDED
#define PROTOCOL_qlinkdatanode_H_INCLUDED

#include "common_qlinkdatanode.h"
#include "feature_macro_qlinkdatanode.h"


#define FIELD_LEN_OF_MSG_HEAD (1)
#define FIELD_LEN_OF_MSG_HEAD_AND_MSG_LEN (5)
#define FIELD_LEN_OF_DATA_TYPE_AND_DATA_LEN (5)
#define INDEX_OF_LAST_BYTE_OF_MSG_LEN_PLUS_ONE (5)

#define PROTOCOL_HEADER_LEN_OF_APDU_COMM (14)
#define PROTOCOL_HEADER_LEN_OF_REMOTE_UIM_DATA_PCK (16)

#define PROTO_NET_COND_MAX_VAL (0x05)
#define PROTO_NET_COND_MIN_VAL (0x00)
#define PROTO_PIN_STATE_MAX_VAL (0x02)
#define PROTO_PIN_STATE_MIN_VAL (0x01)
#define PROTO_MOD_STATE_MAX_VAL (0x15)
#define PROTO_MOD_STATE_MIN_VAL (0x00)
#define PROTO_3G_REG_STATE_MAX_VAL (0x05)
#define PROTO_3G_REG_STATE_MIN_VAL (0x00)
#define PROTO_4G_REG_STATE_MAX_VAL (0x08)
#define PROTO_4G_REG_STATE_MIN_VAL (0x00)

#define PROTO_REQ_TYPE_IDX (5) // Start from 0

#define STATUS_OK		 (1)
#define STATUS_ERROR 	 (-1)
#define STATUS_MIDDLE 	 (2)


typedef enum rsmp_protocol_type_e
{
  REQ_TYPE_IDLE = 0x00, //FIXME
  REQ_TYPE_ID = 0x60,
  REQ_TYPE_APDU = 0x62,
  REQ_TYPE_STATUS = 0x63,
  REQ_TYPE_REMOTE_UIM_DATA = 0x64,
  REQ_TYPE_PWR_DOWN = 0x65,
  REQ_TYPE_REG = 0x66, // mdmImodem registration
  
#ifdef FEATURE_ENABLE_SFOTCARD_DEBUG_1229DEBUG 

  REQ_TYPE_SOFT_KEY = 0x70, 	// add by jackli 20161229
  REQ_TYPE_SOFT_KIOPC = 0x71 	// add by jackli 20161229
  
 #endif
  
}rsmp_protocol_type;

typedef enum _Data_Type_Tag{
  DATA_TYPE_IMEI = 0xa0,
  DATA_TYPE_ALL_DATA = 0xa1,
  DATA_TYPE_IMSI = 0xa2,
  DATA_TYPE_GCI = 0xb0,
  DATA_TYPE_ID_RESULT = 0xc0,
  DATA_TYPE_FCP = 0xc1,
  DATA_TYPE_UEID = 0xd0,
  DATA_TYPE_DATA = 0xd1,
  DATA_TYPE_FILE_ID = 0x80,
  DATA_TYPE_FCP_AND_DATA = 0xe0,
  DATA_TYPE_APDU = 0xf0,
  DATA_TYPE_FLUX = 0x20,
  DATA_TYPE_POWER = 0x21,
  DATA_TYPE_SIG = 0x22,
  DATA_TYPE_REG = 0x23
}Data_Type;

#define DATA_LEN_FLUX (0x08)
#define DATA_LEN_POWER (0x01)
#define DATA_LEN_SIG (0x01)
#define DATA_LEN_REG (0XA)

#define REQ_TYPE_Emodem_UDP (0x10)

typedef enum rsmp_state_type_s
{
  FLOW_STATE_IDLE, /*0*/
  FLOW_STATE_IMSI_ACQ, /*1*/
  FLOW_STATE_ID_AUTH, /*2*/
  FLOW_STATE_REMOTE_UIM_DATA_REQ, /*3*/
  FLOW_STATE_SET_OPER_MODE, /*4*/
  FLOW_STATE_APDU_COMM, /*5*/
  FLOW_STATE_STAT_UPLOAD, /*6*/
  FLOW_STATE_PWR_DOWN_UPLOAD, /*7*/
  FLOW_STATE_REG_UPLOAD, /*8*/
  FLOW_STATE_RESET_UIM, /*9*/
  FLOW_STATE_MDMEmodem_AT_CMD_REQ, /*10*/
  FLOW_STATE_MAX /**/
}rsmp_state_s_type;

typedef struct rsmp_control_block_type_s
{
  char UEID[4];
  rsmp_state_s_type flow_state;
  rsmp_protocol_type req_type;
  bool isNeedtoReqRemoteUSIM;
  //Before downloading remote USIM data,
  //0x00: Remove local USIM data
  //0x01: Keep local USIM data, and cover it with newly downloaded remote USIM data.
  char flag;
 
  void *data;//rsp data and its length.
  unsigned int data_len;//rsp data and its length.
}rsmp_control_block_type;

extern void *proto_encode_raw_data(void *data, int *len, rsmp_protocol_type type, Dev_Info *sim5360_di, Dev_Info *rsmp_di);
extern void *proto_decode_wrapped_data(void *data, int *err_num, rsmp_control_block_type *fi);

#endif // PROTOCOL_qlinkdatanode_H_INCLUDED
