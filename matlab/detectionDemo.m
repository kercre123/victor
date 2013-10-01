function detectionDemo(varargin)
% Helper to run detectAndDisplay with CameraCapture.  Passes extra args to CameraCapture.

CameraCapture('processFcn', @detectAndDisplay, ...
    'doContinuousProcessing', true, varargin{:});