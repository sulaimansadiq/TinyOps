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

#include "main.h"
#include "tensorflow/lite/micro/examples/hello_world/main_functions.h"
#include "tensorflow/lite/micro/examples/hello_world/model.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/examples/hello_world/constants.h"
#include "tensorflow/lite/micro/examples/hello_world/model.h"
#include "tensorflow/lite/micro/examples/hello_world/output_handler.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "overlay_fw.h"

#ifdef OVERLAY_FW
// On-chip Tensors

volatile TfLiteEvalTensor __attribute__((aligned (32))) oc_input_tensors[OP_MAX_INPUT_SIZE];
//volatile uint8_t __attribute__((aligned (32))) oc_input_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
//volatile uint8_t __attribute__((aligned (32))) __attribute__((section (".ram_bss"))) oc_input_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
volatile uint8_t * oc_input_tensors_buffers[OP_MAX_INPUT_SIZE];
volatile int __attribute__((aligned (32))) oc_input_tensors_dims_data[OP_MAX_INPUT_SIZE][MAX_NUM_DIMS];
volatile TfLiteIntArray __attribute__((aligned (32))) oc_input_tensors_dims_array_0 = { 4, {0, 0, 0, 0} };
volatile TfLiteIntArray __attribute__((aligned (32))) oc_input_tensors_dims_array_1 = { 4, {0, 0, 0, 0} };

volatile TfLiteEvalTensor __attribute__((aligned (32))) oc_input2_tensors[OP_MAX_INPUT_SIZE];
//volatile uint8_t __attribute__((aligned (32))) oc_input2_tensors_buffers_0[INPUT2_TENSOR_BUFFER_0_SIZE];
//volatile uint8_t __attribute__((aligned (32))) oc_input2_tensors_buffers_1[INPUT2_TENSOR_BUFFER_1_SIZE];
//volatile uint8_t __attribute__((aligned (32))) __attribute__((section (".ram_bss"))) oc_input2_tensors_buffers_0[INPUT2_TENSOR_BUFFER_0_SIZE];
//volatile uint8_t __attribute__((aligned (32))) __attribute__((section (".ram_bss"))) oc_input2_tensors_buffers_1[INPUT2_TENSOR_BUFFER_1_SIZE];
volatile uint8_t * oc_input2_tensors_buffers_0;
volatile uint8_t * oc_input2_tensors_buffers_1;
//alignas(32) volatile uint8_t oc_input2_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
volatile int __attribute__((aligned (32))) oc_input2_tensors_dims_data[OP_MAX_INPUT_SIZE][MAX_NUM_DIMS];
volatile TfLiteIntArray __attribute__((aligned (32))) oc_input2_tensors_dims_array_0 = { 4, {0, 0, 0, 0} };
volatile TfLiteIntArray __attribute__((aligned (32))) oc_input2_tensors_dims_array_1 = { 4, {0, 0, 0, 0} };

#ifdef OVERLAY_BIASES
volatile TfLiteEvalTensor __attribute__((aligned (32))) oc_input3_tensor;
//volatile uint8_t __attribute__((aligned (32))) __attribute__((section (".ram_bss"))) oc_input3_tensor_buffer[BIAS_TENSOR_BUFFER_SIZE];
volatile uint8_t * oc_input3_tensor_buffer;
//volatile uint8_t __attribute__((aligned (32))) oc_input3_tensor_buffer[BIAS_TENSOR_BUFFER_SIZE];
volatile int __attribute__((aligned (32))) oc_input3_tensor_dims_data[MAX_NUM_DIMS];
volatile TfLiteIntArray __attribute__((aligned (32))) oc_input3_tensor_dims_array = { 4, {0, 0, 0, 0} };
#endif

volatile TfLiteEvalTensor __attribute__((aligned (32))) oc_output_tensors;
//alignas(32) uint8_t oc_output_tensors_buffers[TENSOR_BUFFER_SIZE];
volatile int __attribute__((aligned (32))) oc_output_tensors_dims_data[MAX_NUM_DIMS];
volatile TfLiteIntArray __attribute__((aligned (32))) oc_output_tensors_dims_array = {4, {0, 0, 0, 0}};

#ifdef OVERLAY_OUTPUT_TENSORS
volatile TfLiteEvalTensor __attribute__((aligned (32))) oc_output2_tensors[OP_MAX_INPUT_SIZE];
volatile uint8_t __attribute__((aligned (32))) oc_output2_tensors_buffers[OP_MAX_INPUT_SIZE][TENSOR_BUFFER_SIZE];
volatile int __attribute__((aligned (32))) oc_output2_tensors_dims_data[OP_MAX_INPUT_SIZE][MAX_NUM_DIMS];
volatile TfLiteIntArray __attribute__((aligned (32))) oc_output2_tensors_dims_array_0 = { 4, {0, 0, 0, 0} };
volatile TfLiteIntArray __attribute__((aligned (32))) oc_output2_tensors_dims_array_1 = { 4, {0, 0, 0, 0} };
#endif

