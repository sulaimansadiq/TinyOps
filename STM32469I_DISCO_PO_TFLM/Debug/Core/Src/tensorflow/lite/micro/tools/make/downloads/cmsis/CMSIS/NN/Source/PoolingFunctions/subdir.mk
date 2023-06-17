################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_avgpool_s8.c \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_max_pool_s8.c \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_pool_q7_HWC.c 

OBJS += \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_avgpool_s8.o \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_max_pool_s8.o \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_pool_q7_HWC.o 

C_DEPS += \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_avgpool_s8.d \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_max_pool_s8.d \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/arm_pool_q7_HWC.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/%.o: ../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/%.c Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCMSIS_NN -DUSE_HAL_DRIVER -DSTM32F469xx -c -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/examples/hello_world" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Drivers/BSP/STM32469I-Discovery" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

