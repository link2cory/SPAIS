#ifndef APP_TASK_DATA_SEND_CFG_H
#define APP_TASK_DATA_SEND_CFG_H

#define TASK_APP_DATA_SEND_PRIORITY 55
#define TASK_APP_DATA_SEND_STK_SIZE 256
#define TASK_APP_DATA_SEND_NAME "DataSend"

// initialize the task.  Only call this once!!!
extern void initializeDataSendTask();

#endif
