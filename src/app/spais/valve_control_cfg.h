/*
Author: Cory Perkins
*/
#ifndef APP_TASK_VALVE_CONTROL_CFG_H
#define APP_TASK_VALVE_CONTROL_CFG_H

#define TASK_APP_VALVE_CONTROL_PRIORITY 56
#define TASK_APP_VALVE_CONTROL_STK_SIZE 256
#define TASK_APP_VALVE_CONTROL_NAME "ValveControl"

// initialize the task.  Only call this once!!!
extern void initializeValveControlTask();

#endif
