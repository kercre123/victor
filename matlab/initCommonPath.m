function initCommonPath

commonDir = fileparts(which(mfilename)); % includes 'matlab'
addpath(genpath(commonDir));

end % initCommonPath()