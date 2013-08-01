function triangleMat(varargin)

% Parameters:
matWidth  = 500; % mm
matHeight = 500; % mm
sideLength = 15; % mm

parseVarargin(varargin{:});

rectangle('Pos', [0 0 matWidth matHeight]);

h_tri = plot([0 sideLength sideLength/2 0], ...
    [0 0 sideLength*cos(pi/6) 0]);

slope = 2*sin(pi/3);
for x1 = 0:sideLength:(matWidth-sideLength)
    
    y_int = -slope*x1;
    x2 = min(matWidth, (matHeight-y_int) / slope);
    y2 = slope*x2 + y_int;
    line([x1 x2], [0 y2]); 
end

vertSpacing = sin(pi/3)*sideLength;
for y = 0:vertSpacing:(matHeight-vertSpacing)
   line([0 matWidth], [y y]); 
end

for x1 = sideLength:sideLength:matWidth
    
    y_int = slope*x1;
    x2 = max(0, (matHeight-y_int) / -slope);
    y2 = -slope*x2 + y_int;
    line([x1 x2], [0 y2]); 
end

axis equal

end % FUNCTION