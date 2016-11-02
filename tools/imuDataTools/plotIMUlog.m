
cd '/Users/kevin/src/cozmo/cozmo-copy/build/mac/Debug/playbackLogs/webotsCtrlGameEngine/imu_logs';
filename = 'imuRawLog_1.dat';

delimiterIn = ' ';
headerLinesIn = 1;
A = importdata(filename, delimiterIn, headerLinesIn);

%%%%% Accelerometer data %%%%%
figure(1);
plot(A.data(:,1:3));
title('Accel');


% Display accelerometer magnitude (squared), (mm/s^2)^2
figure(4);
A.data(:,1:3);
accData = A.data(:,1:3);
accSum = [];
ACC_RANGE_CONST  = (1.0 / 16384.0) * 9810.0; 
for i=1:size(accData,1)
    accSum = [accSum, (accData(i,1)*accData(i,1) + accData(i,2)*accData(i,2) + accData(i,3)*accData(i,3)) * (ACC_RANGE_CONST^2)];
    
end
plot(accSum);
title('AccSum');



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