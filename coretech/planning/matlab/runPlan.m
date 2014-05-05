% run this on brads machine form the matlab directory

system('rm -f path.txt');

close all;

rectWorld('test3.env');

mprim = "cozmo_mprim.json"
%mprim = "cozmo_mprim_expensiveTurns.json"

plot(0,0,'ro');

[x,y] = ginput(2);

plot(x(1),y(1),'ko');
plot(x,y,'k-');

theta = atan2(diff(y), diff(x));

tic
system(sprintf('"/Users/bneuman/Documents/code/products-cozmo/build/Unix Makefiles/bin/Debug/run_coreTechPlanningStandalone" %s test3.env %f %f %f', mprim, x(1), y(1), theta));
toc

S = plotPath('path.txt', true);
