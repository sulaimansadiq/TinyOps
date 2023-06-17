################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Core/Src/overlay_fw.cc 

C_SRCS += \
../Core/Src/dmacopying.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c 

CC_DEPS += \
./Core/Src/overlay_fw.d 

OBJS += \
./Core/Src/dmacopying.o \
./Core/Src/main.o \
./Core/Src/overlay_fw.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o 

C_DEPS += \
./Core/Src/dmacopying.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCMSIS_NN -DUSE_HAL_DRIVER -DSTM32F469xx -c -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/examples/hello_world" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Drivers/BSP/STM32469I-Discovery" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/%.o: ../Core/Src/%.cc Core/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCMSIS_NN -DUSE_HAL_DRIVER -DSTM32F469xx -c -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/examples/hello_world" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Drivers/BSP/STM32469I-Discovery" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

