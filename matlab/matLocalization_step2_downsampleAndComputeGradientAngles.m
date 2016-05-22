function [orient, mag] = matLocalization_step2_downsampleAndComputeGradientAngles(img, orientationSample, derivSigma, embeddedConversions, DEBUG_DISPLAY)


% Note the downsampling here here, since we don't really need to use ALL
% pixels just to estimate the gradient orientation via a histogram
[Ix,Iy] = smoothgradient( ...
    img(1:orientationSample:end, 1:orientationSample:end), derivSigma);
mag = sqrt(Ix.^2 + Iy.^2);
orient = atan2(Iy, Ix);

% TODO: Can we just use absolute values of Ix and/or Iy instead of this?
orient(orient < 0) = pi + orient(orient < 0);
