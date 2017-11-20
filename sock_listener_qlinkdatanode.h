#ifndef SOCK_LISTENER_qlinkdatanode_H_INCLUDED
#define SOCK_LISTENER_qlinkdatanode_H_INCLUDED

#define UNIX_SERV_NAME "/usr/bin/qlinkdatanode_unix_server"
#define MAX_LEN_OF_UNIX_SERV_NAME (sizeof(UNIX_SERV_NAME)) // Include a '\0'

typedef struct {
	int result;
	char serv_name[MAX_LEN_OF_UNIX_SERV_NAME];
}Input_Param_t;

#endif
