#ifndef EVENT_qlinkdatanode_H_INCLUDED
#define EVENT_qlinkdatanode_H_INCLUDED

#include <sys/time.h>
#include <time.h>

#include "common_qlinkdatanode.h"

#define MAX_FD_EVENTS (16)

typedef void (*evt_cb)(void *userdata);

struct _Evt_Tag;
typedef struct _Evt_Tag{
  struct _Evt_Tag *next_evt;
  struct _Evt_Tag *prev_evt;

  int fd;
  int index;//index of watch table
  bool persist;
  struct timeval timeout;
  evt_cb cb_func;
  void *param;
}Evt;

extern void set_evt(Evt *ev, int fd, bool persist, evt_cb func, void * param);
extern void add_evt(Evt *ev, char tag);
extern void remove_evt(Evt *ev);
extern void trigger_EventLoop(void);
extern void add_timer(Evt *ev, struct timeval *tv);

#endif //EVENT_qlinkdatanode_H_INCLUDED
