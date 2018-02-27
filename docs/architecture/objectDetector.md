## Deep Learning Based Object Detector

### Quick Summary

* Sub-component of the VisionSystem designed to wrap an underlying deep learning / neural network 
* Meant to look for objects in the image and return their name, a confidence, and bounding box
* Has two "modes":
  * "Classification" simply classifies the entire image (and thus returns a single, full-image bounding box)
  * "Detection" attempts to localize (potentially multiple) objects within the scene and return their bounding boxes. The assumption is that this is "harder"
* Because forward inference on neural networks of any useful size is slow, this detector runs asynchronously. Thus VisionProcessingResults containing any detected objects may come in to VisionComponent substantially more delayed than other vision outputs.


---

### Details

The main ObjectDetector class hides a private implementation of a "Model", which wraps the underlying API for interacting with some deep learning library. We have experimented with [TensorFlow](https://www.tensorflow.org/), [TensorFlow Lite](https://www.tensorflow.org/mobile/tflite/), and [OpenCV's DNN Module](https://docs.opencv.org/3.4.0/d6/d0f/group__dnn.html). Long story short, we are currently relying on OpenCV's DNN because it was far easier to integrate (since we already use OpenCV), and until TensorFlow better supports quantization and/or shows a marked speed improvement, OpenCV seems to run just about as fast on the robot. 


See also the [Rock, Paper Scissors project from the 2018 R&D Sprint](https://ankiinc.atlassian.net/wiki/spaces/RND/pages/197591128/Rock+Paper+Scissors+using+Deep+Learning), and the associated slides for more details.


The only private implementation checked into master right now is the OpenCV DNN, but corresponding TensorFlow \[Lite\] implementations exist on a branch. This approach allows us to swap out the underlying library used for inference at build time.

### Asynchrony

Because forward inference through a neural network of any useful depth is slow relative to other vision processing we do, the ObjectDetector is designed to run asynchronously. This detail is handled by the base class (via an `std::future`) so that underlying implementations do not need to worry with it.

The side effect here is that the ObjectDetector is handled slightly differently by the VisionSystem versus other vision sub-components. It also creates its own VisionProcessingResult (with its own timestamp). Note that this result can appear in the "mailbox" in the VisionComponent on the main thread _after_ newer images have already been processed for things like Markers, Faces, etc. 

### Model Storage

Since this system isn't widely used yet, and we don't have a simple solution for checking large files into the repo, currently the assumption is that you've got symlinks to any necessary model files in a `resources/config/engine/vision/dnn_models` directory. Models used by the tests as well as a few others we've tested with are currently available in `Dropbox/VictorDeepLearningModels`, to which you can symlink. For now, if the model files for the test do not exist, the test still _passes_.

The Plan™ is to use `git-lfs` to check the large model files into the repo and not worry about all of the above.

### Looking Ahead

There is interest in sharing an underlying neural network for mobile use cases by feeding its features into a variety of final layers. If we go this route, this system probably should be renamed / re-architected to provide the _features_ as more of a service (in the VisionProcessingResult?) and then allow other systems to use them as they see fit.

It may be preferable / simpler to actually run forward inference through a network as a separate process, instead of having an asynchronous call from an already-asynchronous VisionSystem. That will require a new way to feed the images in and get the results out, but otherwise, the ObjectDetector should be pretty simple to run independently (as is done in the "ObjectDetector.DetectionAndClassification" test in [coreTechVisionTests](../../coretech/vision/test/coreTechVisionTests.cpp)).