/*
Author: Cory Perkins
*/
#ifndef APP_TASK_MOISTURE_SENSE_CFG_H
#define APP_TASK_MOISTURE_SENSE_CFG_H

#define TASK_APP_MOISTURE_SAMPLE_NAME    "moistureSample"
#define TASK_APP_MOISTURE_SENSE_PRIORITY 54
#define TASK_APP_MOISTURE_SENSE_STK_SIZE 256
#define TASK_APP_MOISTURE_SENSE_NAME "MoistureSense"

// initialize the task.  Only call this once!!!
extern void initializeMoistureSenseTask();

// obtain the most recent soil moisture data
extern INT16U retrieveMoistureSenseData();

#endif
