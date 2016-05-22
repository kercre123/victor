function [threshold, brightValue, darkValue] = ComputeThreshold(img, tform, method)

switch(method)
  
  case 'Otsu'
    
    probeValues = VisionMarkerTrained.GetProbeValues(img, tform);
    
    threshold = graythresh(probeValues);
    brightMask  = probeValues >= threshold;
    brightValue = mean(probeValues(brightMask));
    darkValue   = mean(probeValues(~brightMask));
    
  case 'FiducialProbes'
    
    brightValues = ProbeHelper(img, tform, VisionMarkerTrained.BrightProbes);
    darkValues   = ProbeHelper(img, tform, VisionMarkerTrained.DarkProbes);
    
    if any(brightValues < VisionMarkerTrained.MinContrastRatio*darkValues)
      % Make sure every corresponding pair of bright/dark probes has enough
      % contrast.
      threshold = -1;
      brightValue = [];
      darkValue = [];
    else
      % If so, compute the threshold from all of them
      brightValue = mean(brightValues);
      darkValue   = mean(darkValues);
      threshold = (brightValue + darkValue)/2;
    end
    
  otherwise
    
    error('Unrecognized threshold method "%s"', method);
    
end % switch(method)

end


function values = ProbeHelper(img, tform, probes)

numPoints = length(probes.x);

patternLength = length(VisionMarkerTrained.ProbePattern.x);

X = VisionMarkerTrained.ProbePattern.x(ones(1,numPoints),:) + probes.x(:)*ones(1,patternLength);
Y = VisionMarkerTrained.ProbePattern.y(ones(1,numPoints),:) + probes.y(:)*ones(1,patternLength);

[xi,yi] = tformfwd(tform, X, Y);
    
%values = mean(interp2(img, xi, yi, 'linear'),2);
values = mean(mexInterp2(img, xi, yi),2);

end