function probes = CreateProbes(probeType)

switch(probeType)
    
    case 'dark'
        midSq = VisionMarkerTrained.SquareWidthFraction/2;
        
        probes = struct( ...
            'x', [midSq midSq 1-midSq 1-midSq], ...
            'y', [midSq 1-midSq midSq 1-midSq]);
        
    case 'bright'
        midPad = VisionMarkerTrained.SquareWidthFraction + VisionMarkerTrained.FiducialPaddingFraction/2;

        probes = struct( ...
            'x', [midPad midPad 1-midPad 1-midPad], ...
            'y', [midPad 1-midPad midPad 1-midPad]);
        
    otherwise
        error('Unrecognized probe type "%s"', probeType);
end

end