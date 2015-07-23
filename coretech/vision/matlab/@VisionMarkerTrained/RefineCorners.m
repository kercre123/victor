function [newCorners, newH] = RefineCorners(this, img, varargin)
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
DarkValue = 0;
BrightValue = 1;
MaxCornerChange = 5; % in pixels

parseVarargin(varargin{:});

Contrast = BrightValue - DarkValue;

diagonal = sqrt(max( sum((this.corners(1,:)-this.corners(4,:)).^2), ...
    sum((this.corners(2,:)-this.corners(3,:)).^2))) / sqrt(2);

N = ceil(NumSamples/8);
curveOffset = VisionMarkerTrained.CornerRadiusFraction;
xSquareOuter = [linspace(curveOffset,1-curveOffset,N) linspace(curveOffset,1-curveOffset,N) zeros(1,N)      ones(1,N)];
ySquareOuter = [zeros(1,N)      ones(1,N)       linspace(curveOffset,1-curveOffset,N) linspace(curveOffset, 1-curveOffset,N)];

xSquareInner = [linspace(max(this.SquareWidthFraction, curveOffset), 1-max(this.SquareWidthFraction,curveOffset), N) ...
    linspace(max(this.SquareWidthFraction, curveOffset), 1-max(this.SquareWidthFraction,curveOffset), N) ...
    this.SquareWidthFraction*ones(1,N) ...
    (1-this.SquareWidthFraction)*ones(1,N)];

ySquareInner = [this.SquareWidthFraction*ones(1,N) ...
    (1-this.SquareWidthFraction)*ones(1,N) ...
    linspace(this.SquareWidthFraction+curveOffset/2, 1-this.SquareWidthFraction-curveOffset/2, N) ...
    linspace(this.SquareWidthFraction+curveOffset/2, 1-this.SquareWidthFraction-curveOffset/2, N)];

TxOuter = [-1 zeros(1,N-2) 1, -1 zeros(1,N-2) 1, -ones(1,N), ones(1,N)];
TyOuter = [-ones(1,N), ones(1,N), -1 zeros(1,N-2) 1, -1 zeros(1,N-2) 1];

TxInner = -TxOuter;
TyInner = -TyOuter;

xsquare = [xSquareInner xSquareOuter]'; 
ysquare = [ySquareInner ySquareOuter]';
Tx = Contrast/2 * diagonal*[TxInner TxOuter]'; 
Ty = Contrast/2 * diagonal*[TyInner TyOuter]';
template = ((DarkValue+BrightValue)/2)*ones(size(xsquare));

A = [ xsquare.*Tx  ysquare.*Tx  Tx  ...
    xsquare.*Ty  ysquare.*Ty  Ty ...
    (-xsquare.^2.*Tx-xsquare.*ysquare.*Ty) ...
    (-xsquare.*ysquare.*Tx-ysquare.^2.*Ty)];

if DebugDisplay
    %hold off, imagesc(img), axis image, hold on, colormap(gray)
    delete(findobj(gca, 'Tag', 'CornerRefinement'))
    
    hold on
    plot(this.corners(:,1), this.corners(:,2), 'bx', 'MarkerSize', 10, 'Tag', 'CornerRefinement');
    
    h_out = plot(nan, nan, 'r', 'LineWidth', 2, 'Tag', 'CornerRefinement');
    h_in  = plot(nan, nan, 'r', 'LineWidth', 2, 'Tag', 'CornerRefinement');
    h_pr  = plot(nan, nan, 'r.', 'MarkerSize', 10, 'Tag', 'CornerRefinement');
end

newH = this.H;

iteration = 1;
while iteration < MaxIterations
    temp = newH*[xsquare ysquare ones(length(xsquare),1)]';
    xi = temp(1,:)./temp(3,:);
    yi = temp(2,:)./temp(3,:);
    
    if DebugDisplay
        
        temp = newH*[[0 0 1 1; 0 1 0 1]' ones(4,1)]';
        xc = temp(1,:)./temp(3,:);
        yc = temp(2,:)./temp(3,:);
        set(h_out, 'XData', xc([1 2 4 3 1]), 'YData', yc([1 2 4 3 1]));
        
        temp = newH*[[this.SquareWidthFraction*[1 1] (1-this.SquareWidthFraction)*[1 1]; 
            this.SquareWidthFraction 1-this.SquareWidthFraction this.SquareWidthFraction 1-this.SquareWidthFraction]' ones(4,1)]';
        xc = temp(1,:)./temp(3,:);
        yc = temp(2,:)./temp(3,:);
        set(h_in, 'XData', xc([1 2 4 3 1]), 'YData', yc([1 2 4 3 1]));
         
        set(h_pr, 'XData', xi, 'YData', yi)
        
        title(iteration)
        pause
    end
    
    %imgi = interp2(img, xi, yi, 'linear');
    %inBounds = ~isnan(imgi);
    
    if ~isempty(find(isnan(xi), 1)) || ~isempty(find(isnan(xi), 2))
        newCorners = nan * ones([length(xi),2]);
        newH = eye(3);
        return;
    else
        try        
            [imgi, outOfBounds] = mexInterp2(img, xi, yi);
        catch
            newCorners = nan * ones([length(xi),2]);
            newH = eye(3);
            return;
        end
    end
    
    inBounds = ~outOfBounds;
    
    It = imgi(:) - template;
    
    AtA = A(inBounds,:)'*A(inBounds,:);
    if any(isnan(AtA(:))) || rank(AtA) < 8
      break;
    end
    
    update = AtA \ (A(inBounds,:)'*It(inBounds));
    
    H_update = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
    newH = newH / H_update;
    iteration = iteration + 1;
end

temp = newH*[[0 0 1 1; 0 1 0 1]' ones(4,1)]';
newCorners = (temp(1:2,:)./temp([3 3],:))';

if any(sqrt(sum((newCorners - this.corners).^2,2)) > MaxCornerChange)
    % Corners moved too much, restore originals    
    newCorners = this.corners;
    newH = this.H;
end

if nargout == 1
    this.H = newH;
    this.corners = newCorners;
    
    newCorners = this;
end

end