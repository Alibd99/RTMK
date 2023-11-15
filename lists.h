#ifndef INCLUDE_LISTS_H
#define INCLUDE_LISTS_H
#include "kernel_functions.h"
#include "system_sam3x.h"
#include "at91sam3x8.h"

extern list *ReadyList;
extern list *WaitingList;
extern list *TimerList;

list* createlist(void);
void insert( list* new_list, listobj* obj);

listobj * create_listobj(TCB * task);
void get_Ready(listobj* object);
listobj * extract_readylist(void);
void remove_objlist(listobj * temp);

void insertMessage(mailbox* box, msg* message);
void messageRemove(mailbox* mBox, msg* msg );
void taskRemove(list **list, listobj *node);
bool isContain(list ** list, listobj *node);







#endif