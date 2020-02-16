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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "openthread/instance.h"
#include "openthread/thread.h"
#include "openthread/tasklet.h"
#include "openthread/ip6.h"
#include "openthread/mqttsn.h"
#include "openthread/dataset.h"
#include "openthread/link.h"
#include "openthread-system.h"

#include "power_manager.h"
#include "nrf_pwr_mgmt.h"
#include "nrfx_rtc.h"
#include "driver_htu21d.h"
#include "nrf_gpio.h"

/**
 * @file
 *   This file contains implementation of periodical measurement of temperature, humidity
 *   and power supply voltage and sending measured data over MQTT-SN with UDP transport
 *   layer over Thread network.
 *
 */

// Static Thread connection settings are used for simplification. Joiner commissioning should be used instead.
// Thread network name
#define NETWORK_NAME "OTBR4444"
// Thread PANID
#define PANID 0x4444
// Thread extended PANID
#define EXTPANID {0x33, 0x33, 0x33, 0x33, 0x44, 0x44, 0x44, 0x44}
// Thread channel
#define DEFAULT_CHANNEL 15
// Thread master key
#define MASTER_KEY {0x33, 0x33, 0x44, 0x44, 0x33, 0x33, 0x44, 0x44, 0x33, 0x33, 0x44, 0x44, 0x33, 0x33, 0x44, 0x44}

// Static MQTT-SN gateway connection settings. Broadcast SEARCHGW request is not possible in MTD.
// MQTT-SN gateway UDP port
#define GATEWAY_PORT 10000
// MQTT-SN gateway static address with 2018:ff9b NAT64 prefix.
#define GATEWAY_ADDRESS "2018:ff9b::ac1c:168"

// MQTT-SN client ID
#define CLIENT_ID "SENSOR2"
// Client UDP listener port
#define CLIENT_PORT 10000
// Predefined topic ID of topic for measured temperature and humidity
#define PREDEFINED_SENSOR_TOPIC_ID 1
// Predefined topic ID of topic for telemetry messages
#define PREDEFINED_TELEMETRY_TOPIC_ID 2

// Measurement period in seconds. Device is in MQTT-SN sleep mode during the cycle-
#define SLEEP_DURATION 60
#define AWAKE_TIMEOUT_MS 2000
#define SHORT_POLLING_CYCLE 10
#define RTC_PRESCALER 4095

static const uint8_t sExpanId[] = EXTPANID;
static const uint8_t sMasterKey[] = MASTER_KEY;

static nrfx_twim_t twi = NRFX_TWIM_INSTANCE(0);
static driver_htu21d_t htu21d = HTU21D_INSTANCE(&twi);

static bool sleeping = false;
static nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(1);
static uint32_t compare_countertime = 0;

static otInstance *instance;
static otMqttsnTopic sensor_topic;
static otMqttsnTopic telemetry_topic;
#define SENSOR_MESSAGE_FORMAT "{\"id\":\"%s\",\"temperature\":%f,\"humidity\":%f,\"dewpoint\":%f}"
#define SENSOR_MESSAGE_SIZE 100
#define TELEMETRY_MESSAGE_FORMAT "{\"id\":\"%s\",\"battery\":%f}"
#define TELEMETRY_MESSAGE_SIZE 50

static void IncrementCountertime(void)
{
    compare_countertime += SLEEP_DURATION * (RTC_INPUT_FREQ / RTC_PRESCALER);
}

/**
 * Measure temperature and humidity data and send results to the measurement topic.
 */
static otError SendMeasurement(otInstance *instance)
{
    otError error;
    uint8_t buf[SENSOR_MESSAGE_SIZE];
    float temperature = 0;
    float humidity = 0;
    float dew_point = 0;

    driver_htu21d_get_hum_hold(&htu21d, &humidity);
    driver_htu21d_get_temp_hold(&htu21d, &temperature);
    dew_point = driver_htu21d_calc_dew_point(temperature, humidity);
    int32_t size = snprintf((char *)buf, SENSOR_MESSAGE_SIZE, SENSOR_MESSAGE_FORMAT, CLIENT_ID, temperature, humidity, dew_point);
    error = otMqttsnPublish(instance, buf, size, kQos0, false, &sensor_topic, NULL, NULL);
    return error;
}

/**
 * Measure power supply voltage and send message to the telemetry topic.
 */
static otError SendBatteryLevel(otInstance *instance)
{
    otError error;
    float voltage = 0;
    uint8_t buf[TELEMETRY_MESSAGE_SIZE];
    power_manager_read_voltage(&voltage);
    int32_t size = snprintf((char *)buf, TELEMETRY_MESSAGE_SIZE, TELEMETRY_MESSAGE_FORMAT, CLIENT_ID, voltage);
    error = otMqttsnPublish(instance, buf, size, kQos0, false, &telemetry_topic, NULL, NULL);
    return error;
}

static void WakeUp()
{
    otLinkSetPollPeriod(instance, SHORT_POLLING_CYCLE);
    otMqttsnAwake(instance, AWAKE_TIMEOUT_MS);
}

static void HandleConnected(otMqttsnReturnCode aCode, void* aContext)
{
    otInstance *instance = (otInstance *)aContext;
    // Handle connected
    if (aCode != kCodeAccepted)
    {
        return;
    }
    otMqttsnSleep(instance, SLEEP_DURATION + 10);
    sleeping = true;
}

