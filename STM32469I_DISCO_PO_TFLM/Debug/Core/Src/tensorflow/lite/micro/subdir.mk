################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Core/Src/tensorflow/lite/micro/all_ops_resolver.cc \
../Core/Src/tensorflow/lite/micro/memory_helpers.cc \
../Core/Src/tensorflow/lite/micro/micro_allocator.cc \
../Core/Src/tensorflow/lite/micro/micro_error_reporter.cc \
../Core/Src/tensorflow/lite/micro/micro_interpreter.cc \
../Core/Src/tensorflow/lite/micro/micro_profiler.cc \
../Core/Src/tensorflow/lite/micro/micro_string.cc \
../Core/Src/tensorflow/lite/micro/micro_time.cc \
../Core/Src/tensorflow/lite/micro/micro_utils.cc \
../Core/Src/tensorflow/lite/micro/recording_micro_allocator.cc \
../Core/Src/tensorflow/lite/micro/recording_simple_memory_allocator.cc \
../Core/Src/tensorflow/lite/micro/simple_memory_allocator.cc \
../Core/Src/tensorflow/lite/micro/system_setup.cc \
../Core/Src/tensorflow/lite/micro/test_helpers.cc 

CC_DEPS += \
./Core/Src/tensorflow/lite/micro/all_ops_resolver.d \
./Core/Src/tensorflow/lite/micro/memory_helpers.d \
./Core/Src/tensorflow/lite/micro/micro_allocator.d \
./Core/Src/tensorflow/lite/micro/micro_error_reporter.d \
./Core/Src/tensorflow/lite/micro/micro_interpreter.d \
./Core/Src/tensorflow/lite/micro/micro_profiler.d \
./Core/Src/tensorflow/lite/micro/micro_string.d \
./Core/Src/tensorflow/lite/micro/micro_time.d \
./Core/Src/tensorflow/lite/micro/micro_utils.d \
./Core/Src/tensorflow/lite/micro/recording_micro_allocator.d \
./Core/Src/tensorflow/lite/micro/recording_simple_memory_allocator.d \
./Core/Src/tensorflow/lite/micro/simple_memory_allocator.d \
./Core/Src/tensorflow/lite/micro/system_setup.d \
./Core/Src/tensorflow/lite/micro/test_helpers.d 

OBJS += \
./Core/Src/tensorflow/lite/micro/all_ops_resolver.o \
./Core/Src/tensorflow/lite/micro/memory_helpers.o \
./Core/Src/tensorflow/lite/micro/micro_allocator.o \
./Core/Src/tensorflow/lite/micro/micro_error_reporter.o \
./Core/Src/tensorflow/lite/micro/micro_interpreter.o \
./Core/Src/tensorflow/lite/micro/micro_profiler.o \
./Core/Src/tensorflow/lite/micro/micro_string.o \
./Core/Src/tensorflow/lite/micro/micro_time.o \
./Core/Src/tensorflow/lite/micro/micro_utils.o \
./Core/Src/tensorflow/lite/micro/recording_micro_allocator.o \
./Core/Src/tensorflow/lite/micro/recording_simple_memory_allocator.o \
./Core/Src/tensorflow/lite/micro/simple_memory_allocator.o \
./Core/Src/tensorflow/lite/micro/system_setup.o \
./Core/Src/tensorflow/lite/micro/test_helpers.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tensorflow/lite/micro/%.o: ../Core/Src/tensorflow/lite/micro/%.cc Core/Src/tensorflow/lite/micro/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCMSIS_NN -DUSE_HAL_DRIVER -DSTM32F469xx -c -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/examples/hello_world" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Drivers/BSP/STM32469I-Discovery" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

