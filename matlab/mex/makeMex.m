
useAndrewsPaths = false;

if useAndrewsPaths
    rootDir = fileparts(fileparts(fileparts(fileparts(which('makeMex.m')))));
    mexWrapperDir = fullfile(rootDir, 'coretech-common', 'matlab', 'mex');

    %%
    mex('-g', 'mexBundleAdjust.cpp', ...
        fullfile(mexWrapperDir, 'mexWrappers.cpp'), ...
        '-I/usr/local/include/', ['-I', mexWrapperDir], ...
        '-L/usr/local/bin/', '-l opencv_core', '-l opencv_contrib', ...
        '-l opencv_calib3d');
else % if useAndrewsPaths
    
    compileAnkiMex('mexBundleAdjust.cpp');
    
end % if useAndrewsPaths ... else
