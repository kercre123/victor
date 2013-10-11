function initVisionPath

visionDir = fileparts(which(mfilename)); % includes 'matlab'
addpath(genpath(visionDir));
addpath(fullfile(visionDir, '..', 'build', 'mex'));

end % initVisionPath()