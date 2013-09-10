
useAndrewsPaths = false;

if useAndrewsPaths
    rootDir = fileparts(fileparts(fileparts(fileparts(which('makeMex.m')))));
    mexWrapperDir = fullfile(rootDir, 'coretech-common', 'matlab', 'mex');

    %%
    mex mexRegionProps.cpp
    %%
    mex mexClosestIndex.cpp
    %% 
    mex mexHist.cpp
    %%
    mex('mexGaussianBlur.cpp', ...
        fullfile(mexWrapperDir, 'mexWrappers.cpp'), ...
        '-I/usr/local/include/', ...
        ['-I', mexWrapperDir], ...
        '-L/usr/local/bin/', '-l opencv_core', '-l opencv_imgproc');
    %%
    mex('-g', 'mexBundleAdjust.cpp', ...
        fullfile(mexWrapperDir, 'mexWrappers.cpp'), ...
        '-I/usr/local/include/', ['-I', mexWrapperDir], ...
        '-L/usr/local/bin/', '-l opencv_core', '-l opencv_contrib', ...
        '-l opencv_calib3d');
else % if useAndrewsPaths

    compileAnkiMex('mexRegionProps.cpp');
    compileAnkiMex('mexClosestIndex.cpp');
    compileAnkiMex('mexHist.cpp');
    compileAnkiMex('mexGaussianBlur.cpp');
    compileAnkiMex('mex_cp2tform_projective.cpp');
    compileAnkiMex('mexBinomialFilter.cpp');
    compileAnkiMex('mexDownsampleByFactor.cpp');
    compileAnkiMex('mexComputeCharacteristicScale.cpp');
    compileAnkiMex('mexTraceBoundary.cpp');

end % if useAndrewsPaths ... else
   