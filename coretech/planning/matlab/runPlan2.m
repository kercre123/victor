% run this on brads machine form the matlab directory

system('rm -f path.txt');

close all;

rectWorld('realtest.env');

mprim = "cozmo_mprim.json"
% mprim = "newPrim.json"
%mprim = "cozmo_mprim_expensiveTurns.json"

[startX,startY] = ginput(2);

plot(startX(1),startY(1),'ro');
plot(startX,startY,'r-');

startTheta = atan2(diff(startY), diff(startX));

[endX,endY] = ginput(2);

plot(endX(1),endY(1),'ko');
plot(endX,endY,'k-');

endTheta = atan2(diff(endY), diff(endX));

tic
command = sprintf('"/Users/bneuman/Documents/code/products-cozmo/build/Unix Makefiles/bin/Debug/run_coreTechPlanningStandalone" %s realtest.env %f %f %f %f %f %f', mprim, endX(1), endY(1), endTheta, startX(1), startY(1), startTheta)
system(command);
toc

S = plotPath('path.txt', true);
