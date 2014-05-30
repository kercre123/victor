function probes = CreateProbes(probeType)
% Creates probe locations for checking bright/dark values.
%
% probes = CreateProbes(probeType)
%
%  Creates probes in the 4 corners of the fiducial square (or the gap
%  region) and in the middle of its 4 sides, when probeType == 'dark' (or
%  'bright').
%

switch(probeType)
    
    case 'dark'
        midCorner = VisionMarkerTrained.SquareWidthFraction/2;
        
    case 'bright'
        midCorner = VisionMarkerTrained.SquareWidthFraction + VisionMarkerTrained.FiducialPaddingFraction/2;
        
    otherwise
        error('Unrecognized probe type "%s"', probeType);
end

midEdge = 0.5;

probes = struct( ...
    'x', [midCorner midCorner 1-midCorner 1-midCorner midCorner 1-midCorner midEdge midEdge], ...
    'y', [midCorner 1-midCorner midCorner 1-midCorner midEdge midEdge midCorner 1-midCorner]);

end