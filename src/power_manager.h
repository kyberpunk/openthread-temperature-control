/*
 *  Copyright (c) 2020, Vit Holasek
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef POWER_MANAGER_H_
#define POWER_MANAGER_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *   This file defines functions for reading power supply voltage with ADC.
 *
 */

/**
 * Initialize power manager peripherals.
 *
 * @retval NRFX_SUCCESS   Initialization was successful.
 */
nrfx_err_t power_manager_init(void);

/**
 * Measure and read power supply voltage in volts.
 *
 * @param[out]  voltage   A pointer to voltage value to be set.
 *
 * @retval NRFX_SUCCESS   Measurement was successful.
 */
nrfx_err_t power_manager_read_voltage(float *voltage);

/**
 * Uninitialize power manager peripherals.
 *
 */
void power_manager_uninit(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* POWER_MANAGER_H_ */
