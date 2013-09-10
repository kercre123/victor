function homographyTotal = ICP_projective(P1, P2, varargin)
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

homographyInit = [];
sampleFraction = 1;
maxIterations = 5;
tolerance = 1e-3;
DEBUG_DISPLAY = nargout==0;

parseVarargin(varargin{:});
if ~isempty(homographyInit)
    if DEBUG_DISPLAY
        P1_orig = P1;
    end

    projectedPoints = homographyInit * [P1, ones(size(P1,1),1)]';
    
    projectedPoints = projectedPoints ./ repmat(projectedPoints(3,:), [3,1]);
    
    P1 = projectedPoints(1:2,:)';
        
    homographyTotal = homographyInit;
else
    homographyTotal = eye(3);
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
    homography = mex_cp2tform_projective(P1, P2(closest,:));

    homographyTotal = homography * homographyTotal;

    projectedPoints = homography * [P1, ones(size(P1,1),1)]';
    projectedPoints = projectedPoints ./ repmat(projectedPoints(3,:), [3,1]);
               
    if ~isempty(tolerance)
        change = max(column(abs(P1 - [projectedPoints(1,:)' projectedPoints(2,:)'])));
    end
    
    P1 = projectedPoints(1:2,:)';
    
    if DEBUG_DISPLAY
        set(h, 'XData', x, 'YData', y);
        title(i)
        pause
    end
   
    i = i+1;
end

homographyTotal = homographyTotal / homographyTotal(3,3);

if DEBUG_DISPLAY
    [x,y] = tformfwd(tformTotal, P1_orig(:,1), P1_orig(:,2));
    plot(x,y, 'go');
    title('Final');
end

end % function homographyTotal = ICP_projective(P1, P2, varargin)


function index = closestPoint(P1, P2)

    N1 = size(P1,1);

    index = zeros(N1,1);
    for i = 1:N1
        Dsq = (P1(i,1)-P2(:,1)).^2 + (P1(i,2)-P2(:,2)).^2;
        [~,index(i)] = min(Dsq);
    end

end % function index = closestPoint(P1, P2)


