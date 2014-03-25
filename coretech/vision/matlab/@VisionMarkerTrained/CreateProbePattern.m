function pattern = CreateProbePattern()

theta = linspace(0, 2*pi, VisionMarkerTrained.ProbeParameters.NumAngles+1);
theta = theta(1:end-1);

pattern = struct( ...
    'x', [0 VisionMarkerTrained.ProbeParameters.Radius*cos(theta)], ...
    'y', [0 VisionMarkerTrained.ProbeParameters.Radius*sin(theta)] );

end