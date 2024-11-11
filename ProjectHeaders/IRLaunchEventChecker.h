/****************************************************************************
 Module
    IRLaunchEventChecker.h
 Description
     header file for the IR Launch Sensor event function
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/18/15 11:50 jec      added #include for stdint & stdbool
 08/06/13 14:37 jec      started coding
*****************************************************************************/

#ifndef IRLaunchEventChecker_H
#define IRLaunchEventChecker_H

// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

// function prototypes

void InitIRLaunchSensorStatus(void);

bool CheckIRLaunchEvents(void);

#endif /* IRLaunchEventChecker_H */
