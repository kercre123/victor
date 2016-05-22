function testCloseUpTrack(img, varargin)

maxIterations = 50;
changeTolerance = 1e-3;
dampening = .9;
xInit = [];
yInit = [];
doAffine = true;
target = [];

parseVarargin(varargin{:});

if isempty(xInit) || isempty(yInit)
    clf
    imshow(img)
    [xInit, yInit] = ginput(4);
end

initScale = sqrt( (xInit(1)-xInit(3))^2 + (yInit(1)-yInit(3))^2 )/ sqrt(18);

if isempty(target)
    target = ones(3,3);
    target(1:2:end) = 1/256;
    
    target = imresize(target, initScale, 'nearest');
    target = separable_filter(target, gaussian_kernel(.1));
end

targetSize = size(target);
% meanval = mean(target(:));
% target = target - meanval;
% img = img - meanval;
% target = imfilter(target, fspecial('log'));
% img = imfilter(img, fspecial('log'));

Tx = (image_right(target) - image_left(target))/2;
Ty = (image_down(target) - image_up(target))/2;

Tx = Tx(:);
Ty = Ty(:);

if doAffine
    %[xgrid,ygrid] = meshgrid(linspace(xInit(1), xInit(2), targetSize), ...
    %    linspace(yInit(1), yInit(4), targetSize));
    [xgrid,ygrid] = meshgrid(1:targetSize(2), 1:targetSize(1));
    
    A = [xgrid(:).*Tx ygrid(:).*Tx Tx xgrid(:).*Ty ygrid(:).*Ty Ty];
    
        initTform = cp2tform( ...
            [xgrid(1,1) ygrid(1,1); xgrid(1,end) ygrid(1,end);
            xgrid(end, end) ygrid(end, end); xgrid(end,1) ygrid(end,1)], ...
            [xInit(:) yInit(:)], 'affine');
    
        affineWarp = initTform.tdata.T';
                
        temp = affineWarp * [xgrid(:) ygrid(:) ones(prod(targetSize),1)]';
        xi = temp(1,:)';
        yi = temp(2,:)';
                
%     affineWarp = eye(3);
% xi = xgrid(:);
% yi = ygrid(:);
    
else
    [xgrid,ygrid] = meshgrid(1:targetSize(2), 1:targetSize(1));
    
    A = [Tx Ty];
    
    tx = mean(xInit(:)) - mean(xgrid(:));
    ty = mean(yInit(:)) - mean(ygrid(:));
    xi = xgrid(:) + tx;
    yi = ygrid(:) + ty;
end

subplot 121, hold off
imshow(img), hold on, plot(xInit, yInit, 'rx');
h = plot(xi, yi, 'b.', 'MarkerSize', 10);

subplot 122, hold off
h_diff = imshow(target);

pause

iterations = 0;
update = 1;

weights = exp(-.5*((xgrid-targetSize(2)/2).^2 + ...
    (ygrid-targetSize(1)/2).^2)/(mean(targetSize)/2)^2);
weights = weights(:);


while iterations < maxIterations && any(abs(update) > changeTolerance)
    
    iterations = iterations + 1;
    
    img_i = interp2(img, xi, yi, 'bilinear');
    inbounds = ~isnan(img_i);
    img_i(~inbounds) = 0;
    
    if ~any(inbounds)
        error('Entire target drifted out of bounds.');
    end
    
%     if iterations==1
%         minval = min(img_i(:));
%         maxval = max(img_i(:));
%         target = target*(maxval-minval) + minval;
%     end
    It = target(inbounds) - img_i(inbounds);
    
    temp = zeros(targetSize);
    temp(inbounds) = It;
    set(h_diff, 'CData', abs(temp));
    title(iterations)

    %AtA = A(inbounds,:)'*(weights(inbounds,ones(1,size(A,2))).*A(inbounds,:));
    %update = AtA \ (A(inbounds,:)'*(weights(inbounds).*It));
    update = least_squares_norm(A, It, weights);
    
    if doAffine
        warpUpdate = eye(3) + dampening*[update(1:3)'; update(4:6)'; zeros(1,3)];
        affineWarp = warpUpdate*affineWarp;
                
        temp = affineWarp * [xgrid(:) ygrid(:) ones(prod(targetSize),1)]';
        xi = temp(1,:)';
        yi = temp(2,:)';
    else
        tx = tx + update(1);
        ty = ty + update(2);
        xi = xgrid(:) + tx;
        yi = ygrid(:) + ty;
    end    
    
    set(h, 'XData', xi, 'YData', yi);

    pause(.1)
end



