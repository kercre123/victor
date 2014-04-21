function [threshold, brightValue, darkValue] = ComputeThreshold(img, tform)

brightValue = mean(ProbeHelper(img, tform, VisionMarkerTrained.BrightProbes));
darkValue   = mean(ProbeHelper(img, tform, VisionMarkerTrained.DarkProbes));

if brightValue < VisionMarkerTrained.MinContrastRatio*darkValue
    threshold = -1;
else
    threshold = (brightValue + darkValue)/2;
end

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