% File:   imuLogTool.m
% Author: Kevin Yoon
% Date:   2014-08-01
%
% Description: Script for viewing IMU logs from the robot.
%

clear all;

% Remember folder of the script so we can change back to it at the end
origFolder = fileparts(mfilename('fullpath'));

% Go to folder where the data is and load it
cd('../simulator/controllers/blockworld_controller/imu_logs/');

% This is the name of the file containing the imu data
robot34_imu5

% Copy to more readable variables
ax = imuData0;
ay = imuData1;
az = imuData2;
gx = imuData3;
gy = imuData4;
gz = imuData5;

% Display data in graphs
len = length(ax);
ymin = -140;
ymax = 140;

figure(3);
subplot(3,2,1);
plot(ax);
ylabel('ax');
axis([0 len ymin ymax]);

subplot(3,2,3);
plot(ay);
ylabel('ay');
axis([0 len ymin ymax]);

subplot(3,2,5);
plot(az);
ylabel('az');
axis([0 len ymin ymax]);

subplot(3,2,2);
plot(gx);
ylabel('gx');
axis([0 len ymin ymax]);

subplot(3,2,4);
plot(gy);
ylabel('gy');
axis([0 len ymin ymax]);

subplot(3,2,6);
plot(gz);
ylabel('gz');
axis([0 len ymin ymax]);


% ===== Scratch area: Try to detect pickup ======

detectWindow = 100;
avgWindowSize = 10;

% Generate features
a_totalSq = ax.*ax + ay.*ay + az.*az;
cumSumTotal = cumsum(a_totalSq);
diffZ = diff(az);
 
convFilt = ones(1,avgWindowSize) / avgWindowSize;
convA = conv(az, convFilt, 'valid');

diffA = zeros(1, length(convA));
diffA(find( diff(convA) > 0)) = 1;

pickup = zeros(length(ax));
%...


figure(10);
subplot(4,1,1);
%plot(a_totalSq);
%ylabel('a_totalSq');
plot(az);
ylabel('az');

subplot(4,1,2);
plot(convA);
ylabel('convA');

subplot(4,1,3);
plot(diffA);
ylabel('diffA');

subplot(4,1,4);
plot(pickup);
ylabel('pickup');

% =============================================

% Go back to folder that contains this script
cd(origFolder);