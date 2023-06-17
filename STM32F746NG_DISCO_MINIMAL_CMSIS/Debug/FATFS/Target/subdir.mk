################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FATFS/Target/bsp_driver_sd.c \
../FATFS/Target/fatfs_platform.c \
../FATFS/Target/sd_diskio.c 

OBJS += \
./FATFS/Target/bsp_driver_sd.o \
./FATFS/Target/fatfs_platform.o \
./FATFS/Target/sd_diskio.o 

C_DEPS += \
./FATFS/Target/bsp_driver_sd.d \
./FATFS/Target/fatfs_platform.d \
./FATFS/Target/sd_diskio.d 


# Each subdirectory must supply rules for building sources it contributes
FATFS/Target/%.o: ../FATFS/Target/%.c FATFS/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DCMSIS_NN -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/FATFS/Target" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Middlewares/Third_Party/FatFs/src" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/FATFS/App" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/ruy" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/flatbuffers/include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/third_party/gemmlowp" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src" -I"C:/Users/Sadaf Bhopal/STM32CubeIDE/workspace_1.8.0/STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FATFS-2f-Target

clean-FATFS-2f-Target:
	-$(RM) ./FATFS/Target/bsp_driver_sd.d ./FATFS/Target/bsp_driver_sd.o ./FATFS/Target/fatfs_platform.d ./FATFS/Target/fatfs_platform.o ./FATFS/Target/sd_diskio.d ./FATFS/Target/sd_diskio.o

.PHONY: clean-FATFS-2f-Target
