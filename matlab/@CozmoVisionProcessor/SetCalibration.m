function SetCalibration(this, packet)

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

this.calibrationMatrix = double([f(1) 0 c(1); 0 f(2) c(2); 0 0 1]);

% Compare the calibration resolution to the tracking
% resolution and since it will only be used on data at
% tracking resolution, adjust it accordingly
adjFactor = this.trackingResolution ./ double([dims(2) dims(1)]);
assert(adjFactor(1) == adjFactor(2), ...
    'Expecting calibration scaling factor to match for X and Y.');
this.calibrationMatrix = this.calibrationMatrix * adjFactor(1);

this.StatusMessage(1, 'Camera calibration set (scaled by %.1f): \n', adjFactor(1));
disp(this.calibrationMatrix);

end % SetCalibration()
