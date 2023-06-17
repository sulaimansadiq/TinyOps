import tensorflow as tf
import torch
import functools
import numpy as np

import argparse

from collections import OrderedDict

# Training settings
parser = argparse.ArgumentParser()
parser.add_argument('--cfg_path', default='../../assets/configs/test/proxyless-w0.75-r112.json', help='path to model config json')
parser.add_argument('--tflite_path', default='../../assets/configs/test/proxyless-w0.75-r112.tflite', help='path to tflite file for evaluation')
parser.add_argument('--ckpt_path', default='../../assets/configs/test/model_best.pth', help='path to model checkpoint')

# dataset args.
parser.add_argument('--dataset', default='imagenet', type=str)
parser.add_argument('--train_dir', default='../../data/val',
                    help='path to training data')
parser.add_argument('--val_dir', default='../../data/val',
                    help='path to validation data')
parser.add_argument('--batch_size', type=int, default=1,
                    help='input batch size for training')
parser.add_argument('-j', '--workers', default=1, type=int, metavar='N',
                    help='number of data loading workers')

args = parser.parse_args()


def generate_tflite_with_weight(pt_model, resolution, tflite_fname, calib_loader, n_calibrate_sample=500):
    # 1. convert the state_dict to tensorflow format
    pt_sd = pt_model.state_dict()

    tf_sd = {}
    for key, v in pt_sd.items():
        if key.endswith('depth_conv.conv.weight'):
            v = v.permute(2, 3, 0, 1)
        elif key.endswith('conv.weight'):
            v = v.permute(2, 3, 1, 0)
        elif key == 'classifier.linear.weight':
            v = v.permute(1, 0)
        tf_sd[key.replace('.', '/')] = v.numpy()

    # 2. build the tf network using the same config
    weight_decay = 0.

    with tf.Graph().as_default() as graph:
        with tf.compat.v1.Session() as sess:
        # with tf.Session() as sess:
            def network_map(images):
                net_config = pt_model.config
                from tinynas.tf_codebase.tf_model_zoo import ProxylessNASNets
                net_tf = ProxylessNASNets(net_config=net_config, net_weights=tf_sd,
                                          n_classes=pt_model.classifier.linear.out_features,
                                          graph=graph, sess=sess, is_training=False,
                                          images=images, img_size=resolution)
                logits = net_tf.logits
                return logits, {}

            def arg_scopes_map(weight_decay=0.):
                arg_scope = tf.contrib.framework.arg_scope
                with arg_scope([]) as sc:
                    return sc

            slim = tf.contrib.slim

            @functools.wraps(network_map)
            def network_fn(images):
                arg_scope = arg_scopes_map(weight_decay=weight_decay)
                with slim.arg_scope(arg_scope):
                    return network_map(images)

            input_shape = [1, resolution, resolution, 3]
            placeholder = tf.placeholder(name='input', dtype=tf.float32, shape=input_shape)

            out, _ = network_fn(placeholder)

            # 3. convert to tflite (with int8 quantization)
            converter = tf.lite.TFLiteConverter.from_session(sess, [placeholder], [out])
            converter.optimizations = [tf.lite.Optimize.DEFAULT]
            converter.inference_output_type = tf.int8
            converter.inference_input_type = tf.int8

            def representative_dataset_gen():

                for i_b, (data, _) in enumerate(calib_loader):
                    if i_b == n_calibrate_sample:
                        break
                    data = data.numpy().transpose(0, 2, 3, 1).astype(np.float32)
                    yield [data]

            converter.representative_dataset = representative_dataset_gen
            converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

            tflite_buffer = converter.convert()
            tf.gfile.GFile(tflite_fname, "wb").write(tflite_buffer)


