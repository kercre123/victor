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
        shift = 0.25; % shifts the position towards the outer edge, to help with eroded fiducial due to over-exposure
        
    case 'bright'
        midBarWidth = VisionMarkerTrained.SquareWidthFraction + VisionMarkerTrained.FiducialPaddingFraction/2;
        shift = 1.0;
        
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
          shift*midBarWidth               1-shift*midBarWidth ...
          shift*midBarWidth               1-shift*midBarWidth ...
          shift*midBarWidth               1-shift*midBarWidth ...
          endBarLength midBarLength 1-endBarLength], ...
    'y', [shift*midBarWidth  shift*midBarWidth  shift*midBarWidth ...
          endBarLength              endBarLength ...
          midBarLength              midBarLength ...
          1-endBarLength            1-endBarLength ...
          1-shift*midBarWidth 1-shift*midBarWidth 1-shift*midBarWidth]);
end