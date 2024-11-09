/****************************************************************************
 Module
    PCEventChecker.h
 Description
     header file for the poker chip insertion event function
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/18/15 11:50 jec      added #include for stdint & stdbool
 08/06/13 14:37 jec      started coding
*****************************************************************************/

#ifndef PCEventChecker_H
#define PCEventChecker_H

// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

// function prototypes

void InitPCSensorStatus(void);

bool CheckPCDetectionEvents(void);

#endif /* PCEventChecker_H */
