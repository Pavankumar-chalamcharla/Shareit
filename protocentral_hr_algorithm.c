//////////////////////////////////////////////////////////////////////////////////////////
//
//    Heart Rate Algorithm - Ported to DA14695 (DA1469x SDK / SmartSnippets Studio)
//
//    Original Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
//    ALL RIGHTS RESERVED
//
//    Port changes only:
//      - C++ class scope (hr_algo::) replaced with C function prefix + self pointer
//      - Arduino-specific includes removed (none were actually used in this file)
//      - unsigned long -> uint32_t, unsigned int -> uint32_t, unsigned char -> uint8_t
//        (these are identical in width on ARM Cortex-M33, change is for portability)
//    ALL algorithm logic, ALL variable names, ALL timing constants: UNCHANGED.
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
/////////////////////////////////////////////////////////////////////////////////////////

#include "protocentral_hr_algorithm.h"

/* -----------------------------------------------------------------------
 * Module-level state variables - identical to original globals
 * unsigned long -> uint32_t, unsigned int -> uint32_t (same on ARM)
 * unsigned char -> uint8_t (same on ARM)
 * ----------------------------------------------------------------------- */
static uint32_t peakWindowHP[21];
static uint32_t lastOnsetValueLED1;
static uint32_t lastPeakValueLED1;
static uint8_t  HR[12];
static uint8_t  temp_val;          /* 'temp' renamed to avoid C reserved-word conflict */
static uint32_t lastPeak;
static uint32_t lastOnset;
static uint32_t movingWindowHP;
static uint8_t  ispeak          = 0;
static uint8_t  movingWindowCount;
static uint8_t  movingWindowSize;
static uint8_t  smallest;
static uint8_t  foundPeak;
static uint32_t totalFoundPeak;
static uint32_t freq;
static uint32_t currentRatio    = 0;

/* -----------------------------------------------------------------------
 * initStatHRM()
 * Logic: UNCHANGED
 * ----------------------------------------------------------------------- */
void hr_algo_initStatHRM(hr_algo *self)
{
    uint8_t i;

    /* Init HR variables */
    lastPeak          = 0;
    lastOnset         = 0;
    movingWindowHP    = 0;
    movingWindowCount = 0;

    for (i = 20; i >= 1; i--)
        peakWindowHP[(uint8_t)(i - 1)] = 0;

    for (i = 12; i >= 1; i--)
        HR[(uint8_t)(i - 1)] = 0;

    /* Sampling frequency */
    freq = 125;
    /* Moving average window size (removes high frequency noise) */
    movingWindowSize = (uint8_t)(freq / 50);
    /* Length of the shortest pulse possible */
    smallest = (uint8_t)(freq * 60 / 220);
    foundPeak      = 0;
    totalFoundPeak = 0;
    self->HeartRate = 0;
}

/* -----------------------------------------------------------------------
 * statHRMAlgo()
 * Logic: UNCHANGED - every branch, every counter, every threshold identical
 * ----------------------------------------------------------------------- */
void hr_algo_statHRMAlgo(hr_algo *self, uint32_t ppgData)
{
    uint8_t i;

    /* moving average calculation */
    movingWindowHP += ppgData;

    if (movingWindowCount > movingWindowSize)
    {
        /* Data processing */
        movingWindowCount = 0;

        /* update data buffer */
        hr_algo_updateWindow(self, peakWindowHP, movingWindowHP,
                             (uint8_t)(movingWindowSize + 1));

        /* reset moving average */
        movingWindowHP = 0;
        ispeak = 0;

        if (lastPeak > smallest)
        {
            /* looking for a local maximum using the 20 point buffer */
            ispeak = 1;
            for (i = 10; i >= 1; i--)
            {
                if (peakWindowHP[10] < peakWindowHP[(uint32_t)(10 - i)])
                    ispeak = 0;
                if (peakWindowHP[10] < peakWindowHP[(uint32_t)(10 + i)])
                    ispeak = 0;
            }

            if (ispeak == 1)
            {
                /* if we have a local maximum */
                /* values for SPO2 ratio */
                lastPeakValueLED1 = hr_algo_findMax(self, peakWindowHP);
                totalFoundPeak++;

                if (totalFoundPeak > 2)
                {
                    /* Update the HR and SPO2 buffer */
                    hr_algo_updateHeartRate(self, HR, freq, lastPeak);
                }
                ispeak   = 1;
                lastPeak = 0;
                foundPeak++;
            }
        }

        if ((lastOnset > smallest) && (ispeak == 0))
        {
            /* looking for a local minimum using the 20 point buffer */
            ispeak = 1;
            for (i = 10; i >= 1; i--)
            {
                if (peakWindowHP[10] > peakWindowHP[(uint32_t)(10 - i)])
                    ispeak = 0;
                if (peakWindowHP[10] > peakWindowHP[(uint32_t)(10 + i)])
                    ispeak = 0;
            }

            /* if we have a local minimum */
            if (ispeak == 1)
            {
                /* values for SPO2 ratio */
                lastOnsetValueLED1 = hr_algo_findMin(self, peakWindowHP);
                totalFoundPeak++;

                if (totalFoundPeak > 2)
                {
                    /* Update the HR and SPO2 buffer */
                    /* currentRatio = updateSPO2(SPO2, lastOnsetValueIR, lastOnsetValueRed,
                     *                           lastPeakValueIR, lastPeakValueRed); */

                    /* If you wanted to run an auto calibration here is the ratio that should be used */
                    /* AutoCalibrate = peakRed / onsetRed; */
                    /* AutoCalibrate ratio should be greater than 1-2% if not you need to increase */
                    /* the LED current or adjust the settings */
                }
                lastOnset = 0;
                foundPeak++;
            }
        }

        if (foundPeak > 2)
        {
            /* Every 4 new peaks update return values */
            foundPeak = 0;
            temp_val  = hr_algo_chooseRate(self, HR);
            if ((temp_val > 40) && (temp_val < 220))
                self->HeartRate = temp_val;
        }
    }

    movingWindowCount++;
    lastOnset++;
    lastPeak++;
}

