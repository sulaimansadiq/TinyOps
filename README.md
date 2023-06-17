<div align="center">
  
# Enabling ImageNet-Scale Deep Learning on MCUs for Accurate and Efficient Inference

This is the __official__ code for the paper ["Enabling ImageNet-Scale Deep Learning on MCUs for Accurate and Efficient Inference"].

## About

We find that conventional approaches in TinyML that only utilise internal storage and memory yield low accuracy and, surprisingly, high latency. We develop the TinyOps inference framework which utilises external memories and DMA based operation partitioning to open a new model design space for MCUs. We derive efficient models from the TinyOps design space which outperform internal memory models with up to 6.7% higher accuracy and 1.4x faster inference latency, demonstrating the strength of the TinyOps design space.

<img src="pareto_frontier_lat.png"/>

</div>

## Training and Quantisation

We trained our models in PyTorch, followed by standard INT8 post training quantisation 
