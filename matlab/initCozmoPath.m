function initCozmoPath

cozmoDir = fileparts(fileparts(which(mfilename))); % excludes 'matlab'
addpath(genpath(fullfile(cozmoDir, 'matlab')));
% addpath(fullfile(cozmoDir, 'build', 'mex'));
addpath(genpath(fullfile(cozmoDir, 'systemTests')));

coretechLibs = dir(fullfile(cozmoDir, 'coretech'));
coretechLibs(~[coretechLibs.isdir]) = []; % keep only directories
coretechLibs(1:2) = []; % remove . and .. entries
coretechLibs = {coretechLibs.name};

for i = 1:length(coretechLibs)
    matlabDir = fullfile(cozmoDir, 'coretech', coretechLibs{i}, 'matlab');
    if isdir(matlabDir)
        addpath(genpath(matlabDir));
    end
end

% For now, coretech-external is special and lives outside this repo, 
% assumed to be in ../coretech-external
run(fullfile(cozmoDir, '..', '..', '..', '..', 'coretech-external', 'matlab', 'initExternalPath.m'));

end % initCozmoPath()