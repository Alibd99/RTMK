#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"
#include "lists.h"

exception wait(uint nTicks) {
    //Disable interrupt
    isr_off();
    
    //Update PreviousTask
    PreviousTask = NextTask;
    listobj * temp = ReadyList->pHead;
    temp->nTCnt = nTicks;
    //Place running task in the TimerList
    taskRemove(&ReadyList, temp);
    insert(TimerList, temp);
    //Update NextTask
    NextTask = ReadyList->pHead->pTask;
    
    //Switch context
    SwitchContext();
    
    //IF deadline is reached THEN
    if(Ticks >= NextTask->Deadline) {
      
      //Status is DEADLINE_REACHED
      return DEADLINE_REACHED;
    }
    
    //ELSE
    else { 
      //Status is OK
      return OK;
    }//ENDIF

}


void set_ticks( uint nTicks) {
    //Set the tick counter.
  Ticks = nTicks;
}


uint ticks(void) {
    //Return the tick counter
  return Ticks;
}


uint deadline(void){
  //Return the deadline of the current task
  return NextTask->Deadline;
}


void set_deadline(uint deadline) {
  //  Disable interrupt
  isr_off();
  
  //Set the deadline field in the calling TCB.
  NextTask->Deadline = deadline;
  
  //Update PreviousTask
  PreviousTask = NextTask; 
  
  //Reschedule ReadyList
  taskRemove(&ReadyList, ReadyList->pHead);
  insert(ReadyList, ReadyList->pHead);
  
  //Update NextTask
  NextTask = ReadyList->pHead->pTask;
   
  //Switch context
  SwitchContext();
}
