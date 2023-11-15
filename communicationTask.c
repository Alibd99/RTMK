#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"
#include "lists.h"


mailbox* create_mailbox(uint nMessages, uint nDataSize ){
    //Allocate memory for the mailbox
    mailbox* mailBox = (mailbox*)malloc(sizeof(mailbox));
    if (mailBox == NULL){
      free(mailBox);
      return NULL;
    }
    else{
      //Initialize mailbox structure
      mailBox->nMaxMessages = nMessages;
      mailBox->nDataSize = nDataSize;
      mailBox->nMessages = 0;
      mailBox->nBlockedMsg = 0;
      mailBox->pHead = NULL;
      mailBox->pTail = NULL;
      
      //Return mailbox*
      return mailBox;;
    }
}


int no_messages(mailbox* mBox){
  if (mBox == NULL){
    return -1;
  }
  return mBox->nMessages;
}


exception remove_mailbox(mailbox* mBox){
  if(!(no_messages(mBox))){
    free(mBox);
      return OK;  
  } else
      return NOT_EMPTY;
}


exception send_wait( mailbox* mBox, void* pData ){
    //Disable interrupt 
    isr_off();
    msg* reTask = mBox->pHead;
    //IF receiving task is waiting THEN
    if(reTask->Status == RECEIVER){
        
        //Copy sender�s data to the data area of the receivers Message
        memcpy(reTask->pData, pData, mBox->nDataSize);
        
        //Remove receiving task�s Message struct from the mailbox
        messageRemove(mBox,reTask);
        
        //Update PreviousTask
        PreviousTask = NextTask;
        
        //taskRemove(&WaitingList, reTask->pBlock);
        //Insert receiving task in ReadyList
        insert(ReadyList, reTask->pBlock);
        
        //Update NextTask
        NextTask = ReadyList->pHead->pTask;
    }
    else {
      
        //Allocate a Message structure 
        msg* message = (msg*) malloc (sizeof(msg)); 
        if (message == NULL){
          return FAIL;
        }
        
        message->pData = (char*) malloc (mBox->nDataSize);
        if(message->pData == NULL){
          free(message);
          return FAIL;
        }
        message->Status = SENDER;     
        message->pNext = NULL;
        message ->pPrevious = NULL;
        ReadyList->pHead->pMessage=message;
        message->pBlock = ReadyList->pHead;
        
        //Set data pointer
        memcpy(message->pData, pData, mBox->nDataSize);
        
        //Add Message to the mailbox
        insertMessage(mBox, message);
        
        //Update PreviousTask
        PreviousTask = NextTask;
        
        //Move sending task from ReadyList to WaitingList
        taskRemove(&ReadyList, message->pBlock);
        insert(WaitingList, message->pBlock);
        
        //Update NextTask
        NextTask = ReadyList->pHead->pTask;
    }
    
    //Switch context
    SwitchContext();
    
    //IF deadline is reached THEN;
    if(Ticks >= NextTask->Deadline ){
      //Disable interrupt
      isr_off();
      
      //Remove send Message 
      listobj * temp = ReadyList->pHead;
      messageRemove(mBox, ReadyList->pHead->pMessage );
      free(temp->pMessage->pData);
      free(temp->pMessage);
      
      //Enable interrupt
      isr_on();
      return DEADLINE_REACHED;
    }
    else { return OK; }
}



