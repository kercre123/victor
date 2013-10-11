function dirs = getdirnames(rootPath, pattern, useFullPath)
% Helper function to get a list of directory names, excluding '.' and '..'
%
% dirs = getdirnames(<rootPath>, <pattern>, <useFullPath>)
%
%   Return a cell array of directory names found in the given 'rootPath'
%   which match the specified pattern.   This is just a simple 
%   wrapper around Matlab's "dir" command.
%
%   If unspecified, 'rootPath' defaults to the current directory.  
%
%   If unspecified, 'pattern' defaults to '*' (everything) and the current 
%   and parent directories, "." and "..", are automatically excluded from 
%   the returned list. 
%
%   If 'useFullPath' is true (default is false), then the root path will
%   be added to each directory name so that each entry in the returned list
%   is the full path.
%
% See also: getfnames
% ------------
% Andrew Stein
% 

if nargin == 0 || isempty(rootPath)
    rootPath = '.';
end

if nargin < 2 || isempty(pattern)
    pattern = '*';
end

if nargin < 3 || isempty(useFullPath)
    useFullPath = false;
end

dirs = dir(fullfile(rootPath, pattern));
dirs = {dirs([dirs.isdir]).name};

if strcmp(pattern, '*')
    dirs(1:2) = []; % get rid of '.' and '..' entries
end

if useFullPath
	dirs = cellfun(@(dir)fullfile(rootPath, dir), dirs, ...
        'UniformOutput', false);
end

end % function