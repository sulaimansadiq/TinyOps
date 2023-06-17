# Code from https://github.com/mit-han-lab/once-for-all/blob/a617982d4fecc191c9355f4e2c84b3deaf749042/ofa/utils/flops_counter.py
import numpy as np
import torch
import torch.nn as nn
# from tinynas.nn.networks import MobileInvertedResidualBlock
# from tinynas.nn.modules.layers import MBInvertedConvLayer

__all__ = ['profile']


def count_convNd(m, x, y):
    cin = m.in_channels

    kernel_ops = m.weight.size()[2] * m.weight.size()[3]
    ops_per_element = kernel_ops
    output_elements = y.nelement()

    # cout x oW x oH
    total_ops = cin * output_elements * ops_per_element // m.groups
    m.total_ops = torch.zeros(1).fill_(total_ops)

    m.layer_cfg = {"resin": x[0].shape[2] + (m.weight.size()[2]//2)*2,    # get padding size and pad on top and bottom
                   "resin_nopad": x[0].shape[2],
                   "resout": y.shape[2],
                   "cin": x[0].shape[1],
                   "cout": y.shape[1],
                   "ks": m.weight.size()[2],
                   "stride": m.stride[0],
                   "groups": m.groups,
                   "macs": total_ops}

    # print("Conv, " + str(m.total_ops.item()))


def count_linear(m, x, y):
    total_ops = m.in_features * m.out_features

    m.total_ops = torch.zeros(1).fill_(total_ops)

    m.layer_cfg = {"resin": 1,
                   "resout": 1,
                   "cin": m.in_features,
                   "cout": m.out_features,
                   "ks": 1,
                   "stride": 1,
                   "groups": 1,
                   "macs": total_ops}

    # print("Linear, " + str(m.total_ops.item()))


def count_mbinvconv(m, x, y):
    m.layer_cfg = {"resin": x[0].shape[2]}

    # print("")

register_hooks = {
    nn.Conv1d: count_convNd,
    nn.Conv2d: count_convNd,
    nn.Conv3d: count_convNd,
    ######################################
    nn.Linear: count_linear,
    ######################################
    # MobileInvertedResidualBlock: count_mbinvconv,
    # nn.Identity,
    nn.Dropout: None,
    nn.Dropout2d: None,
    nn.Dropout3d: None,
    nn.BatchNorm2d: None,
}


def profile(model, input_size, custom_ops=None):
    handler_collection = []
    custom_ops = {} if custom_ops is None else custom_ops

    def add_hooks(m_):
        if len(list(m_.children())) > 0:
            return

        m_.register_buffer('total_ops', torch.zeros(1))
        m_.register_buffer('total_params', torch.zeros(1))

        for p in m_.parameters():
            m_.total_params += torch.zeros(1).fill_(p.numel())

        m_type = type(m_)
        fn = None

        if m_type in custom_ops:
            fn = custom_ops[m_type]
        elif m_type in register_hooks:
            fn = register_hooks[m_type]

        if fn is not None:
            _handler = m_.register_forward_hook(fn)
            handler_collection.append(_handler)

    original_device = model.parameters().__next__().device
    training = model.training

    model.eval()
    model.apply(add_hooks)

    x = torch.zeros(input_size).to(original_device)
    x = torch.randn(input_size).to(original_device)
    with torch.no_grad():
        model(x)

    total_ops = 0
    total_params = 0
    layer_cfgs = []

    for m in model.modules():
        if len(list(m.children())) > 0:  # skip for non-leaf module
            continue
        total_ops += m.total_ops
        total_params += m.total_params
        if isinstance(m, nn.Conv2d) or isinstance(m, nn.Linear):
            layer_cfgs.append(m.layer_cfg)
    # np.save("mbv3-w1.00-r224-layers.npy", layer_configs[1:,:])
    total_ops = total_ops.item()
    total_params = total_params.item()

    model.train(training).to(original_device)
    for handler in handler_collection:
        handler.remove()

    return total_ops, total_params, layer_cfgs
