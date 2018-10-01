## Neural Net Runner / Model

### Quick Summary

* Forward inference through neural nets run as their own process, `vic-neuralnets`
  - NeuralNetRunner is a sub-component of the VisionSystem which communicates with the underlying NeuralNetModel and abstracts away the additional level of asynchrony from the engine's point of view.
  - We have wrappers for OpenCV's DNN module (deprecated), TensorFlow, and TensorFlow Lite (TFLite). TFLite is the current default.
  - We have our own ["private fork" of TensorFlow](https://github.com/anki/tensorflow), which has an `anki` branch with an `anki` subdirectory containing our special build scripts (e.g. for building a `vicos`-compatible version of TensorFlow / TFLite).
* Parameters of the underlying network model are configured in their own section of `vision_config.json`
* NeuralNets return instances of SalientPoints, which are centroids of "interesting" things in an image, with some associated information (like confidence, shape/bounding box, and type). These are stored in the SalientPointComponent in the engine, for the behaviors to check and act upon.

---

### Details

In the VisionSystem, we have a NeuralNetRunner, which wraps the underlying system actually doing forward inference with the neural net. This means that the rest of the engine / VisionSystem does not need to know whether the neural net is running in a separate process (as is the current approach) or as part of the same thread (as we used to do), and this class handles the additional asynchrony for us (using a `std::future`). The current underlying "model" it uses is a "messenger", which communicates with the standalone process, `vic-neuralnets`. Currently this is using "poor man's" interprocess communication (IPC): a.k.a. the file system. That file system is currently memory-mapped to improve performance, but we intend to switch to a shared memory approach soon.

A side effect of the additional asynchrony here is that the NeuralNetRunner is handled slightly differently by the VisionSystem versus other vision sub-components. SalientPoints may appear in a VisionProcessingResult for an image well after the one used for forward inference through the network. Thus the SalientPoints' timestamps may not match the VisionProcessingResult's timestamp. I.e., the SalientPoints may not be from the same image as markers, faces, etc, returned in the same VisionProcessingResult.

The neural net process has a NeuralNetModel, which implements an INeuralNetModel interface. Currently, we have two implementations: one for "full" [TensorFlow](https://www.tensorflow.org/) models, and one for [TensorFlow Lite](https://www.tensorflow.org/mobile/tflite/) models. The latter is far better optimized and supports quantization, and is the one we use in production.

Note that both `vic-neuralnets`, which runs on the robot, and `webotsCtrlNeuralnets`, which is the corresponding Webots controller for running neural nets in simulation, use a common base class, INeuralNetMain. That class handles the IPC, i.e. polling for new image files to process and writing SalientPoint outputs from the neural net model to disk for the NeuralNetRunner in the VisionSystem to pick up.

See also the [Rock, Paper Scissors project from the 2018 R&D Sprint](https://ankiinc.atlassian.net/wiki/spaces/RND/pages/197591128/Rock+Paper+Scissors+using+Deep+Learning), and the associated slides for more details.

### Model Parameters

In the "NeuralNets" field of `vision_config.json`, you can specify several parameters of the model you want to load. See NeuralNetParams for additional info beyond the following.

* `graphFile` - The graph representing the model you want to load, assumed to be in `resources/config/engine/vision/dnn_models` (see also the next section about model storage).
  - `<filename.{pb|tflite}>` will load a TensorFlow (Lite) model stored in a Google protobuf file
  - `<modelname>` (without extension) assumes you are specifying a Caffe model and will load the model from `<modelname>.prototxt` and `<modelname>.caffemodel`, which must _both_ exist.  
* `labelsFile` - The filename containing the labels for translating numeric values returned by the classifier/detector into string names: one label on each line. Assumed to be in the same directory as the model file. Note that the the "background" class is assumed to be the zero label and is not required in the file. So there are actually N-1 rows. So for a binary classifier (Class A vs. Background), you'd just have one row: "Class A".
* `inputWidth` and `inputHeight` - The resolution at which to process the images (should generally match what the model expects). 
* `outputType` - What kind of output the network produces. Four output types are currently supported:
  * "Classification" simply classifies the entire image
  * "BinaryLocalization" uses a coarse grid of classifiers and does connected-components to return a list of SalientPoints, one for each connected components. This is the mode the current Person Detector uses. 
  * "AnchorBoxes" interprets Single Shot Detector (SSD) style outputs (bounding boxes)
  * "Segmentation" is a work in progress, but is designed to support networks which output classifications per pixel.
* `inputScale` and `inputShift` - When input to graph is _float_, data is first divided by scale and then shifted. I.e.:  `float_input = data / scale + shift`


### Model Storage

Model files are stored using git large file storage (LFS). Check them into `resources/config/engine/vision/dnn_models` as normal. There's a filter built into our repo that will automatically use LFS for `.pb` and `.tflite` files.


