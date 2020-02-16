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

#include <stdbool.h>

#include "nrfx_saadc.h"
#include "nrf_drivers_common.h"

/**
 * @file
 *   This file contains implementation of power manager for measuring power supply voltage.
 *   SAADC is used for measuring voltage.
 *
 */

#define POWER_MANAGER_R1 1000
#define POWER_MANAGER_R2 180
#define POWER_MANAGER_GAIN 1.0
#define POWER_MANAGER_PRECISION 10
#define POWER_MANAGER_VREF_MV 600

static bool sampled = false;
static nrf_saadc_value_t adc_values_pool[2];
static nrf_saadc_value_t adc_value = 0;

static void power_manager_saadc_callback(nrfx_saadc_evt_t const * p_event)
{
    if (p_event->type == NRFX_SAADC_EVT_DONE)
    {
        nrfx_err_t error;

        adc_value = *p_event->data.done.p_buffer;
        error = nrfx_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        if (error != NRFX_SUCCESS)
        {
            adc_value = 0;
        }
        sampled = true;
    }
}

nrfx_err_t power_manager_init(void)
{
    nrfx_err_t error;
    nrfx_saadc_config_t config = NRFX_SAADC_DEFAULT_CONFIG;
    EXIT_ON_ERROR(error = nrfx_saadc_init(&config, power_manager_saadc_callback));
    nrf_saadc_channel_config_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);
    // Folowing line is used for configuring defferential ADC measurement in case of low stability
    //nrf_saadc_channel_config_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_DIFFERENTIAL(NRF_SAADC_INPUT_AIN0, NRF_SAADC_INPUT_AIN5);
    channel_config.gain = NRF_SAADC_GAIN1;
    channel_config.acq_time = NRF_SAADC_ACQTIME_10US;
    EXIT_ON_ERROR(error = nrfx_saadc_channel_init(0, &channel_config));
    EXIT_ON_ERROR(error = nrfx_saadc_buffer_convert(&adc_values_pool[0], 1));
    EXIT_ON_ERROR(error = nrfx_saadc_buffer_convert(&adc_values_pool[1], 1));
    sampled = false;
    return NRFX_SUCCESS;

exit:
    nrfx_saadc_uninit();
    return error;
}

float power_manager_calculate_voltage(uint16_t value)
{
    int32_t bitvalue = (((int32_t)value) * POWER_MANAGER_VREF_MV) >> POWER_MANAGER_PRECISION;
    // This line is used for calculation in case of differential ADC measurement
    //int32_t bitvalue = (((int32_t)value) * POWER_MANAGER_VREF_MV) >> (POWER_MANAGER_PRECISION - 1);
    float u2 = ((float)bitvalue) / (POWER_MANAGER_GAIN * 1000);
    return u2 * ((((float)POWER_MANAGER_R1) + POWER_MANAGER_R2) / POWER_MANAGER_R2);
}

nrfx_err_t power_manager_read_voltage(float *voltage)
{
    nrfx_err_t error;
    RETURN_ON_ERROR(error = nrfx_saadc_sample());
    // Wait for measurement being sampled
    while (!sampled) { }
    sampled = false;
    *voltage = 0;
    if (adc_value != 0)
    {
        *voltage = power_manager_calculate_voltage(adc_value);
    }
    return NRFX_SUCCESS;
}

void power_manager_uninit(void)
{
    nrfx_saadc_channel_uninit(0);
    nrfx_saadc_uninit();
}

