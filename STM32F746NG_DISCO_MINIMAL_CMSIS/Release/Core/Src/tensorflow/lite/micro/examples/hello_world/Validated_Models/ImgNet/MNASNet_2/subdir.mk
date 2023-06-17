################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet/MNASNet_2/mnasnet_1.0_192_int8_32.cc 

CC_DEPS += \
./Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet/MNASNet_2/mnasnet_1.0_192_int8_32.d 

OBJS += \
./Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet/MNASNet_2/mnasnet_1.0_192_int8_32.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet/MNASNet_2/%.o: ../Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet/MNASNet_2/%.cc Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet/MNASNet_2/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -DUSE_HAL_DRIVER -DTF_LITE_STRIP_ERROR_STRINGS -DCMSIS_NN -DSTM32F746xx -c -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.6.1/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -Ofast -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

