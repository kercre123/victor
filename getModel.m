function [X, Y, Z, color] = getModel(blockType, faceType, topSide)

% Canonical block coordinates
% Note that Z coords are negative because they go backward from marker on
% the face.
X = [0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
Y = [0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
Z = -[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';

originOffset = zeros(1,3);
scale = 10*ones(1,3);
color = 'k';

switch(blockType)
    
    case 3
        % puzzleMaster box
        switch(faceType)
            case 9
                
                originOffset = [25 55 0];
                scale = [86 122 31];
                
                color = 'r';
%                 
%                 % Blue clipboard
%                 originOffset = [16.5 49 0];
%                 scale = [225 317 2.5];
%                     
%                 color = 'b';
                
            otherwise
                warning('Unrecognized face for blockType %d', blockType);
        end
        
    case 17
        % Angel Cards box
        switch(faceType)
            case 9
                
                originOffset = [32 46 0];
                scale = [101 101 32];
                
                color = [.4 .3 .7];
                
            otherwise
                warning('Unrecognized face for blockType %d', blockType);
        end
        
    case 78
        % Cat cube
        switch(faceType)
            case 1
                
                originOffset = [15 47 0];
                scale = [70 70 70];
                    
                color = [.6 .5 .5];
                
            case 12
                warning('Have not set up second face of same block yet.');
                
            otherwise
                warning('Unrecognized face for blockType %d', blockType);
        end
        
    case 90
        % Staples
        switch(faceType)
            case 1
                
                originOffset = [35 45 0];
                scale = [103 57 23];
                
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

switch(topSide)
    case 'right'
        Rtop = eye(3);
    case 'down'
        Rtop = rodrigues(-pi/2*[0 0 1]);
    case 'left'
        Rtop = rodrigues(pi*[0 0 1]);
    case 'up'
        Rtop = rodrigues(pi/2*[0 0 1]);
end
P = [X(:) Y(:) Z(:)] * Rtop';
X = reshape(P(:,1), 4, []);
Y = reshape(P(:,2), 4, []);
Z = reshape(P(:,3), 4, []);

end