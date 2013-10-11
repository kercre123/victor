function initExternalPath

externalDir = fileparts(which(mfilename)); % includes 'matlab'
addpath(genpath(externalDir));

end % initExternalPath()