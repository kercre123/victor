function pattern = CreateProbePattern()

theta = linspace(0, 2*pi, VisionMarkerTrained.ProbeParameters.NumAngles+1);
theta = theta(1:end-1);

probeRegion = VisionMarkerTrained.ProbeRegion;

for i = 1:length(VisionMarkerTrained.ProbeParameters.GridSize)
    radius = (probeRegion(2)-probeRegion(1))/(3*VisionMarkerTrained.ProbeParameters.GridSize(i));
    
    pattern(i) = struct( ...
        'x', [0 radius*cos(theta)], ...
        'y', [0 radius*sin(theta)] ); %#ok<AGROW>
end

end