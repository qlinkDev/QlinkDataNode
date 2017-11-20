#include <stdlib.h>
#include <pthread.h>

#include <qmi_idl_lib.h>

#include "qmi_common_qlinkdatanode.h"
#include "device_management_service_v01.h"


extern qmi_client_type dms_client_handle;

QMI_Msg_Config qmi_msg_config;
pthread_mutex_t qmi_msg_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_qmi_msg_config(QMI_Msg_Config *config)
{
  config->user_handle = dms_client_handle;
  config->msg_type = QMI_IDL_INDICATION;
  config->msg_id = 0xffff;
  if(NULL != config->raw_data_buf)
    free(config->raw_data_buf);
  config->raw_data_buf = NULL;
  config->raw_data_len = -1;
  if(NULL != config->data_buf)
    free(config->data_buf);
  config->data_buf = NULL;
  config->data_len = -1;
  return;
}

/*
void init_qmi_nas_msg_config(QMI_Msg_Config *config)
{
	config->user_handle = nas_client_handle;
	config->msg_type = QMI_IDL_INDICATION;
	config->msg_id = 0xffff;
	if(NULL != config->raw_data_buf)
		free(config->raw_data_buf);
	config->raw_data_buf = NULL;
	config->raw_data_len = -1;
	if(NULL != config->data_buf)
		free(config->data_buf);
	config->data_buf = NULL;
	config->data_len = -1;
	return;
}
*/