#ifdef OVERLAY_QUANT_PARAMS
//volatile uint8_t __attribute__((aligned (32))) quant_out_multiplier[QUANT_PARAMS_BUFFER_SIZE+128];
//volatile uint8_t __attribute__((aligned (32))) quant_out_shift[QUANT_PARAMS_BUFFER_SIZE+128];
//volatile uint8_t __attribute__((aligned (32))) __attribute__((section (".ram_bss"))) quant_out_multiplier[QUANT_PARAMS_BUFFER_SIZE+128];
//volatile uint8_t __attribute__((aligned (32))) __attribute__((section (".ram_bss"))) quant_out_shift[QUANT_PARAMS_BUFFER_SIZE+128];
volatile uint8_t * quant_out_multiplier;
volatile uint8_t * quant_out_shift;
#endif

#ifdef OVERLAY_FW
alignas(32) uint8_t __attribute__((section (".ram_bss"))) tinyops_arena[PLATFORM_AVAILABLE_SRAM];
#endif

volatile PartitionedExecutionState __attribute__((section (".sdram_bss"))) partitioned_exec_state;

uint32_t dma_waiting = 0;

extern uint32_t dma_waiting_conv;
extern uint32_t dma_waiting_depconv;
extern uint32_t dma_waiting_pad;
extern uint32_t dma_waiting_add;
extern uint32_t dma_waiting_pool;

extern uint32_t dma_waiting_conv_p0;
extern uint32_t dma_waiting_depconv_p0;
extern uint32_t dma_waiting_pad_p0;
extern uint32_t dma_waiting_add_p0;
extern uint32_t dma_waiting_pool_p0;

extern uint16_t dma_waiting_pad_time;

#endif

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim10;

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
int inference_count = 0;

}  // namespace

//constexpr int kTensorArenaSize = 150000;

//constexpr int kTensorArenaSize = 64150; // 523000; // SC_7 // 64150; // SC_5 // 56000; // SC_6
//constexpr int kTensorArenaSize = 233000; // Mbv2(w0.3,r80)
//constexpr int kTensorArenaSize = 588000; // Mbv2(w0.35,r144)
//constexpr int kTensorArenaSize = 227000; // Proxyless(w0.25,r112)
//constexpr int kTensorArenaSize = 239000; // Proxyless(w0.25,r112)
//constexpr int kTensorArenaSize = 463000; // Proxyless(w0.3,r176)
//constexpr int kTensorArenaSize = 479000; // MCUNet-256kb-1mb
//constexpr int kTensorArenaSize = 504000; // MCUNet-320kb-1mb
//constexpr int kTensorArenaSize = 772000; // MCUNet-512kb-1mb
//constexpr int kTensorArenaSize = 425000; // MNASNet(w1.0,r128)
//constexpr int kTensorArenaSize = 572300; // MNASNet(w1.0,r160)
//constexpr int kTensorArenaSize = 753000; // MNASNet(w1.0,r192)
//constexpr int kTensorArenaSize = 520000; // MNASNet(w0.5,r224)
//constexpr int kTensorArenaSize = 930000; // MNASNet(w0.75,r224)

//constexpr int kTensorArenaSize = 930000;
//uint8_t __attribute__((section (".sdram_bss"))) tensor_arena[kTensorArenaSize];
#ifdef MODEL_TENSOR_ARENA_SDRAM
uint8_t * tensor_arena;
extern volatile unsigned long model_size;
uint32_t kTensorArenaSize;
#else

//constexpr int kTensorArenaSize = PLATFORM_AVAILABLE_SRAM; // MNASNet(w1.0,r96)
//alignas(32) uint8_t __attribute__((section (".ram_bss"))) tensor_arena[kTensorArenaSize];
constexpr int kTensorArenaSize = 2700000; //2867200;
alignas(32) uint8_t __attribute__((section (".sdram_bss"))) tensor_arena[kTensorArenaSize];
#endif

// The name of this function is important for Arduino compatibility.
void setup() {

  char uart_buf[50];
  int uart_buf_len;

  tflite::InitializeTarget();

  // Set up logging. Google style is to avoid globals or statics because of
  // lifetime uncertainty, but since this has a trivial destructor it's okay.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  for (int i=0; i<g_model_len; i++)
	  g_model_sdram[i]=g_model[i];

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(&g_model_sdram[0]);
//  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::MicroMutableOpResolver<13> resolver;
  // mcunet ops
    resolver.AddAdd();
    resolver.AddAveragePool2D();
    resolver.AddConv2D();
    resolver.AddDepthwiseConv2D();
    resolver.AddPad();
    resolver.AddRelu6();
    resolver.AddReshape();

  // mnasnet ops
//    resolver.AddAdd();
//    resolver.AddConv2D();
//    resolver.AddDepthwiseConv2D();
//    resolver.AddFullyConnected();
//    resolver.AddMean();
//    resolver.AddRelu();

  // Build an interpreter to run the model with.
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }


  uart_buf_len = sprintf(uart_buf, "Allocated Tensors\n");
  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

  // Obtain pointers to the model's input and output tensors.
  input = interpreter->input(0);
  output = interpreter->output(0);

  // Keep track of how many inferences we have performed.
  inference_count = 0;

}

