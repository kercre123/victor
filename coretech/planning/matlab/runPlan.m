% run this on brads machine form the matlab directory

system('rm path.txt');

close all;

rectWorld('test3.env')

plot(0,0,'ro')

[x,y] = ginput(1)

plot(x,y,'kx')

tic
system(sprintf('"/Users/bneuman/Documents/code/products-cozmo/build/Unix Makefiles/bin/Debug/run_coreTechPlanningStandalone" cozmo_mprim.json test3.env %f %f 0', x, y))
toc

S = plotPath('path.txt', true);
