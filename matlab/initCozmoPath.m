function initCozmoPath

cozmoDir = fileparts(which(mfilename)); % includes 'matlab'
addpath(genpath(cozmoDir));
addpath(fullfile(cozmoDir, '..', 'build', 'mex'));

rootDir = fileparts(fileparts(cozmoDir));

% Initialize each coretech library we need:
% (Capitalize first letter!)
coretechLibs = {'Common', 'Vision', 'External'};

for i = 1:length(coretechLibs)
    run(fullfile(rootDir, sprintf('coretech-%s', lower(coretechLibs{i})), ...
        'matlab', sprintf('init%sPath', coretechLibs{i})));
end

end % initCozmoPath()