// The name of this function is important for Arduino compatibility.
uint16_t loop(unsigned char * out_ptr, unsigned int * mem_usage, unsigned int * tensor_arena_size, unsigned int * tensor_arena_ptr) {

//  // Quantize the input from floating-point to integer
//  int8_t x_quantized = x / input->params.scale + input->params.zero_point;
//  // Place the quantized input in the model's input tensor
//  for (int i = 0; i < (64); i++ )

//  for (int j = -125; j < 120; j+=30)
//  {
//	  uart_buf_len = sprintf(uart_buf, "Input fill: %i\n", j);
//	  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
//	  for (int i = 0; i < (112*112*3); i++ )

	uint16_t timer_val;
	char uart_buf[50];
	int uart_buf_len;

//	tflite::InitializeTarget();
//
//	// Set up logging. Google style is to avoid globals or statics because of
//	// lifetime uncertainty, but since this has a trivial destructor it's okay.
//	// NOLINTNEXTLINE(runtime-global-variables)
//	static tflite::MicroErrorReporter micro_error_reporter;
//	error_reporter = &micro_error_reporter;
//
//	// Map the model into a usable data structure. This doesn't involve any
//	// copying or parsing, it's a very lightweight operation.
//	#ifdef MODEL_LOC_INT_FLASH
//	model = tflite::GetModel(&g_model_int_flash[0]);
//	#else
//	model = tflite::GetModel(&g_model_sdram[MODEL_SDRAM_OFFSET]);
//	#endif
//	if (model->version() != TFLITE_SCHEMA_VERSION) {
//	TF_LITE_REPORT_ERROR(error_reporter,
//						 "Model provided is schema version %d not equal "
//						 "to supported version %d.",
//						 model->version(), TFLITE_SCHEMA_VERSION);
//	return 0;
//	}
//
//
//	tflite::MicroMutableOpResolver<13> resolver;
//	// mcunet ops
//	resolver.AddAdd();
//	resolver.AddAveragePool2D();
//	resolver.AddConv2D();
//	resolver.AddDepthwiseConv2D();
//	resolver.AddPad();
//	resolver.AddRelu6();
//	resolver.AddReshape();
//
//	#ifdef MODEL_TENSOR_ARENA_SDRAM
//	long remainingArena = MODEL_SDRAM_TOTAL_ARENA - MODEL_SDRAM_OFFSET - model_size;
//	if (remainingArena < 0) {
//		kTensorArenaSize = 0;
//	}
//	else {
//		kTensorArenaSize = remainingArena - 32;								// remove 32 bytes for alignment - required can be less than 32 bytes but doesn't matter if we remove extra chunk
//	}
//
//	tensor_arena = g_model_sdram + MODEL_SDRAM_OFFSET + model_size + 32;
////	void *ptr = ((uintptr_t)mem+15) & ~ (uintptr_t)0x0F;
//	tensor_arena = (uint8_t *)( (uintptr_t)tensor_arena & ~(uintptr_t)(0x0F) );						// align pointer on 32 bytes
//	#endif
//
//	*tensor_arena_size = kTensorArenaSize;
//	*tensor_arena_ptr = (uint32_t)tensor_arena;
//
//	// Build an interpreter to run the model with.
//	tflite::MicroInterpreter static_interpreter(
//	  model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
//	interpreter = &static_interpreter;
//
//	// Allocate memory from the tensor_arena for the model's tensors.
//	TfLiteStatus allocate_status = interpreter->AllocateTensors();
//	if (allocate_status != kTfLiteOk) {
//	TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
//	return 0;
//	}


//	uart_buf_len = sprintf(uart_buf, "Allocated Tensors\n");
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
//
	// Obtain pointers to the model's input and output tensors.
	input = interpreter->input(0);
	output = interpreter->output(0);

	char j = 0;

	for (unsigned int i = 0; i < input->bytes; i++) {
		input->data.int8[i] = j;
		j++;
	}

	timer_val = HAL_GetTick(); //
//	timer_val2 = __HAL_TIM_GET_COUNTER(&htim10);

	// Run inference, and report any error
	TfLiteStatus invoke_status = interpreter->Invoke();
	if (invoke_status != kTfLiteOk) {
		TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed");
		return 0;
	}

	timer_val = HAL_GetTick() - timer_val; //
//	timer_val2 = __HAL_TIM_GET_COUNTER(&htim10) - timer_val2;

	#ifdef DEBUG_PRINTS
	uart_buf_len = sprintf(uart_buf, "Inference Time: %u ms\n", timer_val);
	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	uart_buf_len = sprintf(uart_buf, "Output Ptr: %X \n", output->data.data);
	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
	#endif

//	uart_buf_len = sprintf(uart_buf, "Inference Time: %u/10 ms\n", timer_val2);
//	HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);

	void * temp = output->data.data;

	*mem_usage = interpreter->arena_used_bytes();
	out_ptr = (unsigned char *) temp;

	return timer_val;


}
