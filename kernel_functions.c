#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"
#include "lists.h"

int Ticks;
int KernelMode; 
TCB *PreviousTask, *NextTask;
bool state;

void idle(){

  while(1){}
	
}

void TimerInt(void){
  /*  Increment tick counter*/
  Ticks++;
  
  // Check the TimerList for tasks that are ready for execution 
  // (a task that was sleeping is ready for execution if either its sleep period is over, OR if its deadline has expired) 
  listobj * current = TimerList->pHead;

  while (current != NULL){
      current->nTCnt--;
      if(current->pTask->Deadline <= Ticks || current->nTCnt == 0) {  //OR if its deadline has expired
          listobj * temp = current;
          current = current->pNext;
          // /* move these to ReadyList.*/
          taskRemove(&TimerList, temp);
          PreviousTask = NextTask;
          insert(ReadyList, temp);
          NextTask = ReadyList->pHead->pTask;
      } else
          current = current->pNext;
  }

  listobj * wait_list = WaitingList->pHead;
  while (wait_list != NULL){
      // Check the WaitingList for tasks that have expired deadlines,
      if(wait_list->pTask->Deadline <= Ticks ){
          // Move these to ReadyList.
          listobj * temp2 = wait_list;
          wait_list = wait_list->pNext;
          taskRemove(&WaitingList, temp2);
          
          //temp2->pMessage = NULL;

          PreviousTask = NextTask;

          insert(ReadyList, temp2);
          NextTask = ReadyList->pHead->pTask;
      }
      else
          wait_list = wait_list->pNext;
  }
}


exception init_kernel(void){

  Ticks = 0;
  ReadyList = createlist();
  TimerList = createlist();
  WaitingList = createlist();
  
  if(ReadyList == NULL){
    free(ReadyList);
      return FAIL;
  }
  
  
  if (TimerList == NULL){
    free(ReadyList);
    free(TimerList);
    return FAIL;
  }
  
 
  if (WaitingList == NULL){
    free(ReadyList);
    free(TimerList);
    free(WaitingList);
    return FAIL;
  }
  exception status = create_task(idle, UINT_MAX);
  
  if (status != OK){
    free(ReadyList);
    free(TimerList);
    free(WaitingList);
    return status;
  } 
  
  return OK;
}


exception create_task( void (* task_body)(), uint deadline){
    TCB * new_tcb;
    isr_off();
    new_tcb = (TCB *) calloc (1, sizeof(TCB));

  if (new_tcb == NULL){
    free(new_tcb);
    return FAIL;
  }
  
    new_tcb->PC = task_body;
    new_tcb->SPSR = 0x21000000;
    new_tcb->Deadline = deadline;
    
    new_tcb->StackSeg[STACK_SIZE - 2] = 0x21000000;
    new_tcb->StackSeg[STACK_SIZE - 3] = (uint) task_body;
    new_tcb->SP = &(new_tcb->StackSeg[STACK_SIZE - 9]);
    
    isr_on(); 
    if(KernelMode == INIT){
      insert(ReadyList, create_listobj(new_tcb));
      return OK;
    }
    
    else{
      isr_off();
      PreviousTask = NextTask;
      insert(ReadyList, create_listobj(new_tcb));
      NextTask = ReadyList->pHead->pTask;
      SwitchContext();
    }
    
    return OK;
}


void run( void ){
  Ticks = 0;
  KernelMode = RUNNING;
  NextTask = ReadyList->pHead->pTask;
  LoadContext_In_Run();

}



void terminate( void ){
  isr_off();
  listobj * leaveObj;
  leaveObj =  extract_readylist();

  NextTask = ReadyList->pHead->pTask;
  switch_to_stack_of_next_task();
  
  free(leaveObj->pTask);
  free(leaveObj);
  
  LoadContext_In_Terminate();

}