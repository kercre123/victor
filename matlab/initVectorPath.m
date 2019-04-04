function initVectorPath

victorDir = fileparts(fileparts(which(mfilename))); % excludes 'matlab'
addpath(genpath(fullfile(victorDir, 'matlab')));
% addpath(fullfile(cozmoDir, 'build', 'mex'));
addpath(genpath(fullfile(victorDir, 'systemTests')));

coretechLibs = dir(fullfile(victorDir, 'coretech'));
coretechLibs(~[coretechLibs.isdir]) = []; % keep only directories
coretechLibs(1:2) = []; % remove . and .. entries
coretechLibs = {coretechLibs.name};

for i = 1:length(coretechLibs)
    matlabDir = fullfile(victorDir, 'coretech', coretechLibs{i}, 'matlab');
    if isdir(matlabDir)
        addpath(genpath(matlabDir));
    end
end

end % initVectorPath()
