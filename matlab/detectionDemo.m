function detectionDemo(varargin)
% Helper to run detectAndDisplay with CameraCapture.  Passes extra args to CameraCapture.

MarkerLibrary = [];

CamCapArgs = parseVarargin(varargin{:});

CameraCapture('processFcn', @(a,b,c)detectAndDisplay(a,b,c,'markerLibrary', MarkerLibrary), ...
    'doContinuousProcessing', true, CamCapArgs{:});