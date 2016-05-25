/*
* Project: SPAIS
* Module: valve_control_cfg.h
* Author: Cory Perkins
*/
#ifndef APP_TASK_VALVE_CONTROL_CFG_H
#define APP_TASK_VALVE_CONTROL_CFG_H

//=========================== definitions ======================================
// OS Defines
#define TASK_APP_VALVE_CONTROL_PRIORITY 56
#define TASK_APP_VALVE_CONTROL_STK_SIZE 256
#define TASK_APP_VALVE_CONTROL_NAME "ValveControl"

//============================ typedefs ========================================
typedef enum VALVE_STATUS {
    open,
    closed
} VALVE_STATUS;

//======================== public functions ====================================
/*
* initializeValveControlTask - initialize the valve control task and any
*                              hardware as necessary
*/
extern void initializeValveControlTask();

/*
* postValveControlData - post a valve status change request to the message
*                        queue.  It will be serviced later
*
* @param VALVE_STATUS valve_control_data the desired valve status
*/
extern void postValveControlData(VALVE_STATUS valveControlData);

#endif
