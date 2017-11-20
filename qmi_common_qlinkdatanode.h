#ifndef QMI_COMMON_qlinkdatanode_INCLUDED
#define QMI_COMMON_qlinkdatanode_INCLUDED

#include "qmi.h"
#include "qmi_client.h"
#include "qmi_idl_lib.h"

 //Dynamic buffer, not released explicitedly; Created when parse_qmi_ind at first time.
typedef struct _QMI_Msg_Config_Tag{
  qmi_client_type user_handle;
  qmi_idl_type_of_message_type msg_type;
  int msg_id;
  void *raw_data_buf;
  int raw_data_len;
  void *data_buf;
  int data_len;
}QMI_Msg_Config;

extern void init_qmi_msg_config(QMI_Msg_Config *config);

#endif //QMI_COMMON_qlinkdatanode_INCLUDED