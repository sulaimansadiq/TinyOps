################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_w.c \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_x.c \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_y.c \
../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_z.c 

OBJS += \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_w.o \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_x.o \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_y.o \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_z.o 

C_DEPS += \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_w.d \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_x.d \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_y.d \
./Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/arm_concatenation_s8_z.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/%.o: ../Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/%.c Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DTF_LITE_STRIP_ERROR_STRINGS -DCMSIS_NN -DSTM32F469xx -c -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/examples/hello_world" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Drivers/BSP/STM32469I-Discovery" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Ofast -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

