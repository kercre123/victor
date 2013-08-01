function tformTotal = ICP(P1, P2, tformType, varargin)
% Find transformation between point sets using Iterated Closest Point.
%
% tform = ICP(P1, P2, tformType, ...)
%
%  Returns a transformation of type 'tformType' (e.g. 'projective') that
%  maps 2D points P1 to P2.  The points are stored as [x(:) y(:)].
%
%   
% 
% ------------
% Andrew Stein
%

tformInit = [];
sampleFraction = 1;
maxIterations = 5;
tolerance = 1e-3;
DEBUG_DISPLAY = nargout==0;

parseVarargin(varargin{:});
  

if ~isempty(tformInit)
    if DEBUG_DISPLAY
        P1_orig = P1;
    end
    [x,y] = tformfwd(tformInit, P1(:,1), P1(:,2));
    P1 = [x(:) y(:)];
    tformTotal = tformInit;
else
    tformTotal = maketform(tformType, eye(3));
end

if DEBUG_DISPLAY
    namedFigure('ICP')
    hold off, plot(P2(:,1), P2(:,2), 'bo'); hold on
    h = plot(P1(:,1), P1(:,2), 'rx');
    axis equal
    title('Initial')
    pause
end

if sampleFraction < 1
    P1 = sample_array(P1, sampleFraction);
    P2 = sample_array(P2, sampleFraction);
end

i = 1;
change = inf;
while i < maxIterations && (isempty(tolerance) || change > tolerance)
    %D = pairwiseDist(P1,P2);
    %[~,closest] = min(D,[],1);
    closest = mexClosestIndex(P1, P2); %closestPoint(P1, P2);
    
    if DEBUG_DISPLAY
        h_closest = line([P1(:,1) P2(closest,1)]', [P1(:,2) P2(closest,2)]', ...
            'Color', 'g');
        pause
        delete(h_closest);
    end
    
    % Compute the transformation to transform P2 into P1, using the
    % current correspondence in "closest"
    try
        tform = cp2tform(P1, P2(closest,:), tformType);
    catch E
        warning(E.message);
        tformTotal = tformInit;
        return
    end
    tformTotal = maketform('composite', tform, tformTotal);
    
    [x,y] = tformfwd(tform, P1(:,1), P1(:,2));
    
    if ~isempty(tolerance)
        change = max(column(abs(P1 - [x(:) y(:)])));
    end
    
    P1 = [x(:) y(:)];
    
    if DEBUG_DISPLAY
        set(h, 'XData', x, 'YData', y);
        title(i)
        pause
    end
   
    i = i+1;
end

if DEBUG_DISPLAY
    [x,y] = tformfwd(tformTotal, P1_orig(:,1), P1_orig(:,2));
    plot(x,y, 'go');
    title('Final');
end


end


function index = closestPoint(P1, P2)

N1 = size(P1,1);

index = zeros(N1,1);
for i = 1:N1
    Dsq = (P1(i,1)-P2(:,1)).^2 + (P1(i,2)-P2(:,2)).^2;
    [~,index(i)] = min(Dsq);
end

end


