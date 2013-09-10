
% function compileAnkiMex(filename)

function compileAnkiMex(sourceFilename)

compileVerbose = '-v';
    
rootDir = fileparts(fileparts(fileparts(fileparts(which('makeMex.m')))));
rootDir(rootDir == '\') = '/';
mexWrapperDir = fullfile(rootDir, 'coretech-common', 'matlab', 'mex');
mexWrapperDir(mexWrapperDir == '\') = '/';
ankiLibraryDirs = {[rootDir, '/coretech-common/include/'], [rootDir, '/coretech-vision/include/']};

if ismac()
    useDebugLibraries = true;
    openCvVersionString = '';

    ankiLibDirs = {[rootDir, '/coretech-common/build/xcode4/lib/'],...
                   [rootDir, '/coretech-vision/build/xcode4/lib/']};

    if useDebugLibraries
        openCvLibDir = [rootDir, '/coretech-external/build/xcode4/opencv-2.4.6.1/lib/Debug/'];
        ankiLibs = {'-lCoreTech_Common_64Debug', '-lCoreTech_Common_Embedded_64Debug', '-lCoreTech_Vision_64Debug', '-lCoreTech_Vision_Embedded_64Debug'}; 
    else
        openCvLibDir = [rootDir, '/coretech-external/build/xcode4/opencv-2.4.6.1/lib/Release/'];
        ankiLibs = {'-lCoreTech_Common_64Release', '-lCoreTech_Common_Embedded_64Release', '-lCoreTech_Vision_64Release', '-lCoreTech_Vision_Embedded_64Release'};
    end
elseif ispc()
    useDebugLibraries = false;
    openCvVersionString = '246';

    ankiLibDirs = {[rootDir, '/coretech-common/build/msvc2012/lib/'],...
                   [rootDir, '/coretech-vision/build/msvc2012/lib/']};

    if useDebugLibraries
        openCvLibDir = [rootDir, '/coretech-external/build/msvc2012/opencv-2.4.6.1/lib/Debug/'];
        ankiLibs = {'-lCoreTech_Common_32Debug', '-lCoreTech_Common_Embedded_32Debug', '-lCoreTech_Vision_32Debug', '-lCoreTech_Vision_Embedded_32Debug'}; 
    else
        openCvLibDir = [rootDir, '/coretech-external/build/msvc2012/opencv-2.4.6.1/lib/Release/'];
        ankiLibs = {'-lCoreTech_Common_32Release', '-lCoreTech_Common_Embedded_32Release', '-lCoreTech_Vision_32Release', '-lCoreTech_Vision_Embedded_32Release'};
    end
end % if ismac() ... elseif ispc()

openCvModules = {'calib3d', 'contrib', 'core', 'features2d', 'flann', 'highgui', 'imgproc', 'legacy', 'ml', 'nonfree', 'objdetect', 'ocl', 'photo', 'stitching', 'superres', 'video', 'videostab'};
openCvIncludeDirs = cell(length(openCvModules)+1,1);
openCvIncludeDirs{1} = [rootDir, '/coretech-external/opencv-2.4.6.1/include/'];
for i = 1:length(openCvModules)
    openCvIncludeDirs{i+1} = [rootDir, '/coretech-external/opencv-2.4.6.1/modules/', openCvModules{i}, '/include'];
end

mex(compileVerbose, '-g', '-compatibleArrayDims',  sourceFilename,...
    fullfile(mexWrapperDir, 'mexWrappers.cpp'), fullfile(mexWrapperDir, 'mexEmbeddedWrappers.cpp'), ...
    ['-I', ankiLibraryDirs{1}], ['-I', ankiLibraryDirs{2}], ...
    ['-I', openCvIncludeDirs{1}], ['-I', openCvIncludeDirs{2}], ['-I', openCvIncludeDirs{3}], ['-I', openCvIncludeDirs{4}], ['-I', openCvIncludeDirs{5}], ['-I', openCvIncludeDirs{6}], ['-I', openCvIncludeDirs{7}], ['-I', openCvIncludeDirs{8}], ['-I', openCvIncludeDirs{9}], ['-I', openCvIncludeDirs{10}], ['-I', openCvIncludeDirs{11}], ['-I', openCvIncludeDirs{12}], ['-I', openCvIncludeDirs{13}], ['-I', openCvIncludeDirs{14}], ['-I', openCvIncludeDirs{15}], ['-I', openCvIncludeDirs{16}], ['-I', openCvIncludeDirs{17}], ['-I', openCvIncludeDirs{18}], ...
    ['-L', ankiLibDirs{1}], ['-L', ankiLibDirs{2}], ankiLibs{1}, ankiLibs{2}, ankiLibs{3}, ankiLibs{4},...
    ['-I', mexWrapperDir], ...        
    ['-L', openCvLibDir], ['-lopencv_core', openCvVersionString], ['-lopencv_imgproc', openCvVersionString], ['-lopencv_highgui', openCvVersionString], ['-lopencv_calib3d', openCvVersionString], ['-lopencv_contrib', openCvVersionString]);

end % function compileAnkiMex(filename)
