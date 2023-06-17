################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../Core/Src/tensorflow/lite/micro/kernels/activations.cc \
../Core/Src/tensorflow/lite/micro/kernels/add_n.cc \
../Core/Src/tensorflow/lite/micro/kernels/arg_min_max.cc \
../Core/Src/tensorflow/lite/micro/kernels/batch_to_space_nd.cc \
../Core/Src/tensorflow/lite/micro/kernels/cast.cc \
../Core/Src/tensorflow/lite/micro/kernels/ceil.cc \
../Core/Src/tensorflow/lite/micro/kernels/circular_buffer.cc \
../Core/Src/tensorflow/lite/micro/kernels/comparisons.cc \
../Core/Src/tensorflow/lite/micro/kernels/concatenation.cc \
../Core/Src/tensorflow/lite/micro/kernels/conv_common.cc \
../Core/Src/tensorflow/lite/micro/kernels/depthwise_conv_common.cc \
../Core/Src/tensorflow/lite/micro/kernels/dequantize.cc \
../Core/Src/tensorflow/lite/micro/kernels/detection_postprocess.cc \
../Core/Src/tensorflow/lite/micro/kernels/div.cc \
../Core/Src/tensorflow/lite/micro/kernels/elementwise.cc \
../Core/Src/tensorflow/lite/micro/kernels/elu.cc \
../Core/Src/tensorflow/lite/micro/kernels/ethosu.cc \
../Core/Src/tensorflow/lite/micro/kernels/exp.cc \
../Core/Src/tensorflow/lite/micro/kernels/expand_dims.cc \
../Core/Src/tensorflow/lite/micro/kernels/fill.cc \
../Core/Src/tensorflow/lite/micro/kernels/floor.cc \
../Core/Src/tensorflow/lite/micro/kernels/fully_connected_common.cc \
../Core/Src/tensorflow/lite/micro/kernels/hard_swish.cc \
../Core/Src/tensorflow/lite/micro/kernels/kernel_runner.cc \
../Core/Src/tensorflow/lite/micro/kernels/kernel_util.cc \
../Core/Src/tensorflow/lite/micro/kernels/l2_pool_2d.cc \
../Core/Src/tensorflow/lite/micro/kernels/l2norm.cc \
../Core/Src/tensorflow/lite/micro/kernels/leaky_relu.cc \
../Core/Src/tensorflow/lite/micro/kernels/logical.cc \
../Core/Src/tensorflow/lite/micro/kernels/logistic.cc \
../Core/Src/tensorflow/lite/micro/kernels/maximum_minimum.cc \
../Core/Src/tensorflow/lite/micro/kernels/neg.cc \
../Core/Src/tensorflow/lite/micro/kernels/pack.cc \
../Core/Src/tensorflow/lite/micro/kernels/pad.cc \
../Core/Src/tensorflow/lite/micro/kernels/prelu.cc \
../Core/Src/tensorflow/lite/micro/kernels/quantize.cc \
../Core/Src/tensorflow/lite/micro/kernels/quantize_common.cc \
../Core/Src/tensorflow/lite/micro/kernels/reduce.cc \
../Core/Src/tensorflow/lite/micro/kernels/reshape.cc \
../Core/Src/tensorflow/lite/micro/kernels/resize_nearest_neighbor.cc \
../Core/Src/tensorflow/lite/micro/kernels/round.cc \
../Core/Src/tensorflow/lite/micro/kernels/shape.cc \
../Core/Src/tensorflow/lite/micro/kernels/softmax_common.cc \
../Core/Src/tensorflow/lite/micro/kernels/space_to_batch_nd.cc \
../Core/Src/tensorflow/lite/micro/kernels/split.cc \
../Core/Src/tensorflow/lite/micro/kernels/split_v.cc \
../Core/Src/tensorflow/lite/micro/kernels/squeeze.cc \
../Core/Src/tensorflow/lite/micro/kernels/strided_slice.cc \
../Core/Src/tensorflow/lite/micro/kernels/sub.cc \
../Core/Src/tensorflow/lite/micro/kernels/svdf_common.cc \
../Core/Src/tensorflow/lite/micro/kernels/tanh.cc \
../Core/Src/tensorflow/lite/micro/kernels/transpose_conv.cc \
../Core/Src/tensorflow/lite/micro/kernels/unpack.cc \
../Core/Src/tensorflow/lite/micro/kernels/zeros_like.cc 

