% run this on brads machine form the matlab directory

system('rm -f path.txt');

close all;

rectWorld('test3.env');

mprim1 = 'cozmo_mprim.json'
mprim2 = 'cozmo_mprim_expensiveTurns.json'

plotRobot(0, 0, 0);

pause
axis equal;

disp('click twice to define goal and direction')

[x,y] = ginput(2);

plot(x(1),y(1),'ko');
plot(x,y,'k-');

theta = atan2(diff(y), diff(x));

tic
system(sprintf('"/Users/bneuman/Documents/code/products-cozmo/build/Unix Makefiles/bin/Debug/run_coreTechPlanningStandalone" %s test3.env %f %f %f', mprim1, x(1), y(1), theta));
toc

S = plotPath('path.txt', true, {}, 'g.-');

n = ginput(1)

tic
system(sprintf('"/Users/bneuman/Documents/code/products-cozmo/build/Unix Makefiles/bin/Debug/run_coreTechPlanningStandalone" %s test3.env %f %f %f', mprim2, x(1), y(1), theta));
toc

S = plotPath('path.txt', true, {}, 'c.-');

n = ginput(1)

tic
system(sprintf('"/Users/bneuman/Documents/code/products-cozmo/build/Unix Makefiles/bin/Debug/run_coreTechPlanningStandalone" %s test3.env %f %f %f -turnAtGoal', mprim1, x(1), y(1), theta));
toc

S = plotPath('path.txt', true, {}, 'm-');
