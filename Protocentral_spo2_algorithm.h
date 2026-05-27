//////////////////////////////////////////////////////////////////////////////////////////
//
//    SpO2 Algorithm - Ported to DA14695 (DA1469x SDK / SmartSnippets Studio)
//
//    Original Copyright (c) 2018 ProtoCentral
//    Port changes only:
//      - Removed Arduino-specific includes (not needed, pure math)
//      - C++ class replaced with C struct + function declarations
//      - min() macro guarded against redefinition (SDK may define it)
//
//    All algorithm constants, buffer sizes, function signatures: UNCHANGED.
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef PROTOCENTRAL_SPO2_ALGORITHM_H
#define PROTOCENTRAL_SPO2_ALGORITHM_H

#include <stdint.h>
#include <stddef.h>

/* Algorithm constants - completely unchanged from original */
#define SF_spo2       25                 /* sampling frequency */
#define BUFFER_SIZE   (SF_spo2 * 4)     /* 100 samples        */
#define MA4_SIZE      4                  /* DONOT CHANGE       */

/* Guard against SDK redefining min() */
#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

/* -----------------------------------------------------------------------
 * C struct replaces C++ class.
 * spo2_algorithm has no member variables in the original - it is stateless.
 * The struct is kept so call sites use the same pattern as the original.
 * ----------------------------------------------------------------------- */
typedef struct {
    int _unused; /* placeholder - original class had no data members */
} spo2_algorithm;

#ifdef __cplusplus
extern "C" {
#endif

/* All function signatures identical to original class methods */
void spo2_algorithm_estimate_spo2    (spo2_algorithm *self,
                                      uint16_t *pun_ir_buffer,
                                      int32_t   n_ir_buffer_length,
                                      uint16_t *pun_red_buffer,
                                      int32_t  *pn_spo2,
                                      int8_t   *pch_spo2_valid,
                                      int32_t  *pn_heart_rate,
                                      int8_t   *pch_hr_valid);

void spo2_algorithm_find_peak        (spo2_algorithm *self,
                                      int32_t *pn_locs,
                                      int32_t *n_npks,
                                      int32_t *pn_x,
                                      int32_t  n_size,
                                      int32_t  n_min_height,
                                      int32_t  n_min_distance,
                                      int32_t  n_max_num);

void spo2_algorithm_find_peak_above  (spo2_algorithm *self,
                                      int32_t *pn_locs,
                                      int32_t *n_npks,
                                      int32_t *pn_x,
                                      int32_t  n_size,
                                      int32_t  n_min_height);

void spo2_algorithm_remove_close_peaks(spo2_algorithm *self,
                                       int32_t *pn_locs,
                                       int32_t *pn_npks,
                                       int32_t *pn_x,
                                       int32_t  n_min_distance);

void spo2_algorithm_sort_ascend      (spo2_algorithm *self,
                                      int32_t *pn_x,
                                      int32_t  n_size);

void spo2_algorithm_sort_indices_descend(spo2_algorithm *self,
                                         int32_t *pn_x,
                                         int32_t *pn_indx,
                                         int32_t  n_size);

#ifdef __cplusplus
}
#endif

#endif /* PROTOCENTRAL_SPO2_ALGORITHM_H */
