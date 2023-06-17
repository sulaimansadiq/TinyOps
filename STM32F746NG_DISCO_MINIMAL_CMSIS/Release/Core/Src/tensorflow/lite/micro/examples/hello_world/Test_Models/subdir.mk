################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r64.cc \
../Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r80-test.cc 

CC_DEPS += \
./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r64.d \
./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r80-test.d 

OBJS += \
./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r64.o \
./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r80-test.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/%.o: ../Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/%.cc Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DUSE_HAL_DRIVER -DTF_LITE_STRIP_ERROR_STRINGS -DCMSIS_NN -DSTM32F746xx -c -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/FATFS/Target" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Middlewares/Third_Party/FatFs/src" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/FATFS/App" -I../Core/Inc -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/ruy" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/flatbuffers/include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/gemmlowp" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -Ofast -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-tensorflow-2f-lite-2f-micro-2f-examples-2f-hello_world-2f-Test_Models

clean-Core-2f-Src-2f-tensorflow-2f-lite-2f-micro-2f-examples-2f-hello_world-2f-Test_Models:
	-$(RM) ./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r64.d ./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r64.o ./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r80-test.d ./Core/Src/tensorflow/lite/micro/examples/hello_world/Test_Models/proxyless-w0.4-r80-test.o

.PHONY: clean-Core-2f-Src-2f-tensorflow-2f-lite-2f-micro-2f-examples-2f-hello_world-2f-Test_Models

