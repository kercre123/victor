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
        midBarWidth = VisionMarkerTrained.SquareWidthFraction/2;
        
    case 'bright'
        midBarWidth = VisionMarkerTrained.SquareWidthFraction + VisionMarkerTrained.FiducialPaddingFraction/2;
        
    otherwise
        error('Unrecognized probe type "%s"', probeType);
end

endBarLength = VisionMarkerTrained.SquareWidthFraction + ...
    VisionMarkerTrained.FiducialPaddingFraction/2;

midBarLength = 0.5;

% NOTE: for bright probes, we're actually creating two copies of the
% same probe in the same location (in the corners of the gap region). This
% is simpler for continuing to compare pairs of bright/dark probes at run
% time, even though it isn't the most efficient.  
probes = struct( ...
    'x', [endBarLength midBarLength 1-endBarLength ...
          midBarWidth               1-midBarWidth ...
          midBarWidth               1-midBarWidth ...
          midBarWidth               1-midBarWidth ...
          endBarLength midBarLength 1-endBarLength], ...
    'y', [midBarWidth  midBarWidth  midBarWidth ...
          endBarLength              endBarLength ...
          midBarLength              midBarLength ...
          1-endBarLength            1-endBarLength ...
          1-midBarWidth 1-midBarWidth 1-midBarWidth]);
end