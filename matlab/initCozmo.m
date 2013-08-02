% Setup paths
addpath mex

rootDir = fileparts(fileparts(fileparts(which('initCozmo.m'))));

dirs = {'coretech-common', 'coretech-vision', 'coretech-math', 'coretech-externals'};
for i = 1:length(dirs)
    addpath(genpath(fullfile(rootDir, dirs{i}, 'matlab')));
end