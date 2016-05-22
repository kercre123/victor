function initCommonPath

commonDir = fileparts(which(mfilename)); % includes 'matlab'
addpath(genpath(commonDir));
addpath(fullfile(commonDir, '..', 'build', 'mex'));

end % initCommonPath()