if __name__ == '__main__':
    # a simple script to convert the model to
    import sys
    sys.path.append('.')
    import json

    cfg_path = args.cfg_path
    ckpt_path = args.ckpt_path
    tflite_path = args.tflite_path

    from tinynas.nn.networks import ProxylessNASNets

    cfg = json.load(open(cfg_path))
    model = ProxylessNASNets.build_from_config(cfg)
    # model = torch.nn.DataParallel(model)

    if ckpt_path != 'None':
        sd = torch.load(ckpt_path, map_location='cpu')

        new_state_dict = OrderedDict()
        for k, v in sd['state_dict'].items():
            name = k[7:]  # remove `module.`
            new_state_dict[name] = v
        # load params
        sd['state_dict'] = new_state_dict

        model.load_state_dict(sd['state_dict'])

    # prepare calib loader
    # calibrate the model for quantization
    from torchvision import datasets, transforms
    train_dataset = datasets.ImageFolder(args.train_dir,
                                         transform=transforms.Compose([
                                             # transforms.Resize(int(resolution * 256 / 224)),
                                             # transforms.CenterCrop(resolution),
                                             transforms.RandomResizedCrop(cfg['resolution']),
                                             transforms.RandomHorizontalFlip(),
                                             transforms.ToTensor(),
                                             transforms.Normalize(mean=[0.5, 0.5, 0.5], std=[0.5, 0.5, 0.5])
                                         ]))
    train_loader = torch.utils.data.DataLoader(
        train_dataset, batch_size=1,
        shuffle=True, num_workers=4)

    generate_tflite_with_weight(model, cfg['resolution'], tflite_path, train_loader,
                                n_calibrate_sample=500)


    def process(image, label):

        image = tf.cast((image/128)-1, tf.float32)

        label = label+1
        return image, label


    def get_val_dataset(resolution):
        # NOTE: we do not use normalization for tf-lite evaluation; the input is normalized to 0-1
        kwargs = {'num_workers': args.workers, 'pin_memory': False}
        if args.dataset == 'imagenet':
            val_dataset = \
                datasets.ImageFolder(args.val_dir,
                                     transform=transforms.Compose([
                                         transforms.Resize(int(resolution * 256 / 224)),
                                         transforms.CenterCrop(resolution),
                                         transforms.ToTensor(),
                                     ]))
        else:
            raise NotImplementedError
        val_loader = torch.utils.data.DataLoader(
            val_dataset, batch_size=args.batch_size,
            shuffle=False, **kwargs)
        return val_loader


    def eval_image(data):
        image, target = data
        if len(image.shape) == 3:
            image = image.unsqueeze(0)
        image = image.permute(0, 2, 3, 1)
        image_np = image.cpu().numpy()
        image_np = (image_np * 255 - 128).astype(np.int8)
        interpreter.set_tensor(
            input_details[0]['index'], image_np.reshape(*input_shape))
        interpreter.invoke()
        output_data = interpreter.get_tensor(
            output_details[0]['index'])
        output = torch.from_numpy(output_data).view(1, -1)
        from utils import accuracy
        acc1, acc5 = accuracy(output, target.view(1), topk=(1, 5))

        return acc1.item(), acc5.item()


    interpreter = tf.lite.Interpreter(model_path=args.tflite_path)
    interpreter.allocate_tensors()

    # get input & output tensors
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    input_shape = input_details[0]['shape']
    resolution = input_shape[1]

    # we first cache the whole test set into memory for faster data loading
    print(' * start caching the test set...', end='')
    val_loader = get_val_dataset(resolution)  # range [0, 1]
    # val_loader_cache = [v for v in val_loader]
    # images = torch.cat([v[0] for v in val_loader_cache], dim=0)
    # targets = torch.cat([v[1] for v in val_loader_cache], dim=0)
    #
    # val_loader_cache = [[x, y] for x, y in zip(images, targets)]
    # print('done.')
    # print(' * dataset size:', len(val_loader_cache))
    #
    # # use multi-processing for faster evaluation
    # n_thread = 32
    # from multiprocessing import Pool
    # from tqdm import tqdm
    #
    # p = Pool(n_thread)
    correctness1 = []
    correctness5 = []

    # pbar = tqdm(p.imap_unordered(eval_image, val_loader_cache), total=len(val_loader_cache),
    #             desc='Evaluating...')

    # for (acc1, acc5) in enumerate(val_loader):

    for i, data in enumerate(val_loader):

        acc1, acc5 = eval_image(data)

        correctness1.append(acc1)
        correctness5.append(acc5)

        if (i%100) == 0:
            print(i)
            print('* top1: {:.2f}%, top5: {:.2f}%'.format(
                sum(correctness1) / len(correctness1),
                sum(correctness5) / len(correctness5)
            ))

    print('* top1: {:.2f}%, top5: {:.2f}%'.format(
        sum(correctness1) / len(correctness1),
        sum(correctness5) / len(correctness5)
    ))
