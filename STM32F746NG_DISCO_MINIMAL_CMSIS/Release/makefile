################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Middlewares/Third_Party/FatFs/src/option/subdir.mk
-include Middlewares/Third_Party/FatFs/src/subdir.mk
-include FATFS/Target/subdir.mk
-include FATFS/App/subdir.mk
-include Drivers/STM32F7xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/tensorflow/lite/schema/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/SoftmaxFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/SVDFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ReshapeFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/PoolingFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/NNSupportFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/FullyConnectedFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConvolutionFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ConcatenationFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/BasicMathFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Source/ActivationFunctions/subdir.mk
-include Core/Src/tensorflow/lite/micro/memory_planner/subdir.mk
-include Core/Src/tensorflow/lite/micro/mbed/subdir.mk
-include Core/Src/tensorflow/lite/micro/kernels/cmsis_nn/subdir.mk
-include Core/Src/tensorflow/lite/micro/kernels/subdir.mk
-include Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet_Models/subdir.mk
-include Core/Src/tensorflow/lite/micro/examples/hello_world/subdir.mk
-include Core/Src/tensorflow/lite/micro/subdir.mk
-include Core/Src/tensorflow/lite/kernels/internal/subdir.mk
-include Core/Src/tensorflow/lite/kernels/subdir.mk
-include Core/Src/tensorflow/lite/core/api/subdir.mk
-include Core/Src/tensorflow/lite/c/subdir.mk
-include Core/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := STM32F746NG_DISCO_MINIMAL_CMSIS
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
STM32F746NG_DISCO_MINIMAL_CMSIS.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
STM32F746NG_DISCO_MINIMAL_CMSIS.list \


# All Target
all: main-build

# Main-build Target
main-build: STM32F746NG_DISCO_MINIMAL_CMSIS.elf secondary-outputs

# Tool invocations
STM32F746NG_DISCO_MINIMAL_CMSIS.elf: $(OBJS) $(USER_OBJS) C:\Users\ss2n18\STM32CubeIDE\workspace_1.6.1\STM32F746NG_DISCO_MINIMAL_CMSIS\STM32F746NGHX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-g++ -o "STM32F746NG_DISCO_MINIMAL_CMSIS.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"C:\Users\ss2n18\STM32CubeIDE\workspace_1.6.1\STM32F746NG_DISCO_MINIMAL_CMSIS\STM32F746NGHX_FLASH.ld" --specs=nosys.specs -Wl,-Map="STM32F746NG_DISCO_MINIMAL_CMSIS.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

STM32F746NG_DISCO_MINIMAL_CMSIS.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "STM32F746NG_DISCO_MINIMAL_CMSIS.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(SIZE_OUTPUT)$(OBJDUMP_LIST)$(C++_DEPS)$(EXECUTABLES)$(OBJS)$(C_UPPER_DEPS)$(CXX_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(C_DEPS)$(CPP_DEPS) STM32F746NG_DISCO_MINIMAL_CMSIS.elf
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
