/*
Author: Cory Perkins
*/
#ifndef APP_TASK_VALVE_CONTROL_CFG_H
#define APP_TASK_VALVE_CONTROL_CFG_H

// OS Defines
#define TASK_APP_VALVE_CONTROL_PRIORITY 56
#define TASK_APP_VALVE_CONTROL_STK_SIZE 256
#define TASK_APP_VALVE_CONTROL_NAME "ValveControl"

// Network Interface Defins
#define VALVE_CONTROL_KEY 01

// initialize the task.  Only call this once!!!
extern void initializeValveControlTask();
extern void postValveControlData(INT8U valveControlData);

#endif
