
% function [image, quad] = createExampleProbeImage()

% [image, quad] = createExampleProbeImage();

function [image, quad] = createExampleProbeImage()

image = zeros(130,120);
corners = [0,0; 
           0,200;
           200,0;
           200,200];
tform = maketform('projective', eye(3));
       
marker = BlockMarker2D(image, corners, tform);

for bit = 1:size(marker.Xprobes, 1)
    image(round(marker.Xprobes(bit,:)*100), round(marker.Yprobes(bit,:)*100)) = bit * 10;
end

quad = [0,0; 0,100; 100,0; 100,100];

tformInit = cp2tform(quad, [0 0; 0 1; 1 0; 1 1], 'projective');

% h = tformInit.tdata.T';
%  * [quad';1,1,1,1]

% Check that the example is correctly formed
marker = BlockMarker2D(image, quad, tformInit);
allMeans = marker.means';
allMeans = allMeans(:);
for bit = 1:length(allMeans);
    assert(abs(allMeans(bit) - bit*10) < .0001);
end

keyboard
