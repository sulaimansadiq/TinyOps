################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/FatFs/src/diskio.c \
../Middlewares/Third_Party/FatFs/src/ff.c \
../Middlewares/Third_Party/FatFs/src/ff_gen_drv.c 

OBJS += \
./Middlewares/Third_Party/FatFs/src/diskio.o \
./Middlewares/Third_Party/FatFs/src/ff.o \
./Middlewares/Third_Party/FatFs/src/ff_gen_drv.o 

C_DEPS += \
./Middlewares/Third_Party/FatFs/src/diskio.d \
./Middlewares/Third_Party/FatFs/src/ff.d \
./Middlewares/Third_Party/FatFs/src/ff_gen_drv.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/FatFs/src/%.o: ../Middlewares/Third_Party/FatFs/src/%.c Middlewares/Third_Party/FatFs/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCMSIS_NN -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/FATFS/Target" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Middlewares/Third_Party/FatFs/src" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/FATFS/App" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/ruy" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/flatbuffers/include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/gemmlowp" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-FatFs-2f-src

clean-Middlewares-2f-Third_Party-2f-FatFs-2f-src:
	-$(RM) ./Middlewares/Third_Party/FatFs/src/diskio.d ./Middlewares/Third_Party/FatFs/src/diskio.o ./Middlewares/Third_Party/FatFs/src/ff.d ./Middlewares/Third_Party/FatFs/src/ff.o ./Middlewares/Third_Party/FatFs/src/ff_gen_drv.d ./Middlewares/Third_Party/FatFs/src/ff_gen_drv.o

.PHONY: clean-Middlewares-2f-Third_Party-2f-FatFs-2f-src

