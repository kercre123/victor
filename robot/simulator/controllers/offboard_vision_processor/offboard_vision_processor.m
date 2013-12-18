TIME_STEP = 64;

% Set up paths for BlockWorld to work (do this before creating the
% BlockWorldWebotController object, which may rely on things in the Cozmo
% path)
run(fullfile('..', '..', '..', '..', 'matlab', 'initCozmoPath')); 

wb_robot_init();

usbRX = wb_robot_get_device('USB_RX');
usbTX = wb_robot_get_device('USB_TX');
simSerial = SimulatedSerial('RX', usbRX, 'TX', usbTX);

proc = CozmoVisionProcessor('SerialDevice', simSerial, 'DoEndianSwap', false);

proc.Run();
