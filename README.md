<div align="center">
  
# Enabling ImageNet-Scale Deep Learning on MCUs for Accurate and Efficient Inference

This is the __official__ code for the paper ["Enabling ImageNet-Scale Deep Learning on MCUs for Accurate and Efficient Inference"].

## About

We find that conventional approaches in TinyML that only utilise internal storage and memory yield low accuracy and, surprisingly, high latency. We develop the TinyOps inference framework which utilises external memories and DMA based operation partitioning to open a new model design space for MCUs. We derive efficient models from the TinyOps design space which outperform internal memory models with up to 6.7% higher accuracy and 1.4x faster inference latency, demonstrating the strength of the TinyOps design space.

<img src="pareto_frontier_lat.png"/>

</div>

## Training, Quantisation

We trained our models in PyTorch, followed by standard INT8 post training quantisation using Tensorflow

### Requirement

- Python 3.6+
- PyTorch 1.4.0+
- Tensorflow 1.15

To train the models, run

```bash
python 
-u Train.TinyOps.TfPad/imagenet.py 
-a proxyless 
-d datasets/imagenet_2012/ 
--epochs 150 
--lr-decay cos 
--lr 0.05 
--wd 4e-5 
--net_config mbv3-w0.50-r128.json 
-c mbv3-w0.50-r128/
```

The models trained in PyTorch can then be quantised using standard post-training INT8 quantisation in tensorflow by running

```bash
python 
-u Train.TinyOps.TfPad/gen_eval_tflite.py --tflite_path mbv3-w0.55-r128_int8.tflite 
--train_dir imagenet_2012/train 
--val_dir imagenet_2012/val 
--cfg_path mbv3-w0.55-r128.json 
--ckpt_path mbv3-w0.55-r128/model_best.pth.tar


```

## Model Deployment

The quantised models can be deployed with the TinyOps inference framework by converting the *.tflite files to C arrays which are copied into the project directory. Please refer to `STM32F746NG_DISCO_MINIMAL_CMSIS/Core/Src/tensorflow/lite/micro/examples/hello_world/Validated_Models/ImgNet_Models/mbv3-w1.00-r160_int8_1.cc`

