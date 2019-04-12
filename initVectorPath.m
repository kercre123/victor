function initVectorPath

victorDir = fileparts(which(mfilename)); % excludes 'matlab'
run(fullfile(victorDir, 'matlab', 'initVectorPath.m'));

end 