function initCozmoPath

cozmoDir = fileparts(which(mfilename)); % excludes 'matlab'
addpath(genpath(fullfile(cozmoDir, 'matlab')));
%addpath(fullfile(cozmoDir, 'build', 'cmake-mac', 'lib', 'anki', 'cozmo-engine', 'mex'));

% addpath(genpath(fullfile(cozmoDir, 'systemTests')));

% coretechLibs = dir(fullfile(cozmoDir, 'coretech'));
% coretechLibs(~[coretechLibs.isdir]) = []; % keep only directories
% coretechLibs(1:2) = []; % remove . and .. entries
% coretechLibs = {coretechLibs.name};

% for i = 1:length(coretechLibs)
%   matlabDir = fullfile(cozmoDir, 'coretech', coretechLibs{i}, 'matlab');
%   if isdir(matlabDir)
%     addpath(genpath(matlabDir));
%   end
% end

% For now, coretech-external is special and lives outside this repo, 
% assumed to be in ../coretech-external
%run(fullfile(cozmoDir, '..', 'coretech-external', 'matlab', 'initExternalPath.m'));

run(fullfile(cozmoDir, 'matlab', 'initCozmoPath.m'));

% Figure out the nonsensical build products directory :-/
buildProductRoot = getdirnames(fullfile(cozmoDir, 'build', 'mac', 'derived-data'), 'CozmoWorkspace_MAC-*', true);
assert(iscell(buildProductRoot) && length(buildProductRoot)==1, ...
  'Failed to find single build product root.');

% This is where mex products end up
addpath(fullfile(buildProductRoot{1}, 'Build', 'Products', 'Debug'));

end % initCozmoPath()