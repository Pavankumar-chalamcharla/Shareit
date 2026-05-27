//////////////////////////////////////////////////////////////////////////////////////////
//
//    Heart Rate Algorithm - Ported to DA14695 (DA1469x SDK / SmartSnippets Studio)
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//    Original Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
//    ALL RIGHTS RESERVED
//
//    Port changes only:
//      - C++ class replaced with C struct + function declarations
//      - Arduino-specific includes removed (pure math, none needed)
//    All algorithm logic, all constants, all member variables: UNCHANGED.
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef PROTOCENTRAL_HR_ALGORITHM_H
#define PROTOCENTRAL_HR_ALGORITHM_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * C struct replaces C++ class.
 * HeartRate is the only data member in the original class; kept here.
 * ----------------------------------------------------------------------- */
typedef struct {
    uint8_t HeartRate;
} hr_algo;

#ifdef __cplusplus
extern "C" {
#endif

/* All function signatures identical to original class methods */
void          hr_algo_initStatHRM     (hr_algo *self);
void          hr_algo_statHRMAlgo    (hr_algo *self, uint32_t ppgData);
void          hr_algo_updateWindow   (hr_algo *self, uint32_t *peakWindow,
                                      uint32_t Y, uint8_t n);
uint8_t       hr_algo_chooseRate     (hr_algo *self, uint8_t *rate);
void          hr_algo_updateHeartRate(hr_algo *self, uint8_t *rate,
                                      uint32_t freq, uint32_t last);
uint32_t      hr_algo_findMax        (hr_algo *self, uint32_t *X);
uint32_t      hr_algo_findMin        (hr_algo *self, uint32_t *X);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCENTRAL_HR_ALGORITHM_H */
