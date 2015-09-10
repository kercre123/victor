function [xgrid,ygrid] = GetProbeGrid(workingResolution)

if nargin == 0
  workingResolution = VisionMarkerTrained.ProbeParameters.GridSize;
end

probeRegion = VisionMarkerTrained.ProbeRegion;
[xgrid,ygrid] = meshgrid(linspace(probeRegion(1),probeRegion(2),workingResolution+2));
xgrid = column(xgrid(2:end-1,2:end-1));
ygrid = column(ygrid(2:end-1,2:end-1));


end