CC_DEPS += \
./Core/Src/tensorflow/lite/micro/kernels/activations.d \
./Core/Src/tensorflow/lite/micro/kernels/add_n.d \
./Core/Src/tensorflow/lite/micro/kernels/arg_min_max.d \
./Core/Src/tensorflow/lite/micro/kernels/batch_to_space_nd.d \
./Core/Src/tensorflow/lite/micro/kernels/cast.d \
./Core/Src/tensorflow/lite/micro/kernels/ceil.d \
./Core/Src/tensorflow/lite/micro/kernels/circular_buffer.d \
./Core/Src/tensorflow/lite/micro/kernels/comparisons.d \
./Core/Src/tensorflow/lite/micro/kernels/concatenation.d \
./Core/Src/tensorflow/lite/micro/kernels/conv_common.d \
./Core/Src/tensorflow/lite/micro/kernels/depthwise_conv_common.d \
./Core/Src/tensorflow/lite/micro/kernels/dequantize.d \
./Core/Src/tensorflow/lite/micro/kernels/detection_postprocess.d \
./Core/Src/tensorflow/lite/micro/kernels/div.d \
./Core/Src/tensorflow/lite/micro/kernels/elementwise.d \
./Core/Src/tensorflow/lite/micro/kernels/elu.d \
./Core/Src/tensorflow/lite/micro/kernels/ethosu.d \
./Core/Src/tensorflow/lite/micro/kernels/exp.d \
./Core/Src/tensorflow/lite/micro/kernels/expand_dims.d \
./Core/Src/tensorflow/lite/micro/kernels/fill.d \
./Core/Src/tensorflow/lite/micro/kernels/floor.d \
./Core/Src/tensorflow/lite/micro/kernels/fully_connected_common.d \
./Core/Src/tensorflow/lite/micro/kernels/hard_swish.d \
./Core/Src/tensorflow/lite/micro/kernels/kernel_runner.d \
./Core/Src/tensorflow/lite/micro/kernels/kernel_util.d \
./Core/Src/tensorflow/lite/micro/kernels/l2_pool_2d.d \
./Core/Src/tensorflow/lite/micro/kernels/l2norm.d \
./Core/Src/tensorflow/lite/micro/kernels/leaky_relu.d \
./Core/Src/tensorflow/lite/micro/kernels/logical.d \
./Core/Src/tensorflow/lite/micro/kernels/logistic.d \
./Core/Src/tensorflow/lite/micro/kernels/maximum_minimum.d \
./Core/Src/tensorflow/lite/micro/kernels/neg.d \
./Core/Src/tensorflow/lite/micro/kernels/pack.d \
./Core/Src/tensorflow/lite/micro/kernels/pad.d \
./Core/Src/tensorflow/lite/micro/kernels/prelu.d \
./Core/Src/tensorflow/lite/micro/kernels/quantize.d \
./Core/Src/tensorflow/lite/micro/kernels/quantize_common.d \
./Core/Src/tensorflow/lite/micro/kernels/reduce.d \
./Core/Src/tensorflow/lite/micro/kernels/reshape.d \
./Core/Src/tensorflow/lite/micro/kernels/resize_nearest_neighbor.d \
./Core/Src/tensorflow/lite/micro/kernels/round.d \
./Core/Src/tensorflow/lite/micro/kernels/shape.d \
./Core/Src/tensorflow/lite/micro/kernels/softmax_common.d \
./Core/Src/tensorflow/lite/micro/kernels/space_to_batch_nd.d \
./Core/Src/tensorflow/lite/micro/kernels/split.d \
./Core/Src/tensorflow/lite/micro/kernels/split_v.d \
./Core/Src/tensorflow/lite/micro/kernels/squeeze.d \
./Core/Src/tensorflow/lite/micro/kernels/strided_slice.d \
./Core/Src/tensorflow/lite/micro/kernels/sub.d \
./Core/Src/tensorflow/lite/micro/kernels/svdf_common.d \
./Core/Src/tensorflow/lite/micro/kernels/tanh.d \
./Core/Src/tensorflow/lite/micro/kernels/transpose_conv.d \
./Core/Src/tensorflow/lite/micro/kernels/unpack.d \
./Core/Src/tensorflow/lite/micro/kernels/zeros_like.d 

