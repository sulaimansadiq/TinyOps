/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/reference/quantize.h"
#include "tensorflow/lite/kernels/internal/reference/requantize.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"
#include "tensorflow/lite/micro/kernels/quantize.h"
#include "tensorflow/lite/micro/micro_utils.h"

//#include "overlay_fw.h"

#ifdef OVERLAY_FW
extern "C" {
#include "dmacopying.h"
#include "main.h"
}

extern DMACopyQue DMAQue[NUM_DMA_STREAMS_USED];

extern TfLiteEvalTensor oc_input_tensors[OP_MAX_INPUT_SIZE];
extern TfLiteEvalTensor oc_output_tensors;

extern uint8_t oc_input_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
extern uint8_t oc_output_tensors_buffers[TENSOR_BUFFER_SIZE];
extern uint32_t dma_waiting;
#endif

namespace tflite {

TfLiteStatus PrepareQuantizeReference(TfLiteContext* context,
                                      TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  auto* data = static_cast<OpDataQuantizeReference*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, 0);
  TF_LITE_ENSURE(context, input != nullptr);
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE(context, output != nullptr);

  // TODO(b/128934713): Add support for fixed-point per-channel quantization.
  // Currently this only support affine per-layer quantization.
  TF_LITE_ENSURE_EQ(context, output->quantization.type,
                    kTfLiteAffineQuantization);
  const auto* affine_quantization =
      reinterpret_cast<TfLiteAffineQuantization*>(output->quantization.params);
  TF_LITE_ENSURE(context, affine_quantization);
  TF_LITE_ENSURE(context, affine_quantization->scale);
  TF_LITE_ENSURE(context, affine_quantization->scale->size == 1);

  TF_LITE_ENSURE(context, input->type == kTfLiteFloat32 ||
                              input->type == kTfLiteInt16 ||
                              input->type == kTfLiteInt8);
  TF_LITE_ENSURE(context, output->type == kTfLiteInt8 ||
                              output->type == kTfLiteInt16 ||
                              output->type == kTfLiteInt32);

  if ((input->type == kTfLiteInt16 && output->type == kTfLiteInt8) ||
      (input->type == kTfLiteInt8 && output->type == kTfLiteInt8) ||
      (input->type == kTfLiteInt8 && output->type == kTfLiteInt32) ||
      (input->type == kTfLiteInt16 && output->type == kTfLiteInt16) ||
      (input->type == kTfLiteInt16 && output->type == kTfLiteInt32)) {
    double effective_scale = static_cast<double>(input->params.scale) /
                             static_cast<double>(output->params.scale);

    QuantizeMultiplier(effective_scale, &data->requantize_output_multiplier,
                       &data->requantize_output_shift);
  }

  data->quantization_params.zero_point = output->params.zero_point;
  data->quantization_params.scale = static_cast<double>(output->params.scale);

  data->input_zero_point = input->params.zero_point;
  return kTfLiteOk;
}

//#define BUFFER_SIZE 3072
//
//static uint32_t __attribute__((section (".sdram_bss")))  aSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  aDST_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  bSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  bDST_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  cSRC_Const_Buffer[BUFFER_SIZE];
//static uint32_t __attribute__((section (".sdram_bss")))  cDST_Buffer[BUFFER_SIZE];

TfLiteStatus EvalQuantizeReference(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  auto* data = static_cast<OpDataQuantizeReference*>(node->user_data);

  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, 0);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, 0);

  #ifdef OVERLAY_FW
  // copy input tensor
  // copy struct details
  DMA_Copy_Que_Req_Put(&DMAQue[0], (uint8_t *)input, (uint8_t *)&oc_input_tensors[0], sizeof(TfLiteEvalTensor));
  uint32_t inp_size = 4;						// hard-coded for float32 input
  for (int i = 0; i < input->dims->size; i++)
	  inp_size *= input->dims->data[i];
  // copy tensor buffer data to on-chip data buffer
  inp_size = 4000;
  DMA_Copy_Que_Req_Put(&DMAQue[0], (uint8_t *)input->data.data, &oc_input_tensors_buffers[0][0], inp_size);

  // clean data cache, so that tensor details are written to mem from where dma will copy them to overlaid memory
  SCB_CleanDCache_by_Addr((uint32_t*)(((uint32_t)input) & ~(uint32_t)0x1F), sizeof(TfLiteEvalTensor)+32);
  SCB_CleanDCache_by_Addr((uint32_t*)(((uint32_t)input->data.data) & ~(uint32_t)0x1F), inp_size+32);

  // manually kick DMA_engine
//  StartTransfer(&DmaHandle);

  while ( (DMAQue[0].readIdx != DMAQue[0].writeIdx) && (DMAQue[1].readIdx != DMAQue[1].writeIdx) )
  	  dma_waiting++;

  // invalidate cache to maintain coherency. dma copy into overlayed memory not seen by cache. lines will be dirty
  SCB_InvalidateDCache_by_Addr((uint32_t*)(((uint32_t)&oc_input_tensors[0]) & ~(uint32_t)0x1F), sizeof(TfLiteEvalTensor)+32);
  SCB_InvalidateDCache_by_Addr((uint32_t*)(((uint32_t)&oc_input_tensors_buffers[0][0]) & ~(uint32_t)0x1F), TENSOR_BUFFER_SIZE+32);