static void Connect(otInstance *instance)
{
    otIp6Address address;
    otMqttsnConfig config;
    otIp6AddressFromString(GATEWAY_ADDRESS, &address);
    config.mClientId = CLIENT_ID;
    config.mKeepAlive = 120;
    config.mCleanSession = true;
    config.mPort = GATEWAY_PORT;
    config.mAddress = &address;
    config.mRetransmissionCount = 2;
    config.mRetransmissionTimeout = 3;
    otMqttsnConnect(instance, &config);
}

static void HandleDisconnected(otMqttsnDisconnectType aType, void* aContext)
{
    otInstance *instance = (otInstance *)aContext;
    // Handle event when MQTT-SN client going to sleep mode
    if (aType == kDisconnectAsleep)
    {
        otLinkSetPollPeriod(instance, SLEEP_DURATION * 1000);
        sleeping = true;
        // Send measurements
        SendMeasurement(instance);
        SendBatteryLevel(instance);
    }
}

static void StateChanged(otChangedFlags aFlags, void *aContext)
{
    otInstance *instance = (otInstance *)aContext;
    if (aFlags & OT_CHANGED_THREAD_ROLE)
    {
        otDeviceRole role = otThreadGetDeviceRole(instance);
        // Only child role is relevant for MTD
        if (role == OT_DEVICE_ROLE_CHILD)
        {
            if (otMqttsnGetState(instance) == kStateDisconnected)
            {
                Connect(instance);
            }
        }
    }
}

void ThreadInit(otInstance *instance)
{
    otExtendedPanId extendedPanid;
    otMasterKey masterKey;

    // Set predefined network settings
    otThreadSetNetworkName(instance, NETWORK_NAME);
    memcpy(extendedPanid.m8, sExpanId, sizeof(sExpanId));
    otThreadSetExtendedPanId(instance, &extendedPanid);
    otLinkSetPanId(instance, PANID);
    otLinkSetChannel(instance, DEFAULT_CHANNEL);
    memcpy(masterKey.m8, sMasterKey, sizeof(sMasterKey));
    otThreadSetMasterKey(instance, &masterKey);
    otSetStateChangedCallback(instance, StateChanged, instance);
    otIp6SetSlaacEnabled(instance, true);
    otLinkModeConfig mode;
    mode.mDeviceType = false;
    mode.mNetworkData = false;
    mode.mRxOnWhenIdle = false;
    mode.mSecureDataRequests = true;
    otThreadSetLinkMode(instance, mode);
    otLinkSetPollPeriod(instance, SHORT_POLLING_CYCLE);
//  otThreadSetChildTimeout(instance, SLEEP_DURATION_MS);
    otIp6SetEnabled(instance, true);
    otThreadSetEnabled(instance, true);
}

static void RtcHandler(nrfx_rtc_int_type_t int_type)
{
    if (int_type == NRFX_RTC_INT_COMPARE0)
    {
        sleeping = false;
        IncrementCountertime();
        nrfx_rtc_cc_set(&rtc, 0, compare_countertime, true);
    }
}

static void RtcInit()
{
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    config.prescaler = RTC_PRESCALER;
    nrfx_rtc_init(&rtc, &config, RtcHandler);
    nrfx_rtc_tick_enable(&rtc, true);
    IncrementCountertime();
    nrfx_rtc_cc_set(&rtc, 0, compare_countertime, true);
    nrfx_rtc_enable(&rtc);
}

int main(int aArgc, char *aArgv[])
{
    otError error = OT_ERROR_NONE;

    otSysInit(aArgc, aArgv);
    nrf_pwr_mgmt_init();
    // Set TWI pins
    htu21d_twi_config_t twi_config = {
        .scl_pin = NRF_GPIO_PIN_MAP(0, 17),
        .sda_pin = NRF_GPIO_PIN_MAP(0, 20)
    };
    driver_htu21d_init(&htu21d, &twi_config);
    power_manager_init();
    RtcInit();
    instance = otInstanceInitSingle();
    ThreadInit(instance);

    sensor_topic = otMqttsnCreatePredefinedTopicId(PREDEFINED_SENSOR_TOPIC_ID);
    telemetry_topic = otMqttsnCreatePredefinedTopicId(PREDEFINED_TELEMETRY_TOPIC_ID);
    otMqttsnStart(instance, CLIENT_PORT);
    otMqttsnSetConnectedHandler(instance, HandleConnected, (void *)instance);
    otMqttsnSetDisconnectedHandler(instance, HandleDisconnected, (void *)instance);

    while (true)
    {
        otTaskletsProcess(instance);
        otSysProcessDrivers(instance);

        if (!otTaskletsArePending(instance))
        {
            // Go to idle mode until RTC handler is invoked
            while (sleeping)
            {
                nrf_pwr_mgmt_run();
            }
        }
        // When woken up by RTC handler wake up the MQTT-SN client and receive messages queued
        // while staying in sleep mode.
        if (!sleeping && otMqttsnGetState(instance) == kStateAsleep)
        {
            WakeUp();
        }
        // Reconnect in case of connection lost
        if (otMqttsnGetState(instance) == kStateLost)
        {
            otMqttsnReconnect(instance);
        }
    }
    return error;
}

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);
}