/* -----------------------------------------------------------------------
 * updateWindow()
 * Logic: UNCHANGED
 * ----------------------------------------------------------------------- */
void hr_algo_updateWindow(hr_algo *self, uint32_t *peakWindow, uint32_t Y, uint8_t n)
{
    /* Moving average buffer for LED data */
    uint8_t i;
    for (i = 20; i >= 1; i--)
    {
        peakWindow[i] = peakWindow[(uint8_t)(i - 1)];
    }
    if (n > 0)
    {
        peakWindow[0] = (Y / n);
    }
}

/* -----------------------------------------------------------------------
 * chooseRate()
 * Logic: UNCHANGED - trimmed mean calculation identical to original
 * ----------------------------------------------------------------------- */
uint8_t hr_algo_chooseRate(hr_algo *self, uint8_t *rate)
{
    /* Returns the average rate, after removing the lowest and highest values
     * (based on the number of found HR removing 2-4-6 values). */
    uint8_t  max_val, min_val, i, nb;
    uint32_t sum, fullsum;

    max_val = rate[0];
    min_val = rate[0];
    sum     = 0;
    nb      = 0;

    for (i = 7; i >= 1; i--)
    {
        if (rate[(uint32_t)(i - 1)] > 0)
        {
            if (rate[(uint32_t)(i - 1)] > max_val)
            {
                max_val = rate[(uint32_t)(i - 1)];
            }
            if (rate[(uint32_t)(i - 1)] < min_val)
            {
                min_val = rate[(uint32_t)(i - 1)];
            }
            sum += rate[(uint32_t)(i - 1)];
            nb++;
        }
    }

    if (nb > 2)
    {
        fullsum = (uint32_t)((sum - max_val - min_val) * 10 / (nb - 2));
    }
    else if (nb > 0)
    {
        fullsum = (uint32_t)((sum) * 10 / (nb));
    }
    else
    {
        fullsum = 0;
    }

    sum = fullsum / 10;

    if (fullsum - sum * 10 > 4)
        sum++;

    return (uint8_t)sum;
}

/* -----------------------------------------------------------------------
 * updateHeartRate()
 * Logic: UNCHANGED
 * ----------------------------------------------------------------------- */
void hr_algo_updateHeartRate(hr_algo *self, uint8_t *rate, uint32_t freq_val, uint32_t last)
{
    /* Adds a new Heart rate into the array and loses the oldest */
    uint8_t i;
    i = (uint8_t)(60 * freq_val / last);
    if ((i > 40) && (i < 220))
    {
        for (i = 11; i >= 1; i--)
        {
            rate[i] = rate[(uint8_t)(i - 1)];
        }
        rate[0] = (uint8_t)(60 * freq_val / last);
    }
}

/* -----------------------------------------------------------------------
 * findMax()
 * Logic: UNCHANGED - searches indices 8-12 of buffer
 * ----------------------------------------------------------------------- */
uint32_t hr_algo_findMax(hr_algo *self, uint32_t *X)
{
    /* Finds the maximum around the center of the buffer */
    uint32_t res = X[8];
    uint8_t  i;
    for (i = 12; i >= 9; i--)
    {
        if (res < X[i])
            res = X[i];
    }
    return res;
}

/* -----------------------------------------------------------------------
 * findMin()
 * Logic: UNCHANGED - searches indices 8-12 of buffer
 * ----------------------------------------------------------------------- */
uint32_t hr_algo_findMin(hr_algo *self, uint32_t *X)
{
    /* Finds the minimum around the center of the buffer */
    uint32_t res = X[8];
    uint8_t  i;
    for (i = 12; i >= 9; i--)
    {
        if (res > X[i])
            res = X[i];
    }
    return res;
}
