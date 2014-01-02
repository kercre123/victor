function SetCalibration(this, packet, whichCalibration, atResolution)

% Expecting:
% struct {
%   f32 focalLength_x, focalLength_y, fov_ver;
%   f32 center_x, center_y;
%   f32 skew;
%   u16 nrows, ncols;
% }
%

f    = this.Cast(packet(1:8),   'single');
c    = this.Cast(packet(13:20), 'single');
dims = this.Cast(packet(25:28), 'uint16');

assert(length(f) == 2, ...
    'Expecting two single precision floats for focal lengths.');

assert(length(c) == 2, ...
    'Expecting two single precision floats for camera center.');

assert(length(dims) == 2, ...
    'Expecting two 16-bit integers for calibration dimensions.');

calibrationMatrix = double([f(1) 0 c(1); 0 f(2) c(2); 0 0 1]);


% Compare the calibration resolution to the tracking
% resolution and since it will only be used on data at
% tracking resolution, adjust it accordingly
adjFactor = atResolution ./ double([dims(2) dims(1)]);
assert(adjFactor(1) == adjFactor(2), ...
    'Expecting calibration scaling factor to match for X and Y.');
calibrationMatrix(1:2,:) = calibrationMatrix(1:2,:) * adjFactor(1);
this.(whichCalibration) = calibrationMatrix;

this.StatusMessage(1, 'Camera calibration "%s" set (scaled by %.1f): \n', ...
    whichCalibration, adjFactor(1));
disp(this.(whichCalibration));

end % SetCalibration()
