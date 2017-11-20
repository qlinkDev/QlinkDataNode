#include <string.h> // memset, memcpy
#include <stdlib.h> // malloc, free
#include <errno.h>

#include "queue_qlinkdatanode.h"
#include "feature_macro_qlinkdatanode.h"
#include "common_qlinkdatanode.h"


#ifndef FEATURE_ENABLE_QUEUE_qlinkdatanode_C_LOG
#define LOG print_to_null
#endif

/******************************External Variables*******************************/
extern rsmp_control_block_type flow_info;
extern bool isQueued;
extern bool isUimPwrDwn;
extern unsigned int StatInfMsgSn;
extern int StatInfReqNum;


/******************************Global Variables*******************************/
Queue_Node head_of_queue = {
  .next_node = NULL,
  .prev_node = NULL,
  .type = false,
  .request = {-1, NULL, false},
  .last_flow_state = FLOW_STATE_IDLE,
  .last_req_type = REQ_TYPE_IDLE
};

Queue_Node *p_tail_of_queue;

static void init_queue(void)
{
	Queue_Node *next = head_of_queue.next_node;
	
	for(; next != NULL; ){
		Queue_Node *cur = next;
		next = next->next_node;
		free(cur->request.data);
		cur->request.data = NULL;
		free(cur);
	}
	
	p_tail_of_queue = &head_of_queue;
	isQueued = false;
	return;
}

//Description:
//	1. The arg "req" can't be pointer to pointer.
//	2. It is FIFO queue.
//	3. Add node right after p_tail_of_queue. It includes creating node.
//Return Value:
//	Added node pointer.
Queue_Node *add_node(rsmp_transmit_buffer_s_type *req, rsmp_state_s_type state)
{
	Queue_Node *new_node = NULL;

	LOG6("~~~~ add_node start ~~~~\n");

	isQueued = true;
	new_node = (Queue_Node *)malloc(sizeof(Queue_Node));
	if(NULL == new_node){
		LOG("ERROR: malloc() failed. errno=%d.\n", errno);
		return NULL;
	}
	memset((void *)new_node, 0, sizeof(Queue_Node));
	
	new_node->last_flow_state = state;
	switch(state){
		case FLOW_STATE_APDU_COMM:
			new_node->last_req_type = REQ_TYPE_APDU;break;
		case FLOW_STATE_ID_AUTH:
			new_node->last_req_type = REQ_TYPE_ID;break;
		case FLOW_STATE_PWR_DOWN_UPLOAD:
			new_node->last_req_type = REQ_TYPE_PWR_DOWN;break;
		case FLOW_STATE_REMOTE_UIM_DATA_REQ:
			new_node->last_req_type = REQ_TYPE_REMOTE_UIM_DATA;break;
		case FLOW_STATE_STAT_UPLOAD:
			new_node->last_req_type = REQ_TYPE_STATUS;break;
		case FLOW_STATE_IMSI_ACQ:
		case FLOW_STATE_SET_OPER_MODE:
		case FLOW_STATE_RESET_UIM:
		default:{
			LOG("ERROR: Wrong param input. state=%d.\n", state);
			return NULL;
		}
	}
	
	//	StatInfReqNum is for real-time examination of status info msg. So if any status info msg is added
	//into queue, StatInfReqNum would be decreased by 1.
	if(REQ_TYPE_STATUS == new_node->last_req_type){
		StatInfReqNum --;
	}
	
	new_node->next_node = NULL;
	new_node->prev_node = p_tail_of_queue;
	p_tail_of_queue->next_node = new_node;
	new_node->request.size = req->size;
	new_node->request.data = malloc(req->size);
	memset(new_node->request.data, 0, req->size);
	memcpy(new_node->request.data, req->data, req->size);
	if(REQ_TYPE_STATUS == new_node->last_req_type){
		LOG("StatInfMsgSn = %u.\n", new_node->request.StatInfMsgSn);
		new_node->request.StatInfMsgSn = req->StatInfMsgSn;
	}

	p_tail_of_queue = new_node;
	LOG6("~~~~ add_node end ~~~~\n");
	return p_tail_of_queue;
}

