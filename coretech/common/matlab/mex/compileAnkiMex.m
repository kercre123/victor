function compileAnkiMex(moduleName, sourceFilename, varargin)

compileVerbose = false;
useDebugMode   = false;
parseVarargin(varargin{:});
   
if ~iscell(sourceFilename)
    sourceFilename = {sourceFilename};
end

moduleName = lower(moduleName);
validModules = {'vision', 'common', 'external', 'cozmo'};
if ~any(strcmp(moduleName, validModules))
    error('Unrecognized module "%s".', moduleName);
end

rootDir = fileparts(fileparts(fileparts(fileparts(mfilename('fullpath')))));

mexWrapperDir = fullfile(rootDir, 'coretech-common', 'matlab', 'mex');
    
if ismac
    IDE_name = 'xcode4';
    openCvVersionString = '';
    OSbits = 64;
elseif ispc
    IDE_name = 'Visual Studio 11';
    if useDebugMode
        openCvVersionString = '246d';
    else
        openCvVersionString = '246';
    end
    OSbits = 32;
else
    error('Currently only supporting builds on Mac or PC.');
end
  
flags = {};

if useDebugMode
    configName = 'Debug';
    flags{end+1} = '-g';
    
    if ispc
        flags{end+1} = '-D_DEBUG';
    end
else
    configName = 'RelWithDebInfo';
end

ankiIncludeDirs = {fullfile(rootDir, 'coretech-common', 'include')};
if ispc()
    ankiLibDirs = {fullfile(rootDir, 'coretech-common', 'build/lib', configName)};
else
    ankiLibDirs = {fullfile(rootDir, 'coretech-common', 'build', IDE_name, 'lib')};
end
ankiLibs = {'CoreTech_Common', 'CoreTech_Common_Embedded'};

if compileVerbose
    flags{end+1} = '-v';
end

openCvLibDir = fullfile(rootDir, 'coretech-external', 'build', IDE_name, ...
    'opencv-2.4.6.1', 'lib', configName);

openCvRoot = fullfile(rootDir, 'coretech-external', 'opencv-2.4.6.1');
openCvIncludeDirs = [openCvRoot, getdirnames(fullfile(openCvRoot, 'modules'), '*', true)];
% Unlinked OpenCv modules: (move to used list as needed)
%'features2d', 'legacy', 'ml', 'nonfree', 'objdetect', 'ocl', 
% 'stitching', 'superres', 'videostab', 'flann', 'photo' 
openCvLibs = {'calib3d', 'contrib', 'core', 'highgui', 'imgproc'};
matlabLibs = {'mx', 'eng'};

moduleDir  = fullfile(rootDir, ['coretech-' moduleName]);

if ~strcmp(moduleName, 'common')
    ankiIncludeDirs{end+1} = fullfile(moduleDir, 'include');
    
    if ispc()
        ankiLibDirs{end+1} = fullfile(moduleDir, 'build/lib', configName);
    else
        ankiLibDirs{end+1} = fullfile(moduleDir, 'build', IDE_name, 'lib');
    end
    
    moduleLibName = moduleNameHelper(['coretech-' moduleName]);
    ankiLibs = [ankiLibs {moduleLibName, [moduleLibName '_Embedded']}];
end

% Add -I, -L, etc.
openCvIncludeDirs = prefixSuffixHelper('-I', openCvIncludeDirs, [filesep 'include']);
openCvLibs        = prefixSuffixHelper('-lopencv_', openCvLibs, openCvVersionString);
ankiIncludeDirs   = prefixSuffixHelper('-I', ankiIncludeDirs, '');
ankiLibDirs       = prefixSuffixHelper('-L', ankiLibDirs, '');
if ispc()
    ankiLibs          = prefixSuffixHelper('-l', ankiLibs, '');    
else
    ankiLibs          = prefixSuffixHelper('-l', ankiLibs, sprintf('_%d%s', OSbits, configName));    
end
matlabLibs        = prefixSuffixHelper('-l', matlabLibs, '');

outputDir = fullfile(moduleDir, 'build', 'mex');
if ~isdir(outputDir)
    mkdir(outputDir);
end

numFiles = length(sourceFilename);
for i_file = 1:numFiles
    try
        fprintf('Building %d of %d "%s"...\n', i_file, numFiles, sourceFilename{i_file});
       
            mex(flags{:}, '-compatibleArrayDims',  sourceFilename{i_file},...
            fullfile(mexWrapperDir, 'mexWrappers.cpp'), ...
            ankiIncludeDirs{:}, openCvIncludeDirs{:}, ankiLibDirs{:}, ankiLibs{:}, matlabLibs{:}, ...
            ['-I', mexWrapperDir], ['-L', openCvLibDir], openCvLibs{:}, ...
            '-outdir', outputDir);
    catch E
       warning('Failed to build "%s": %s', sourceFilename{i_file}, E.message); 
    end
    
end % FOR each sourceFilename

% Make sure we get rid of .o files
% (Why do i need to do this? Shouldn't the mex call clean up after itself??)
delete(fullfile(outputDir, '*.o'));
    
end % function compileAnkiMex(filename)

function cellArray = prefixSuffixHelper(prefix, cellArray, suffix)
cellArray = cellfun(@(name) [prefix name suffix], cellArray, 'UniformOutput', false);
end

function name = moduleNameHelper(name)
name = strrep(name, '-', '_');
name = strrep(name, 'coretech', 'CoreTech');
name(10) = upper(name(10));
end
