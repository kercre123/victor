%%
mex mexRegionProps.cpp
%%
mex mexClosestIndex.cpp
%% 
mex mexHist.cpp
%%
mex mexGaussianBlur.cpp /Users/andrew/Code/mex/mexWrappers.cpp ...
    -I/usr/local/include/ -I/Users/andrew/Code/mex/ ...
    -L/usr/local/bin/ -l opencv_core -l opencv_imgproc
%%
mex -g mexBundleAdjust.cpp /Users/andrew/Code/mex/mexWrappers.cpp ...
    -I/usr/local/include/ -I/Users/andrew/Code/mex/ ...
    -L/usr/local/bin/ -l opencv_core -l opencv_contrib -l opencv_calib3d