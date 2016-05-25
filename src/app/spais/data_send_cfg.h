/*
* Project: SPAIS
* Module: data_send_cfg.h
* Author: Cory Perkins
*/
#ifndef APP_TASK_DATA_SEND_CFG_H
#define APP_TASK_DATA_SEND_CFG_H

//=========================== definitions ======================================
#define TASK_APP_DATA_SEND_PRIORITY 55
#define TASK_APP_DATA_SEND_STK_SIZE 256
#define TASK_APP_DATA_SEND_NAME "DataSend"

//======================== public functions ====================================
/*
* dataSendTask - retrieve any publicly posted data and send it to the manager
*/
extern void initializeDataSendTask();

#endif
