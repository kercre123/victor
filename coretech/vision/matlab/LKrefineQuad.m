function [corners, H] = LKrefineQuad(img, corners, H, varargin)

MaxIterations = 25;
DebugDisplay = false;

parseVarargin(varargin{:});

diagonal = round(2*sqrt(max( sum((corners(1,:)-corners(4,:)).^2), ...
    sum((corners(2,:)-corners(3,:)).^2))));

border = VisionMarker.SquareWidth*VisionMarker.FiducialPaddingFraction;

[xgrid,ygrid] = meshgrid(linspace(-border,1+border,diagonal));
template = ones(size(xgrid));
template(xgrid>0 & xgrid<1 & ygrid>0& ygrid<1) = 0;
template(xgrid>VisionMarker.SquareWidth & xgrid<(1-VisionMarker.SquareWidth) & ...
    ygrid>VisionMarker.SquareWidth & ygrid<(1-VisionMarker.SquareWidth)) = 1;
% template = separable_filter(template, gaussian_kernel(5));

middle = false(size(xgrid));
midEdge = 0.5*VisionMarker.ProbeParameters.Number*VisionMarker.SquareWidth;
middle(xgrid>0.5-midEdge & xgrid<0.5+midEdge & ygrid>0.5-midEdge & ygrid<0.5+midEdge) = true;

oneCorner = xgrid>VisionMarker.SquareWidth & ...
    xgrid<(2+VisionMarker.FiducialPaddingFraction)*VisionMarker.SquareWidth & ...
    ygrid > VisionMarker.SquareWidth & ...
    ygrid<(2+VisionMarker.FiducialPaddingFraction)*VisionMarker.SquareWidth;
cornerMarkers = oneCorner | rot90(oneCorner) | rot90(rot90(oneCorner)) | rot90(rot90(rot90(oneCorner)));

mask = ~middle & ~cornerMarkers;
mask([1 end],:) = false;
mask(:,[1 end]) = false;

Tx = (image_right(template)-image_left(template))/(2/diagonal);
Ty = (image_down(template)-image_up(template))/(2/diagonal);

Tx = Tx(mask);
Ty = Ty(mask);
xsquare = xgrid(mask);
ysquare = ygrid(mask);
template = template(mask);

A = [ xsquare.*Tx  ysquare.*Tx  Tx  ...
    xsquare.*Ty  ysquare.*Ty  Ty ...
    (-xsquare.^2.*Tx-xsquare.*ysquare.*Ty) ...
    (-xsquare.*ysquare.*Tx-ysquare.^2.*Ty)];

% points = linspace(0,1,diagonal)';
% 
% offset = VisionMarker.SquareWidth/2;
% xsquareOutside = [zeros(diagonal,1)-offset; points; ones(diagonal,1)+offset; flipud(points)];
% ysquareOutside = [points; ones(diagonal,1)+offset; flipud(points); zeros(diagonal,1)-offset];
% xsquareInside  = [zeros(diagonal,1)+offset; points; ones(diagonal,1)-offset; flipud(points)];
% ysquareInside  = [points; ones(diagonal,1)-offset; flipud(points); zeros(diagonal,1)+offset];
% 
% xsquare = [xsquareOutside; xsquareInside];
% ysquare = [ysquareOutside; ysquareInside];
% 
% 
xcen = 0;
ycen = 0;
% xcen = mean(corners(:,1));
% ycen = mean(corners(:,2));
% corners(:,1) = corners(:,1) - xcen;
% corners(:,2) = corners(:,2) - ycen;
% % C = [1 0 -xcen; 0 1 -ycen; 0 0 1];
% % H = C*H;
% 
% tform = cp2tform(corners, [0 0 1 1; 0 1 0 1]', 'projective');
% H = tform.tdata.Tinv';
% 
% temp = H*[xsquare ysquare ones(length(xsquare),1)]';
% xi = temp(1,:)./temp(3,:) + xcen;
% yi = temp(2,:)./temp(3,:) + ycen;
% imgi = interp2(img, xi, yi, 'linear');
% bright = max(imgi(:));
% dark   = min(imgi(:));
% template = [bright*ones(size(xsquareOutside)); ...
%     dark*ones(size(xsquareInside))];
% 
% Tx = (bright-dark)*[-ones(diagonal,1);  zeros(diagonal,1); ones(diagonal,1);   zeros(diagonal,1)]/(2*offset);
% Ty = (bright-dark)*[ zeros(diagonal,1); ones(diagonal,1);  zeros(diagonal,1); -ones(diagonal,1)]/(2*offset);
% Aoutside = [ xsquareOutside.*Tx  ysquareOutside.*Tx  Tx  xsquareOutside.*Ty  ysquareOutside.*Ty  Ty -xsquareOutside.^2.*Tx-xsquareOutside.*ysquareOutside.*Ty -xsquareOutside.*ysquareOutside.*Tx-ysquareOutside.^2.*Ty];
% Ainside  = [ xsquareInside.*Tx   ysquareInside.*Tx   Tx  xsquareInside.*Ty   ysquareInside.*Ty   Ty -xsquareInside.^2.*Tx-xsquareInside.*ysquareInside.*Ty    -xsquareInside.*ysquareInside.*Tx-ysquareInside.^2.*Ty];
% A = [Aoutside; Ainside];

if DebugDisplay
    hold off, imagesc(img), axis image, hold on
    h_out = plot(nan, nan, 'r', 'LineWidth', 2);
    h_in  = plot(nan, nan, 'r', 'LineWidth', 2);
    h_pr  = plot(nan, nan, 'r.', 'MarkerSize', 10);
end

iteration = 1;
while iteration < MaxIterations
    temp = H*[xsquare ysquare ones(length(xsquare),1)]';
    xi = temp(1,:)./temp(3,:) + xcen;
    yi = temp(2,:)./temp(3,:) + ycen;
    
    if DebugDisplay
        
        temp = H*[[0 0 1 1; 0 1 0 1]' ones(4,1)]';
        xc = temp(1,:)./temp(3,:);
        yc = temp(2,:)./temp(3,:);
        set(h_out, 'XData', xc([1 2 4 3 1]), 'YData', yc([1 2 4 3 1]));
        
        temp = H*[[VisionMarker.SquareWidth*[1 1] (1-VisionMarker.SquareWidth)*[1 1]; 
            VisionMarker.SquareWidth 1-VisionMarker.SquareWidth VisionMarker.SquareWidth 1-VisionMarker.SquareWidth]' ones(4,1)]';
        xc = temp(1,:)./temp(3,:);
        yc = temp(2,:)./temp(3,:);
        set(h_in, 'XData', xc([1 2 4 3 1]), 'YData', yc([1 2 4 3 1]));
        
        temp = H*[VisionMarker.XProbes(:) VisionMarker.YProbes(:) ones(numel(VisionMarker.XProbes),1)]';
        xp = temp(1,:) ./ temp(3,:);
        yp = temp(2,:) ./ temp(3,:);
        set(h_pr, 'XData', xp, 'YData', yp);
        
        title(iteration)
        pause
    end
    
    imgi = interp2(img, xi, yi, 'linear');
    inBounds = ~isnan(imgi);
    It = imgi(:) - template;
    
    update = (A(inBounds,:)'*A(inBounds,:)) \ (A(inBounds,:)'*It(inBounds));
    
    H_update = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
    H = H / H_update;
    iteration = iteration + 1;
end

temp = H*[[0 0 1 1; 0 1 0 1]' ones(4,1)]';
corners = (temp(1:2,:)./temp([3 3],:))';

end