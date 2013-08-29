function initVisionPath

visionDir = fileparts(which(mfilename)); % includes 'matlab'
addpath(genpath(visionDir));

end % initVisionPath()