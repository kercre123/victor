function randomDotMat(varargin)

% Parameters
minRadius = 1;   % mm
maxRadius = 4;  % mm
resolution = 0.25; % mm/pixel
radiusDecreaseFactor = 0.8;
matDim = 100; % mm

padFactor = 1/radiusDecreaseFactor; % multiples of radius

matSize = matDim/resolution;
mat = false(matSize);

%[xmat,ymat] = meshgrid(1:matSize);

r_real = maxRadius;

while r_real >= minRadius
    
    r = r_real/resolution;
    rPad = 2*padFactor*r;
    
    available = ~imdilate(mat, strel('disk', ceil(rPad)));
    
    border = false(matSize);
    border([1:ceil(r) (end-ceil(r)):end], :) = true;
    border(:, [1:ceil(r) (end-ceil(r)):end]) = true;    
    
    matIndex = randperm(matSize^2);
    
    [xgrid,ygrid] = meshgrid(-r:r);
    stencil = xgrid.^2 + ygrid.^2 <= r^2;
    
    [xgridPad,ygridPad] = meshgrid(-rPad:rPad);
    stencilPad = xgridPad.^2 + ygridPad.^2 <= rPad^2;
    
    for i = 1:matSize^2
        if available(matIndex(i)) && ~border(matIndex(i))
            %cen = randi(matSize-2*r, [1 2]) + r;
            [ycen,xcen] = ind2sub([matSize matSize], matIndex(i));
            
            index = sub2ind([matSize matSize], ...
                round(ygrid(stencil)+ycen), ...
                round(xgrid(stencil)+xcen));
            
            assert(all(~mat(index)), 'Bug!');
            
            mat(index) = true;
            
            indexPad = sub2ind([matSize matSize], ...
                max(1, min(matSize, round(ygridPad(stencilPad)+ycen))), ...
                max(1, min(matSize, round(xgridPad(stencilPad)+xcen))));
            
            available(indexPad) = false;
            
            if false && rand < .1
                subplot 121,
                imshow(mat), title('mat')
                
                subplot 122,
                imshow(available), title('available')
                
                drawnow
            end
            
        end
    end

    r_real = r_real*radiusDecreaseFactor;
    
end % WHILE(r >= minRadius)

figure
imagesc(mat), axis image, colormap(gray)
hold on

conncomp = bwconncomp(mat, 4);
stats = regionprops(conncomp, {'Area', 'Centroid'});
centroids = vertcat(stats.Centroid);
radii = sqrt([stats.Area]/pi);

h_dot = zeros(1,length(stats));
for i = 1:length(stats)
    h_dot(i) = rectangle('Curvature', [1 1], ...
        'Pos', [centroids(i,:)-radii(i) 2*radii(i)*[1 1]]);
end

set(h_dot, 'EdgeColor', 'r');
h_cen = plot(centroids(:,1), centroids(:,2), 'b.');

tri = delaunay(centroids(:,1), centroids(:,2));


keyboard
 

end % FUNCTION randomDotMat