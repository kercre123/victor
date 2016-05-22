# script to drive around manually. Run it directly from the matlab dir

disp('Please cd to the build dir and run:')
disp('Unix\ Makefiles/bin/Debug/run_coreTechPlanningStandalone ../coretech/planning/matlab/cozmo_mprim.json ../coretech/planning/matlab/test3.env')

system('rm /Users/bneuman/Documents/code/products-cozmo/build/path.txt');

close all;

rectWorld('test3.env')

while !exist('/Users/bneuman/Documents/code/products-cozmo/build/path.txt', 'file')
      sleep (0.25)
end

S = plotPath('/Users/bneuman/Documents/code/products-cozmo/build/path.txt', false);

while true
      S = plotPath('/Users/bneuman/Documents/code/products-cozmo/build/path.txt', true, S);
      sleep (0.1)
end