exception receive_wait( mailbox* mBox, void* pData ){
  //Disable interrupt
  isr_off();
  //IF send Message is waiting THEN
  msg* sendTask = mBox->pHead;
  if(sendTask->Status == SENDER){
    
    //Copy sender�s data to receiving task�s data area
    memcpy(pData, sendTask->pData, mBox->nDataSize);
    
    //Remove sending task�s Message struct from the mailbox
    messageRemove(mBox, sendTask);
  	
    //IF Message was of wait type THEN
    if(isContain(&WaitingList, sendTask->pBlock)) {
          
        //Update PreviousTask 
        PreviousTask = NextTask;
        
        //Move sending task to ReadyList
        taskRemove(&WaitingList, sendTask->pBlock);
        insert(ReadyList, sendTask->pBlock);
        
        //Update NextTask
        NextTask = ReadyList->pHead->pTask;
        }
        else {
          //Free senders data area
          free(sendTask->pData);
        }
  }
  else {
    //Allocate a Message structure
    msg* message = (msg*)malloc(sizeof(msg));      
    if (message == NULL){
      return FAIL;
    }
    message->pData = pData;
    message->Status = RECEIVER;
    
    message->pNext = NULL;
    message->pPrevious = NULL;
    
    ReadyList->pHead->pMessage = message;
    
    //Add Message to the mailbox
    insertMessage(mBox, message);
    
    //Update PreviousTask
    PreviousTask = NextTask;
    
    //Move receiving task from ReadyList to WaitingList 
    taskRemove(&ReadyList, message->pBlock);
    insert(WaitingList, message->pBlock);
    
    //Update NextTask
    NextTask = ReadyList->pHead->pTask;
  }
  
  //Switch context
  SwitchContext();
  
  //If the deadline is reched, remove the reciving message
  if(NextTask->Deadline <= Ticks)  {
    
    //Disable interrupt
    isr_off();
    
    //Remove receive Message
    listobj * temp = ReadyList->pHead;
    messageRemove(mBox, temp->pMessage );
      free(temp->pMessage->pData);
      free(temp->pMessage);
    //Enable interrupt
    isr_on();
    return DEADLINE_REACHED; 
  }
  else
    return OK;
}



exception send_no_wait( mailbox* mBox, void* pData ){
  //Disable interrupt
  isr_off();
  
  //IF receiving task is waiting THEN
  msg* recievie = mBox->pHead;
  if(recievie->Status== RECEIVER) {
    
    //Copy data to receiving tasks� data area.
    memcpy(recievie->pData, pData , mBox->nDataSize);
    
    //Remove receiving task�s Message struct from the mailbox
    messageRemove(mBox, recievie); 
    
    //Update PreviousTask
    PreviousTask = NextTask;
    
    //Move receiving task to Readylist
    taskRemove(&WaitingList, recievie->pBlock);
    insert(ReadyList, recievie->pBlock);       
    
    //Update NextTask
    NextTask = ReadyList->pHead->pTask;
    
    //Switch context
    SwitchContext();
  }
  else {
    //Allocate a Message structure
    msg* message = (msg*)calloc(1, sizeof(msg));
    if (message == NULL){
      return FAIL;
    }
    message->pData =(char*)malloc(mBox->nDataSize);
    message->Status = SENDER;
    
    //Copy Data to the Message
    memcpy(message->pData, pData, mBox->nDataSize);
    
    //IF mailbox is full THEN
    if(mBox->nMessages >= mBox->nMaxMessages) {
      //Remove the oldest Message struct
      msg *temp = mBox->pHead;
      messageRemove(mBox, temp);
    }
    
    //Add Message to the mailbox
    insertMessage(mBox, message);
  }
  //Return status
  return OK;                                                             
}



int receive_no_wait( mailbox* mBox, void* pData ){
  //Disable interrupt
  isr_off();
  
  //IF send Message is waiting THEN
  msg* sending = mBox->pHead;
  if(sending->Status == SENDER) {
      
      //Copy sender�s data to receiving task�s data area
      memcpy(pData, sending->pData, mBox->nDataSize);   //Copy senders data to reciving data area

      //Remove sending task�s Message struct from the mailbox
      messageRemove(mBox, sending);

      //IF Message was of wait type THEN
      if(isContain(&WaitingList, sending->pBlock)) {
        
        //Update PreviousTask
        PreviousTask = NextTask;

        //Move sending task to ReadyList
        taskRemove(&WaitingList, sending->pBlock);
        insert(ReadyList, sending->pBlock);
        
        //Update NextTask
        NextTask = ReadyList->pHead->pTask;
        
        //SwitchContext
        SwitchContext();
 
      }
      else
        //Free sender's data area
        free(sending->pData);
      }
  else { 
    //Return FAIL. 
    return FAIL;
  }
  //Return status on received Message
  return OK;         
}
