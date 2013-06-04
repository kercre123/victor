function [X, Y, Z, color] = getModel(blockType, faceType)

% Canonical block coordinates
% Note that Z coords are negative because they go backward from marker on
% the face.
X = [0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
Y = [0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
Z = -[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';

originOffset = zeros(1,3);
scale = 10;
color = 'k';

switch(blockType)
    case 78
        switch(faceType)
            case 1
                % Cat cube
                originOffset = [15 13 0];
                scale = [70 70 70];
                    
                color = [.6 .5 .5];
                
            case 12
                % puzzleMaster box
                originOffset = [28 16 0];
                scale = [86 122 31];
                
                color = 'r';
                
            otherwise
                warning('Unrecognized face for blockType %d', blockType);
        end
        
    case 90
        switch(faceType)
            case 1
                % Staples
                originOffset = [35 10 0];
                scale = [103 52 23];
                
                color = 'y';
                
            otherwise
                warning('Unrecognized face for blockType %d', blockType);
        end

    otherwise
        warning('Unrecognized blockType %d', blockType);
end

X = scale(1)*X - originOffset(1);
Y = scale(2)*Y - originOffset(2);
Z = scale(3)*Z - originOffset(3);

end