//  SCB_InvalidateDCache_by_Addr(&oc_input_tensors[0], sizeof(TfLiteEvalTensor));
//  SCB_InvalidateDCache_by_Addr(&oc_input_tensors_buffers[0][0], TENSOR_BUFFER_SIZE);
//  SCB_InvalidateDCache();

  // point tensor buffer to on-chip buffer that has just been copied in
  oc_input_tensors[0].data.data = &oc_input_tensors_buffers[0][0];
  // seemlessly replace input pointer to tensor used by rest of func
  input = &oc_input_tensors[0];
  #endif

  if (input->type == kTfLiteFloat32) {
    switch (output->type) {
      case kTfLiteInt8:
        reference_ops::AffineQuantize(
            data->quantization_params, tflite::micro::GetTensorShape(input),
            tflite::micro::GetTensorData<float>(input),
            tflite::micro::GetTensorShape(output),
            tflite::micro::GetTensorData<int8_t>(output));
        break;
      case kTfLiteInt16:
        reference_ops::AffineQuantize(
            data->quantization_params, tflite::micro::GetTensorShape(input),
            tflite::micro::GetTensorData<float>(input),
            tflite::micro::GetTensorShape(output),
            tflite::micro::GetTensorData<int16_t>(output));
        return kTfLiteOk;
      default:
        TF_LITE_KERNEL_LOG(context, "Input %s, output %s not supported.",
                           TfLiteTypeGetName(input->type),
                           TfLiteTypeGetName(output->type));
        return kTfLiteError;
    }
  } else if (input->type == kTfLiteInt16) {
    size_t size = ElementCount(*input->dims);
    switch (output->type) {
      case kTfLiteInt8:
        reference_ops::Requantize(
            tflite::micro::GetTensorData<int16_t>(input), size,
            data->requantize_output_multiplier, data->requantize_output_shift,
            data->input_zero_point, data->quantization_params.zero_point,
            tflite::micro::GetTensorData<int8_t>(output));
        break;
      case kTfLiteInt16:
        reference_ops::Requantize(
            tflite::micro::GetTensorData<int16_t>(input), size,
            data->requantize_output_multiplier, data->requantize_output_shift,
            data->input_zero_point, data->quantization_params.zero_point,
            tflite::micro::GetTensorData<int16_t>(output));
        return kTfLiteOk;
      case kTfLiteInt32:
        reference_ops::Requantize(
            tflite::micro::GetTensorData<int16_t>(input), size,
            data->requantize_output_multiplier, data->requantize_output_shift,
            data->input_zero_point, data->quantization_params.zero_point,
            tflite::micro::GetTensorData<int32_t>(output));
        return kTfLiteOk;
      default:
        TF_LITE_KERNEL_LOG(context, "Input %s, output %s not supported.",
                           TfLiteTypeGetName(input->type),
                           TfLiteTypeGetName(output->type));
        return kTfLiteError;
    }
  } else if (input->type == kTfLiteInt8) {
    // Int8 to Int8 requantization, required if the input and output tensors
    // have different scales and/or zero points.
    size_t size = ElementCount(*input->dims);
    switch (output->type) {
      case kTfLiteInt8:
        reference_ops::Requantize(
            tflite::micro::GetTensorData<int8_t>(input), size,
            data->requantize_output_multiplier, data->requantize_output_shift,
            data->input_zero_point, data->quantization_params.zero_point,
            tflite::micro::GetTensorData<int8_t>(output));
        break;
      case kTfLiteInt32:
        reference_ops::Requantize(
            tflite::micro::GetTensorData<int8_t>(input), size,
            data->requantize_output_multiplier, data->requantize_output_shift,
            data->input_zero_point, data->quantization_params.zero_point,
            tflite::micro::GetTensorData<int32_t>(output));
        break;
      default:
        TF_LITE_KERNEL_LOG(context, "Input %s, output %s not supported.",
                           TfLiteTypeGetName(input->type),
                           TfLiteTypeGetName(output->type));
        return kTfLiteError;
    }
  } else {
    TF_LITE_KERNEL_LOG(context, "Input %s, output %s not supported.",
                       TfLiteTypeGetName(input->type),
                       TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  // copy output tensor
  // copy struct details
//  DMA_Copy_Que_Req_Put(&DMAQue, (uint8_t *)output, oc_output_tensors_buffers, sizeof(TfLiteEvalTensor));
//  uint32_t out_size = 1;						// hard-coded for int8 output
//  for (int i = 0; i < output->dims->size; i++)
//	  out_size *= output->dims->data[i];
//  // copy data on-chip
//  DMA_Copy_Que_Req_Put(&DMAQue, (uint8_t *)output->data.data, (uint8_t *)oc_output_tensors.data.data, out_size/4);
//
//  // manually kick DMA_engine
//  StartTransfer(&DmaHandle);
//
//  while (DMAQue.readIdx != DMAQue.writeIdx);
//
//  oc_output_tensors.data.data = oc_output_tensors_buffers;

  return kTfLiteOk;
}

}  // namespace tflite
