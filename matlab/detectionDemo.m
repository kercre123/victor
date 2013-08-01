% detectionDemo

CameraCapture('fps', 30, 'processFcn', @detectAndDisplay, ...
    'doContinuousProcessing', true);