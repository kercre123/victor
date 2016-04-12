
cd '/Users/Kevin/engine/simulator/controllers/webotsCtrlGameEngine/temp/imu_logs';
filename = 'imuRawLog_chair360_wheels150.dat';

delimiterIn = ' ';
headerLinesIn = 1;
A = importdata(filename, delimiterIn, headerLinesIn);

%%%%% Accelerometer data %%%%%
figure(1);
plot(A.data(:,1:3));
title('Accel');


%%%%%%%%%% Gyro data %%%%%%%%%
figure(2);
subplot(2,1,1);
plot(A.data(:,4:6));
ylabel('raw');
title('Gyro');

% Bias compensation
rawGyroZBias = 0; % deg/s

% Duration of a timestep
mainExecTimestep_ms = 5;

% Conversion factor for raw to degrees/s
gyro_range_const = (1/65.6); %*(pi/180);

% Conversion to degrees per timestemp
degreesPerTimestep = A.data(:,6) * gyro_range_const * (mainExecTimestep_ms * 0.001);

% Plot cumulative sum to show current angle assuming 0 degree starting
% orientation
integratedGyroZ = cumsum(degreesPerTimestep - (rawGyroZBias * mainExecTimestep_ms * 0.001) );
subplot(2,1,2);
plot(integratedGyroZ);
ylabel('orientation (deg)');


%%%%% Timestamp data %%%%%
figure(3);
plot(A.data(:,7));
title('timestamp');

% Check for non-toggling MSB on timestamp
% This indicates drop of dup
t = A.data(:,7) - 127;
dups=[];
for i=2:length(t)
    dups = [dups (t(i) * t(i-1) > 0)*128];
end

hold on;
plot(dups, 'r');
hold off;