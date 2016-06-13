/*
* Project: SPAIS
* Module: moisture_sense_cfg.h
* Author: Cory Perkins
*/
#ifndef APP_TASK_MOISTURE_SENSE_CFG_H
#define APP_TASK_MOISTURE_SENSE_CFG_H

//=========================== definitions ======================================
#define TASK_APP_MOISTURE_SAMPLE_NAME    "moistureSample"
#define TASK_APP_MOISTURE_SENSE_PRIORITY 54
#define TASK_APP_MOISTURE_SENSE_STK_SIZE 256
#define TASK_APP_MOISTURE_SENSE_NAME "MoistureSense"

//======================== public functions ====================================
/*
* initializeMoistureSenseTask - responsible for all initialization of the
*                               moistureSenseTask
*/
extern void initializeMoistureSenseTask(OS_EVENT* joined_sem);

/*
* retrieveMoistureSenseData - retrieve the latest moisture data
*                             Caution: This function PENDS until there is data!
*/
extern INT16U retrieveMoistureSenseData();

#endif
