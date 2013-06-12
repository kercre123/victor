function [X, Y, Z, color] = getModel(blockType, faceType, topSide)

% Canonical block coordinates (in camera frame! y points up, z points back
% away from camera)
% Note that Z coords are negative because they go backward from marker on
% the face.
X = [0 1 1 0; 0 1 1 0; 0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0]';
Y = [0 0 1 1; 0 0 1 1; 0 0 1 1; 0 0 1 1; 1 1 1 1; 0 0 0 0]';
Z = -[0 0 0 0; 1 1 1 1; 0 1 1 0; 0 1 1 0; 0 0 1 1; 0 0 1 1]';

originOffset = zeros(1,3);
scale = 10*ones(1,3);
color = 'k';

switch(blockType)
    case 10 % Green 2x1x4 Block
        color = 'g';
        [scale, originOffset] = megaBlockHelper(faceType, 63,75,31, ...
            [19 44.5], [17 44.5]);
        
    case 15 % Blue 2x1x4 Block
        color = 'b';
        [scale, originOffset] = megaBlockHelper(faceType, 63,75,31, ...
            [19 44.5], [18 43.5]);
        
    case 20 % Red 2x1x4 Block
        color = 'r';
        [scale, originOffset] = megaBlockHelper(faceType, 63,75,31, ...
            [19 44], [19 43.5]);
        
    case 25 % Blue 2x1x5 Block
        color = 'b';
        [scale, originOffset] = megaBlockHelper(faceType, 63,94.5,31, ...
            [18 44], [18 44]);
        
    case 30 % Red 2x1x5 Block
        color = 'r';
        [scale, originOffset] = megaBlockHelper(faceType, 63,94.5,31, ...
            [17 44], [18 44]);
        
    case 35 % Red 2x2x3 Block
        color = 'r';
        [scale, originOffset] = megaBlockHelper(faceType, 63,56,63, ...
            [19 38], [18 38], [18 38], [18 38.5]);
        
    case 40 % Green 2x2x3 Block
        color = 'g';
        [scale, originOffset] = megaBlockHelper(faceType, 63,56,63, ...
            [18.75 38], [18 38.5], [18 38], [18.5 38]);
        
    case 45 % Blue 3x2x3 Block
        color = 'b';
        [scale, originOffset] = megaBlockHelper(faceType, 94,56,63, ...
            [33.5 38], [18.5 38], [33 38], [18 38]);
        
    case 50 % 4x2x3 Yellow Block
        color = 'y';
        [scale, originOffset] = megaBlockHelper(faceType, 126,56,63, ...
            [48 38], [17 38], [51 38], [19 38.5]);
        
    
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

% Xr = R(1,1)*X + R(1,2)*Y + R(1,3)*Z;
% Yr = R(2,1)*X + R(2,2)*Y + R(2,3)*Z;
% Zr = R(3,1)*X + R(3,2)*Y + R(3,3)*Z;

%scale = scale * R';
% originOffset = originOffset * R;
X = scale(1)*X - originOffset(1);
Y = scale(2)*Y - originOffset(2);
Z = scale(3)*Z - originOffset(3);

% Xr = R(1,1)*X + R(1,2)*Y + R(1,3)*Z 
% Yr = R(2,1)*X + R(2,2)*Y + R(2,3)*Z;
% Zr = R(3,1)*X + R(3,2)*Y + R(3,3)*Z;


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


function [scale, originOffset] = megaBlockHelper(faceType, w,h,d, ...
    frontOrigin, rightOrigin, backOrigin, leftOrigin)

twoSided = false;
if nargin == 6
    backOrigin = rightOrigin;
    twoSided = true;
end

switch(faceType)
    case 1 % Front
        scale = [w h d];
        originOffset = frontOrigin;
    case 5 % Right (or Back for tw-sided blocks)
        if twoSided
            scale = [w h d];
            originOffset = backOrigin;
        else
            scale = [d h w];
            originOffset = rightOrigin;
        end
    case 8 % Back
        scale = [w h d];
        originOffset = backOrigin;
    case 10 % Left
        scale = [d h w];
        originOffset = leftOrigin;
    otherwise
        error('No face %d for this block.', faceType);
end % SWITCH(faceType)

originOffset = [originOffset 0];
        
end
