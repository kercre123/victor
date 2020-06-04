function files = getfnames(parentDir, pattern, varargin) 
% Simple function for getting filenames matching a pattern in a directory.
%
% filenames = getfnames(<parentDir>, <pattern>, <Param/Value pairs...>)
% 
%  Wrapper for Matlab's 'dir' command to make getting a listof files from 
%  a directory easier.
%
%  If parentDir is unspecified, the current working directory is used.
%
%  If pattern is unspecified, pattern defaults to '*.*'.  A cell array of
%  patterns can be specified and files matching any of those patterns will
%  all be returned, e.g. {'*.jpg', '*.tif', '*.png'}.  If the special
%  string 'images' is used, then common image formats will be used:
%    *.jp(e)g, *.tif(f), *.png, *.pgm, *.ppm, *.bmp
%
%  Options: [default]
%
%  'useFullPath' [false]
%    If true, the filenames will include the path a well. 
%
%  'excludePattern' ['']
%    Pattern for excluding files, analogous to the matching pattern.  If 
%    the special-case string 'hidden' is used, then hidden files (i.e. 
%    those whose filename begins with '.') will be excluded (which may be 
%    more efficient than specifying '.*' as the exclude pattern).
%
%  'recursive' [false]
%    If true, all subdirectories of the parentDir will be traversed 
%    recursively to search for the files matching the pattern.
%
%
% See also: getdirnames
% -------------
% Andrew Stein
%

% Defaults:
recursive = false;
useFullPath = false;
excludePattern = [];

parseVarargin(varargin{:});


if nargin < 2 || isempty(pattern)
    pattern = {'*.*'};
    
elseif ischar(pattern) && strcmpi(pattern, 'images')
	% Common image file formats:
    pattern = {'*.jpg', '*.jpeg', '*.png', '*.pgm', ...
        '*.tif', '*.tiff', '*.ppm', '*.bmp'};
    
    if ~ispc
        %Also include uppercase versions:
        pattern = [pattern cellfun(@upper, pattern, 'UniformOutput', false)];
    end
elseif ~iscell(pattern)
	pattern = {pattern};
end

if nargin < 1 || isempty(parentDir)
    parentDir = pwd;
end

files = cell(size(pattern));
for i_pattern = 1:length(pattern)
	files{i_pattern} = dir(fullfile(parentDir, pattern{i_pattern}));
	files{i_pattern} = {files{i_pattern}(~[files{i_pattern}.isdir]).name}';
end

files = unique(vertcat(files{:}));

if useFullPath
    %files = cellfun(@(file)[parentDir filesep file], files, ...
	files = cellfun(@(file)fullfile(parentDir, file), files, ...
        'UniformOutput', false); %#ok<UNRCH>
end

if ~isempty(excludePattern)
	
	if strcmpi(excludePattern, 'hidden')
		% This ought to be faster for this special case, since it doesn't
		% require a second call to getfnames
		firstChar = cellfun(@(x)x(1), files);
		files = files(firstChar ~= '.');
	else
		% Slower, but more general:
		excludeFiles = getfnames(parentDir, excludePattern, ...
            'useFullPath', useFullPath);
		files = setdiff(files,excludeFiles);
	end
end


if recursive
    
    subdirs = getdirnames(parentDir); %#ok<UNRCH>
    
    % remove subdirectories that match exclude pattern
    if strcmpi(excludePattern,'hidden')
        excludedirs = cellfun(@(x)(x(1)=='.'),subdirs);
        subdirs = subdirs(~excludedirs);
    elseif ~isempty(excludePattern)
        excludedirs = getdirnames(parentDir, excludePattern);
        subdirs = setdiff(subdirs,excludedirs);
    end
    
    for i_dir = 1:length(subdirs)
        files = [files; ...
            getfnames(fullfile(parentDir, subdirs{i_dir}), pattern, ...
            'useFullPath', useFullPath, ...
            'excludePattern', excludePattern, ...
            'recursive', recursive)];
    end
    
end

return
