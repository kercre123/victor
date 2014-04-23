function RefineCorners(this, img, varargin)
% Iteratively refine the locations of the corners (and adjust transform).
%
% RefineCorners(this, img, varargin)
%
%  Samples points along the canonical inner and outer borders of the
%  fiducial square and using Lucas-Kanade style refinement to update the 
%  corners and transformation.
% 
% ------------
% Andrew Stein
%

MaxIterations = 10;
DebugDisplay = false;
NumSamples = 500; % divided evenly amongst the 8 square sides
Contrast = 1;

parseVarargin(varargin{:});

diagonal = sqrt(max( sum((this.corners(1,:)-this.corners(4,:)).^2), ...
    sum((this.corners(2,:)-this.corners(3,:)).^2))) / sqrt(2);

N = ceil(NumSamples/8);
xSquareOuter = [linspace(0,1,N) linspace(0,1,N) zeros(1,N)      ones(1,N)];
ySquareOuter = [zeros(1,N)      ones(1,N)       linspace(0,1,N) linspace(0,1,N)];

xSquareInner = [linspace(this.SquareWidthFraction, 1-this.SquareWidthFraction, N) ...
    linspace(this.SquareWidthFraction, 1-this.SquareWidthFraction, N) ...
    this.SquareWidthFraction*ones(1,N) ...
    (1-this.SquareWidthFraction)*ones(1,N)];

ySquareInner = [this.SquareWidthFraction*ones(1,N) ...
    (1-this.SquareWidthFraction)*ones(1,N) ...
    linspace(this.SquareWidthFraction, 1-this.SquareWidthFraction, N) ...
    linspace(this.SquareWidthFraction, 1-this.SquareWidthFraction, N)];

TxOuter = [-1 zeros(1,N-2) 1, -1 zeros(1,N-2) 1, -ones(1,N), ones(1,N)];
TyOuter = [-ones(1,N), ones(1,N), -1 zeros(1,N-2) 1, -1 zeros(1,N-2) 1];

TxInner = -TxOuter;
TyInner = -TyOuter;

xsquare = [xSquareInner xSquareOuter]'; 
ysquare = [ySquareInner ySquareOuter]';
Tx = Contrast/2 * diagonal*[TxInner TxOuter]'; 
Ty = Contrast/2 * diagonal*[TyInner TyOuter]';
template = (Contrast/2)*ones(size(xsquare));

A = [ xsquare.*Tx  ysquare.*Tx  Tx  ...
    xsquare.*Ty  ysquare.*Ty  Ty ...
    (-xsquare.^2.*Tx-xsquare.*ysquare.*Ty) ...
    (-xsquare.*ysquare.*Tx-ysquare.^2.*Ty)];

if DebugDisplay
    hold off, imagesc(img), axis image, hold on, colormap(gray)
    h_out = plot(nan, nan, 'r', 'LineWidth', 2);
    h_in  = plot(nan, nan, 'r', 'LineWidth', 2);
    %h_pr  = plot(nan, nan, 'r.', 'MarkerSize', 10);
end

iteration = 1;
while iteration < MaxIterations
    temp = this.H*[xsquare ysquare ones(length(xsquare),1)]';
    xi = temp(1,:)./temp(3,:);
    yi = temp(2,:)./temp(3,:);
    
    if DebugDisplay
        
        temp = this.H*[[0 0 1 1; 0 1 0 1]' ones(4,1)]';
        xc = temp(1,:)./temp(3,:);
        yc = temp(2,:)./temp(3,:);
        set(h_out, 'XData', xc([1 2 4 3 1]), 'YData', yc([1 2 4 3 1]));
        
        temp = this.H*[[this.SquareWidthFraction*[1 1] (1-this.SquareWidthFraction)*[1 1]; 
            this.SquareWidthFraction 1-this.SquareWidthFraction this.SquareWidthFraction 1-this.SquareWidthFraction]' ones(4,1)]';
        xc = temp(1,:)./temp(3,:);
        yc = temp(2,:)./temp(3,:);
        set(h_in, 'XData', xc([1 2 4 3 1]), 'YData', yc([1 2 4 3 1]));
               
        title(iteration)
        pause
    end
    
    %imgi = interp2(img, xi, yi, 'linear');
    imgi = mexInterp2(img, xi, yi);
    inBounds = ~isnan(imgi);
    It = imgi(:) - template;
    
    update = (A(inBounds,:)'*A(inBounds,:)) \ (A(inBounds,:)'*It(inBounds));
    
    H_update = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
    this.H = this.H / H_update;
    iteration = iteration + 1;
end

temp = this.H*[[0 0 1 1; 0 1 0 1]' ones(4,1)]';
this.corners = (temp(1:2,:)./temp([3 3],:))';

end