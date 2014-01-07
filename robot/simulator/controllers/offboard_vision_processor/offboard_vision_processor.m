TIME_STEP = 30;

% Set up paths for BlockWorld to work (do this before creating the
% BlockWorldWebotController object, which may rely on things in the Cozmo
% path)
run(fullfile('..', '..', '..', '..', 'matlab', 'initCozmoPath')); 

wb_robot_init();

usbRX = wb_robot_get_device('USB_RX');
usbTX = wb_robot_get_device('USB_TX');
simSerial = SimulatedSerial('RX', usbRX, 'TX', usbTX, 'TIME_STEP', TIME_STEP);

proc = CozmoVisionProcessor('SerialDevice', simSerial, ...
			    'DoEndianSwap', false, 'FlipImage', false, ...
                'TrackerType', 'homography', ...
                'TIME_STEP', TIME_STEP);

proc.Run();
