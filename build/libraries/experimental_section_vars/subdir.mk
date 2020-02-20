################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libraries/experimental_section_vars/nrf_section_iter.c 

OBJS += \
./libraries/experimental_section_vars/nrf_section_iter.o 

C_DEPS += \
./libraries/experimental_section_vars/nrf_section_iter.d 


# Each subdirectory must supply rules for building sources it contributes
libraries/experimental_section_vars/%.o: ../libraries/experimental_section_vars/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM Cross C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-move-loop-invariants -Wall -Wextra  -g -DOS_USE_SEMIHOSTING -DOS_USE_TRACE_SEMIHOSTING_DEBUG -DOPENTHREAD_FTD=1 -DOPENTHREAD_CONFIG_MQTTSN_ENABLE=1 -DNRF52840_XXAA -DOPENTHREAD_CONFIG_JOINER_ENABLE=1 -DOPENTHREAD_CONFIG_DNS_CLIENT_ENABLE=1 -DOPENTHREAD_CONFIG_IP6_SLAAC_ENABLE=1 -DUART_AS_SERIAL_TRANSPORT=1 -DNRFX_TWIM_ENABLED=1 -DNRFX_TWIM0_ENABLED=1 -DNRF_PWR_MGMT_ENABLED=1 -DAPP_TIMER_ENABLED=1 -DNRFX_RTC_ENABLED=1 -DNRFX_RTC1_ENABLED=1 -DNRF_SECTION_ITER_ENABLED=1 -I../libraries/atomic -I../openthread/include -I../openthread/src/core -I../openthread/third_party/NordicSemiconductor/libraries/nrf_security/include -I../openthread/examples/platforms -I../openthread/third_party/NordicSemiconductor/nrfx/mdk/ -I../openthread/third_party/NordicSemiconductor/cmsis/ -I../openthread/third_party/NordicSemiconductor/nrfx/drivers/include -I../openthread/third_party/NordicSemiconductor/dependencies -I../openthread/third_party/NordicSemiconductor/nrfx -I../openthread/third_party/NordicSemiconductor/libraries/app_error -I../drivers -I../openthread/third_party/NordicSemiconductor/libraries/delay -I../nrf-drivers/drivers/htu21d -I../nrf-drivers/include -I../nrf-drivers/drivers/bmp180 -I../libraries/pwr_mgmt -I../libraries/experimental_section_vars -I../libraries/mutex -I../openthread/third_party/NordicSemiconductor/nrfx/hal -I../include -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


