function probes = CreateProbes(coord)
% Static method for computing the constant probes points' locations.

assert(mod(VisionMarker.ProbeParameters.Number,2)==1, ...
    'Expecting NumProbes to be odd.');

N = VisionMarker.ProbeParameters.Number;

probeWidth = VisionMarker.SquareWidth * VisionMarker.ProbeParameters.WidthFraction;

points = linspace(-VisionMarker.SquareWidth*(N-1)/2, VisionMarker.SquareWidth*(N-1)/2, N);

N_angles = VisionMarker.ProbeParameters.NumAngles;

probeRadius = probeWidth/2;
probes = cell(1,N_angles);
angles = linspace(0, 2*pi, N_angles +1);

points = points(ones(N,1),:) + 0.5;

cornerPoints = [1.5*VisionMarker.SquareWidth*[1; 1] ...
    (1 - 1.5*VisionMarker.SquareWidth)*[1;1]];

switch(coord)
    case 'X'      
        points = [cornerPoints(:); points(:)]; 
        for i = 1:N_angles 
            probes{i} = points + probeRadius*cos(angles(i));
        end
        
    case 'Y'
        points = [column(cornerPoints'); column(points')];
        for i = 1:N_angles 
            probes{i} = points + probeRadius*sin(angles(i));
        end
        
    otherwise
        error('Expecting "X" or "Y" for "coord" argument.');
end

probes = [points probes{:}];

end % function CreateProbes()