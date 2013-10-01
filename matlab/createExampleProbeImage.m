
% function [image, quad, homography] = createExampleProbeImage(blockType, faceType)

% [image, quad, homography] = createExampleProbeImage(blockType, faceType);

function [image, quad, homography] = createExampleProbeImage(blockType, faceType)

image = zeros(130,120);
corners = [0,0; 
           0,200;
           200,0;
           200,200];
tform = maketform('projective', eye(3));
       
marker = BlockMarker2D(image, corners, tform);

if blockType < 0
    for bit = 1:size(marker.Xprobes, 1)
        image(round(marker.Xprobes(bit,:)*100), round(marker.Yprobes(bit,:)*100)) = bit * 10;
    end

    quad = [0,0; 0,100; 100,0; 100,100];
else
    image = generateMarkerImage(blockType, faceType);
    image = double(rgb2gray(image));
    
    [y,x] = find(image < 128);
    
    minX = min(x(:));
    minY = min(y(:));
    maxX = max(x(:));
    maxY = max(y(:));
    
    quad = [minX, minY;
            minX, maxY;
            maxX, minY;
            maxX, maxY];
end

tformInit = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');

homography = tformInit.tdata.T';

topLeft = [68,68];
blockWidth = 24;

ci = 0;
cy = 0;
extraX = 0;
extraY = 0;
for y = topLeft(1):blockWidth:180
    if cy == 4
        extraY = 1;
    end
    
    cx = 0;
    for x = topLeft(2):blockWidth:180
        meanVal = mean(mean(image(y:(y+23), x:(x+23))));
        if meanVal < 128
            image(y:(y+23), x:(x+23)) = image(extraY+(y:(y+23)), x:(x+23)) + ci*2;
        else
            image(y:(y+23), x:(x+23)) = image(extraY+(y:(y+23)), x:(x+23)) - ci*2;
        end
        
        cx = cx + 1;
        ci = ci + 1;
    end
    cy = cy + 1;
end

% Check that the example is correctly formed
if blockType < 0
    marker = BlockMarker2D(image, quad, tformInit);
    allMeans = marker.means';
    allMeans = allMeans(:);

    for bit = 1:length(allMeans);
        assert(abs(allMeans(bit) - bit*10) < .0001);
    end
end

close 1


% keyboard
