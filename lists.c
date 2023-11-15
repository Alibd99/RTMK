#include "kernel_functions.h"
#include "system_sam3x.h"
#include "at91sam3x8.h"
#include <stdlib.h>
#include "lists.h"


list *ReadyList = NULL;
list *WaitingList = NULL;
list *TimerList = NULL;


list * createlist(void){

  list *new_list;
  new_list = (list*)calloc(1, sizeof(list));
    
    if(new_list == 0){
        free(new_list);
        return NULL;
    }
    else{
  
  new_list->pTail->pPrevious = new_list->pHead;
  new_list->pHead->pNext = new_list->pTail;
  return new_list; 
    }
}


void insert(list * new_list, listobj * obj){
    listobj *current = NULL;
    current = new_list->pHead;

    if(current == NULL){
        new_list->pHead = obj;
        new_list->pTail = obj;
        obj->pNext = NULL;
        obj->pPrevious = NULL;
    }

    else if(new_list->pHead == new_list->pTail){
        if (obj->pTask->Deadline < current->pTask->Deadline){
            new_list->pHead=obj;
            obj->pNext=current;
            current->pPrevious = obj;
        }

        else{
            new_list->pTail = obj;
            current->pNext = obj;
            obj->pPrevious = current;
        }

    }
    else if(current->pTask->Deadline >obj->pTask->Deadline){
        obj->pNext=current;
        new_list->pHead=obj;
        current->pPrevious=obj;

    }
    else if(obj->pTask->Deadline > new_list->pTail->pTask->Deadline){
        new_list->pTail->pNext=obj;
        obj->pPrevious=new_list->pTail;
        new_list->pTail=obj;
    }
    else{

        while ((current->pNext != NULL) &&
               (obj->pTask->Deadline > current->pTask->Deadline))
            current = current->pNext;

        obj->pNext = current;
        obj->pPrevious = current->pPrevious;
        current->pPrevious=obj;
        obj->pPrevious->pNext = obj;
    }
}


listobj * create_listobj(TCB * task){
  
  listobj* new_listobj;
  new_listobj = (listobj*)calloc(1, sizeof(listobj));
  new_listobj->pTask = task;
  new_listobj->nTCnt = 0;
  return new_listobj;
}

listobj * extract_readylist(void){
  listobj * re_obj;

  if (ReadyList->pHead == ReadyList->pTail){
    return NULL;
    
  }
    re_obj = ReadyList->pHead;
    remove_objlist(re_obj);
    return re_obj;
}

void remove_objlist(listobj * temp){
        ReadyList->pHead = temp->pNext;
        ReadyList->pHead->pPrevious = NULL;
}

void insertMessage(mailbox* mBox, msg* message)
{
  if (mBox->pHead == NULL) {
    mBox->pHead = message;
    mBox->pTail = message;
  }
  else {
    message->pPrevious = mBox->pTail;
    mBox->pTail->pNext = message;
    mBox->pTail = message;
  }
  (mBox->nMessages)++;
  
}



void messageRemove(mailbox* mBox, msg* msg ){
    // Box in beginning and one element
    if (mBox->pHead == msg && mBox->pTail == msg) {
        mBox->pHead = NULL;
        mBox->pTail = NULL;
    }
    // Box in beginning with two or more element
    if (mBox->pHead == msg && msg->pNext != NULL){
        mBox->pHead = msg->pNext;
        mBox->pHead->pPrevious = NULL;
    }
    //Box in last
    if (mBox->pTail == msg) {
        mBox->pTail = msg->pPrevious;
        mBox->pTail->pNext = NULL;
    }
    // Box in middle
    if(mBox->pHead != NULL && mBox->pTail != NULL){
        msg->pPrevious->pNext = msg->pNext;
        msg->pNext->pPrevious = msg->pPrevious;
    }
    mBox->nMessages--;
}

void taskRemove(list **list, listobj *node){
  if ((*list)->pHead == NULL || node == NULL)
      return;
  //if node is head
  if ((*list)->pHead == node){
      // List has one element
      if((*list)->pTail == node){
          (*list) -> pHead->pNext = NULL;
          (*list)->pHead->pPrevious = NULL,
          (*list) -> pHead = NULL;
          (*list) -> pTail = (*list)->pHead;

    }
      // List have more element
    else{
        (*list) -> pHead = (*list) -> pHead -> pNext;
        (*list) -> pHead -> pNext -> pPrevious = NULL;

    }
  }
  //if node is tail
  else if((*list)->pTail == node){
      (*list)->pTail-> pNext -> pPrevious = NULL;
      (*list)->pTail = (*list)->pTail ->pPrevious;
  }
  // if list in the middle
  else if (node->pPrevious != NULL && node->pNext != NULL){
    node ->pPrevious ->pNext = node ->pNext;
    node -> pNext -> pPrevious = node -> pPrevious;
  }
  node->pNext = NULL;
  node->pPrevious = NULL;
}


bool isContain(list ** lists, listobj * node){
    list *Empty = (*lists);
    listobj *Object = Empty->pHead;
    while (Object != NULL) {
        if (Object == node) {
            return OK;
        }
        Object = Object->pNext;
    }
    return FAIL;
}



