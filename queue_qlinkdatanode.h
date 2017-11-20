#ifndef QUEUE_qlinkdatanode_H_INCLUDED
#define QUEUE_qlinkdatanode_H_INCLUDED

#include "common_qlinkdatanode.h"
#include "protocol_qlinkdatanode.h"

struct _Queue_Node_Tag;

typedef struct _Queue_Node_Tag{
  struct _Queue_Node_Tag *next_node;
  struct _Queue_Node_Tag *prev_node;

  //Description:
  //true: emergency event; false: following event
  bool type;

  rsmp_transmit_buffer_s_type request;

  rsmp_state_s_type last_flow_state;
  rsmp_protocol_type last_req_type;
} Queue_Node;

extern Queue_Node *add_node(rsmp_transmit_buffer_s_type *req, rsmp_state_s_type state);
extern void remove_node(Queue_Node *node);
extern Queue_Node *remove_first_node_from_queue(void);
extern int startQueue(void);

#endif // QUEUE_qlinkdatanode_H_INCLUDED
