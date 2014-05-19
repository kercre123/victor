function pattern = CreateProbePattern()

theta = linspace(0, 2*pi, VisionMarkerTrained.ProbeParameters.NumAngles+1);
theta = theta(1:end-1);

probeRegion = VisionMarkerTrained.ProbeRegion;

radius = (probeRegion(2)-probeRegion(1))/(2*(VisionMarkerTrained.ProbeParameters.GridSize+1));

pattern = struct( ...
    'x', [0 radius*cos(theta)], ...
    'y', [0 radius*sin(theta)] );

end