OBJS += \
./Core/Src/tensorflow/lite/micro/kernels/activations.o \
./Core/Src/tensorflow/lite/micro/kernels/add_n.o \
./Core/Src/tensorflow/lite/micro/kernels/arg_min_max.o \
./Core/Src/tensorflow/lite/micro/kernels/batch_to_space_nd.o \
./Core/Src/tensorflow/lite/micro/kernels/cast.o \
./Core/Src/tensorflow/lite/micro/kernels/ceil.o \
./Core/Src/tensorflow/lite/micro/kernels/circular_buffer.o \
./Core/Src/tensorflow/lite/micro/kernels/comparisons.o \
./Core/Src/tensorflow/lite/micro/kernels/concatenation.o \
./Core/Src/tensorflow/lite/micro/kernels/conv_common.o \
./Core/Src/tensorflow/lite/micro/kernels/depthwise_conv_common.o \
./Core/Src/tensorflow/lite/micro/kernels/dequantize.o \
./Core/Src/tensorflow/lite/micro/kernels/detection_postprocess.o \
./Core/Src/tensorflow/lite/micro/kernels/div.o \
./Core/Src/tensorflow/lite/micro/kernels/elementwise.o \
./Core/Src/tensorflow/lite/micro/kernels/elu.o \
./Core/Src/tensorflow/lite/micro/kernels/ethosu.o \
./Core/Src/tensorflow/lite/micro/kernels/exp.o \
./Core/Src/tensorflow/lite/micro/kernels/expand_dims.o \
./Core/Src/tensorflow/lite/micro/kernels/fill.o \
./Core/Src/tensorflow/lite/micro/kernels/floor.o \
./Core/Src/tensorflow/lite/micro/kernels/fully_connected_common.o \
./Core/Src/tensorflow/lite/micro/kernels/hard_swish.o \
./Core/Src/tensorflow/lite/micro/kernels/kernel_runner.o \
./Core/Src/tensorflow/lite/micro/kernels/kernel_util.o \
./Core/Src/tensorflow/lite/micro/kernels/l2_pool_2d.o \
./Core/Src/tensorflow/lite/micro/kernels/l2norm.o \
./Core/Src/tensorflow/lite/micro/kernels/leaky_relu.o \
./Core/Src/tensorflow/lite/micro/kernels/logical.o \
./Core/Src/tensorflow/lite/micro/kernels/logistic.o \
./Core/Src/tensorflow/lite/micro/kernels/maximum_minimum.o \
./Core/Src/tensorflow/lite/micro/kernels/neg.o \
./Core/Src/tensorflow/lite/micro/kernels/pack.o \
./Core/Src/tensorflow/lite/micro/kernels/pad.o \
./Core/Src/tensorflow/lite/micro/kernels/prelu.o \
./Core/Src/tensorflow/lite/micro/kernels/quantize.o \
./Core/Src/tensorflow/lite/micro/kernels/quantize_common.o \
./Core/Src/tensorflow/lite/micro/kernels/reduce.o \
./Core/Src/tensorflow/lite/micro/kernels/reshape.o \
./Core/Src/tensorflow/lite/micro/kernels/resize_nearest_neighbor.o \
./Core/Src/tensorflow/lite/micro/kernels/round.o \
./Core/Src/tensorflow/lite/micro/kernels/shape.o \
./Core/Src/tensorflow/lite/micro/kernels/softmax_common.o \
./Core/Src/tensorflow/lite/micro/kernels/space_to_batch_nd.o \
./Core/Src/tensorflow/lite/micro/kernels/split.o \
./Core/Src/tensorflow/lite/micro/kernels/split_v.o \
./Core/Src/tensorflow/lite/micro/kernels/squeeze.o \
./Core/Src/tensorflow/lite/micro/kernels/strided_slice.o \
./Core/Src/tensorflow/lite/micro/kernels/sub.o \
./Core/Src/tensorflow/lite/micro/kernels/svdf_common.o \
./Core/Src/tensorflow/lite/micro/kernels/tanh.o \
./Core/Src/tensorflow/lite/micro/kernels/transpose_conv.o \
./Core/Src/tensorflow/lite/micro/kernels/unpack.o \
./Core/Src/tensorflow/lite/micro/kernels/zeros_like.o 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/tensorflow/lite/micro/kernels/%.o: ../Core/Src/tensorflow/lite/micro/kernels/%.cc Core/Src/tensorflow/lite/micro/kernels/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -DUSE_HAL_DRIVER -DTF_LITE_STRIP_ERROR_STRINGS -DCMSIS_NN -DSTM32F469xx -c -I../Core/Inc -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/examples/hello_world" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Drivers/BSP/STM32469I-Discovery" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/ruy" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/flatbuffers/include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/third_party/gemmlowp" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/NN/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/Core/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src/tensorflow/lite/micro/tools/make/downloads/cmsis/CMSIS/DSP/Include" -I"C:/Users/ss2n18/STM32CubeIDE/workspace_1.7.0/STM32469I_DISCO_PO_TFLM/Core/Src" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -Ofast -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

