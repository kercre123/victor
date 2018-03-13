## Deep Learning Based Object Detector

### Quick Summary

* Sub-component of the VisionSystem designed to wrap an underlying deep learning / neural network 
* Meant to look for objects in the image and return their name, a confidence, and bounding box
* Has two "modes":
  * "Classification" simply classifies the entire image (and thus returns a single, full-image bounding box)
  * "Detection" attempts to localize (potentially multiple) objects within the scene and return their bounding boxes. The assumption is that this is "harder"
* Model parameters specified in `vision_config.json`
* Because forward inference on neural networks of any useful size is slow, this detector runs asynchronously. Thus VisionProcessingResults containing any detected objects may come in to VisionComponent substantially more delayed than other vision outputs.


---

### Details

The main ObjectDetector class hides a private implementation of a "Model", which wraps the underlying API for interacting with some deep learning library. We have experimented with [TensorFlow](https://www.tensorflow.org/), [TensorFlow Lite](https://www.tensorflow.org/mobile/tflite/), and [OpenCV's DNN Module](https://docs.opencv.org/3.4.0/d6/d0f/group__dnn.html). Long story short, we are currently relying on OpenCV's DNN because it was far easier to integrate (since we already use OpenCV), and until TensorFlow better supports quantization and/or shows a marked speed improvement, OpenCV seems to run just about as fast on the robot. 

See also the [Rock, Paper Scissors project from the 2018 R&D Sprint](https://ankiinc.atlassian.net/wiki/spaces/RND/pages/197591128/Rock+Paper+Scissors+using+Deep+Learning), and the associated slides for more details.

The only private implementation checked into master right now is the OpenCV DNN, but corresponding TensorFlow \[Lite\] implementations exist on a branch. This approach allows us to swap out the underlying library used for inference at build time.

### Model Parameters

In the "ObjectDetector" field of `vision_config.json`, you can specify several parameters of the model you want to load.

* `graph` - The graph representing the model you want to load, assumed to be in `resources/config/engine/vision/dnn_models` (see also the next section about model storage).
  - `<filename.pb>` will load a TensorFlow model stored in a Google protobuf file
  - `<modelname>` (without extension) assumes you are specifying a Caffe model and will load the model from `<modelname>.prototxt` and `<modelname>.caffemodel`, which must _both_ exist.  
* `labels` - The filename containing the labels for translating numeric values returned by the classifier/detector into string names: one label on each line. Assumed to be in the same directory as the model file.
* `input_width` and `input_height` - The resolution at which to process the images (should generally match what the model expects). 
* `mode` - Only two currently supported:
  - "detection" - Attempts to extract multiple objects and their bounding boxes from the outputs of the model. Note that this is not super robust and is somewhat hard-coded to SSD-style outputs.
  - "classification" - Assumes the whole image is the object and returns the top-scoring image classification and a bounding box corresponding to the entire image. (For example, use this for simple MobileNet models.)
* `input_mean_R/G/B` and `input_std` - Used to preprocess the data by subtracting the mean and dividing by `std`. Should match the values used to train the model.
* `min_score` - Only return detections/classifications above this threshold, [0,1].

### TF Model Optimization


A "frozen" TensorFlow model likely contains significant overhead that's really only needed during training. When deploying a model for forward inference only, it's helpful to optimize it using this Python script:

```
python <tensorflow_root>/tensorflow/python/tools/optimize_for_inference.py \
  --input=<frozen_graph>.pb \
  --output=<optimized_graph>.pb \
  --input_names=<input_layer_name> \
  --output_names=<comma_separated_output_layer_names>
```

You use TensorFlow's `summarize_graph` tool to help you figure the input/output layer names for the above, if you don't know them (note that you'll need to `bazel build` this tool first):

```
<tensorflow_root>/bazel-bin/tensorflow/tools/graph_transforms/summarize_graph \
  --in_graph=<frozen_graph.pb>
```

OpenCV's DNN module doesn't support all possible layers/architectures in TensorFlow. The following command helps make the model more suitable to run with OpenCV:

```
<tensorflow_root>/bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
  --in_graph=<optimized_graph>.pb \
  --out_graph=<opencvdnn_graph>.pb \
  --input_names=<input_layer_name> \
  --output_names=<comma_separated_output_layer_names> \
  --transforms="fold_constants sort_by_execution_order remove_nodes(op=Squeeze, op=PlaceholderWithDefault)"
```

### Model Storage

Since this system isn't widely used yet, and we don't have a simple solution for checking large files into the repo, currently the assumption is that you've got symlinks to any necessary model files in a `resources/config/engine/vision/dnn_models` directory. Models used by the tests as well as a few others we've tested with are currently available in `Dropbox/VictorDeepLearningModels`, to which you can symlink. For now, if the model files for the test do not exist, the test still _passes_.

The Plan™ is to use `git-lfs` to check the large model files into the repo and not worry about all of the above.

### Asynchrony

Because forward inference through a neural network of any useful depth is slow relative to other vision processing we do, the ObjectDetector is designed to run asynchronously. This detail is handled by the base class (via an `std::future`) so that underlying implementations do not need to worry with it.

The side effect here is that the ObjectDetector is handled slightly differently by the VisionSystem versus other vision sub-components. It also creates its own VisionProcessingResult (with its own timestamp). Note that this result can appear in the "mailbox" in the VisionComponent on the main thread _after_ newer images have already been processed for things like Markers, Faces, etc. 

### Looking Ahead

There is interest in sharing an underlying neural network for mobile use cases by feeding its features into a variety of final layers. If we go this route, this system probably should be renamed / re-architected to provide the _features_ as more of a service (in the VisionProcessingResult?) and then allow other systems to use them as they see fit.

It may be preferable / simpler to actually run forward inference through a network as a separate process, instead of having an asynchronous call from an already-asynchronous VisionSystem. That will require a new way to feed the images in and get the results out, but otherwise, the ObjectDetector should be pretty simple to run independently (as is done in the "ObjectDetector.DetectionAndClassification" test in [coreTechVisionTests](../../coretech/vision/test/coreTechVisionTests.cpp)).
