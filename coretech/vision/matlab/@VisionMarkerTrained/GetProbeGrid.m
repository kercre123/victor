function [xgrid,ygrid] = GetProbeGrid()

probeRegion = VisionMarkerTrained.ProbeRegion;
[xgrid,ygrid] = meshgrid(linspace(probeRegion(1),probeRegion(2), ...
  VisionMarkerTrained.ProbeParameters.GridSize)); 

end