#!/bin/bash 

# https://github.com/opencv/opencv/pull/9517#issue-138385178

cd mobilenet

python ~/tensorflow/tensorflow/python/tools/freeze_graph.py --input_graph=mobilenet_v1.pb --input_checkpoint=mobilenet_v1_1.0_224.ckpt --output_graph=mobilenet_v1_frozen_softmax.pb --output_node_names=MobilenetV1/Predictions/Softmax --input_binary

 ~/tensorflow/bazel-bin/tensorflow/tools/graph_transforms/transform_graph --in_graph=mobilenet_v1_frozen_softmax.pb --out_graph=mobilenet_v1_for_dnn_softmax.pb --inputs=input --outputs=MobilenetV1/Predictions/Softmax --transforms="fold_constants sort_by_execution_order remove_nodes(op=Squeeze)"
