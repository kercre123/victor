% Setup paths
cozmoDir = fileparts(which('initCozmo.m')); % includes 'matlab'
addpath(fullfile(cozmoDir, 'mex'));

rootDir = fileparts(fileparts(cozmoDir));

dirs = {'coretech-common', 'coretech-vision', 'coretech-math', ...
    'coretech-external', 'products-cozmo'};
for i = 1:length(dirs)
    addpath(genpath(fullfile(rootDir, dirs{i}, 'matlab')));
end