//Function:
//	Remove "first" node of queue except for header.
//	"Remove" from queue structure, not clear its memory.
//Return Value:
//	Node pointer of removed node.
Queue_Node *remove_first_node_from_queue(void)
{
	Queue_Node *rm_node = head_of_queue.next_node;

	if(isQueued == false){
		LOG("ERROR: Queue is empty.\n");
		return NULL;
	}
	if(rm_node == NULL){
		isQueued = false;
		LOG("ERROR: isQueued=1, but queue is empty.\n");
		return NULL;
	}
	
	head_of_queue.next_node = rm_node->next_node;
	if(rm_node->next_node == NULL){
		isQueued = false;
		p_tail_of_queue = &head_of_queue;
	}else{
		rm_node->next_node->prev_node = &head_of_queue;
	}

	return rm_node;
}

static Queue_Node *get_first_node_from_queue(void)
{
	Queue_Node *p_node = head_of_queue.next_node;

	if(isQueued == false){
		LOG("ERROR: Queue is empty.\n");
		return NULL;
	}
	if(p_node == NULL){
		isQueued = false;
		LOG("ERROR: isQueued=1, but queue is empty.\n");
		return NULL;
	}

	return p_node;
}

static Queue_Node *remove_from_queue(Queue_Node *rm_node)
{
	Queue_Node *nxt_node = NULL, *cur_node = head_of_queue.next_node;

	if(isQueued == false){
		LOG("ERROR: Queue is empty.\n");
		return NULL;
	}
	if(cur_node == NULL){
		isQueued = false;
		LOG("ERROR: isQueued=1, but no node to remove. cur_node=null.\n");
		return NULL;
	}
	if(rm_node == NULL){
		LOG("ERROR: Wrong param input. rm_node=null.\n");
		return NULL;
	}
	
	for(; cur_node!=NULL; cur_node=cur_node->next_node){
		if(cur_node == rm_node){
			rm_node->prev_node->next_node = rm_node->next_node;
			if(rm_node->next_node != NULL){
				rm_node->next_node->prev_node = rm_node->prev_node;
			}else{
				p_tail_of_queue = rm_node->prev_node;
				if(&head_of_queue == rm_node->prev_node)
					isQueued = false;
			}
			break;
		}
	}
	
	return nxt_node;
}

//Function:
//	Clear memory of node.
//NOTE:
//	Not allow to call remove_node() before calling remove_first_node_from_queue() or remove_from_queue!!!
void remove_node(Queue_Node *node)
{
	if (node == NULL){
//		LOG("Wrong param input. node=null.\n");
		return;
	}

	free(node->request.data);
	free(node);
	return;
}

static rsmp_protocol_type queue_check_node_req_type(void)
{
	Queue_Node *p_first_node = get_first_node_from_queue();
	
	return p_first_node->last_req_type;
}

//Description:
//	Check if queued node is valid(Not out-of-date).
//Return Values:
//	1: Valid.
//	0: Invalid.
int queue_check_if_node_valid(void)
{
	rsmp_protocol_type node_req_type = queue_check_node_req_type();
	int ret = 1;
		
	if(REQ_TYPE_APDU == node_req_type 
		&& (true == isUimPwrDwn || FLOW_STATE_RESET_UIM == flow_info.flow_state)){
		LOG("The cached data is out-of-date APDU!\n");
		ret = 0;
	}
	
	return ret;
}

//Description:
//Clr specific-type nodes in queue.
//Return Values:
//The num of clred nodes.
static int queue_clr_node(rsmp_protocol_type req_t)
{
	Queue_Node *cur_node = head_of_queue.next_node;
	int count = 0;
	
	if(NULL == cur_node){
		return 0;
	}
	
	for(; cur_node != NULL;){
		if(req_t == cur_node->last_req_type){
			Queue_Node *rm_node = cur_node;
			
			cur_node = cur_node->next_node;
			remove_from_queue(rm_node);
			remove_node(rm_node);
			count ++;
			continue;
		}
		
		cur_node = cur_node->next_node;
	}
	
	return count;
}

void queue_clr_apdu_node(void)
{
	int clr_num = queue_clr_node(REQ_TYPE_APDU);
	
	LOG("Clear %d APDU node(s) in total.\n", clr_num);
	return;
}

void queue_clr_stat_inf_node(void)
{
	int clr_num = queue_clr_node(REQ_TYPE_STATUS);
	
	LOG("Clear %d status info node(s) in total.\n", clr_num);
	return;
}

int startQueue(void)
{
#ifdef FEATURE_ENABLE_QUEUE_SYSTEM
  init_queue();
#endif
  return 1;
}
