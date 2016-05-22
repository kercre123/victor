function [x,y] = findSubpixelCorner(imgPatch, varargin)

sigma = 1;
kappa = 0.04;
winRad = 1;
numScales = 1;

parseVarargin(varargin{:});

for i_scale = numScales:-1:1
    % Compute the Harris corner measure at each pixel
    %
    % A = [sum(Ix^2)   sum(Ix*Iy);
    %      sum(Ix*Iy)  sum(Iy^2)];
    %
    % H = det(A) - kappa*trace(A)^2
    %
    inc = 2^(i_scale-1);
    imgScaled = imgPatch(1:inc:end, 1:inc:end);
    Ix = (image_right(imgScaled) - image_left(imgScaled))/2;
    Iy = (image_down(imgScaled) - image_up(imgScaled))/2;
    
    g = gaussian_kernel(sigma);
    sumIx2  = separable_filter(Ix.^2,  g);
    sumIy2  = separable_filter(Iy.^2,  g);
    sumIxIy = separable_filter(Ix.*Iy, g);
    
    detA = sumIx2.*sumIy2 - 2*sumIxIy;
    trA  = sumIx2 + sumIy2;
    
    H = detA - kappa*trA;
    
    if i_scale == numScales
        % Find the pixel with the highest score
        [~,index] = max(abs(H(:)));
        [y_init, x_init] = ind2sub(size(imgScaled), index);
    else
        x_init = round((x-1)*2 + 1);
        y_init = round((y-1)*2 + 1);
    end
    
    % Fit a quadratic to the 3x3 patch around that pixel
    offsets = -winRad:winRad;
    [X,Y] = meshgrid(offsets, offsets);
    X = X(:);
    Y = Y(:);
    
    A = [X.^2 Y.^2 X.*Y X Y ones(length(X),1)];
    b = column(H(y_init+offsets, x_init+offsets));
    
    p = A\b;
    
    % Compute the maximum of the quadratic fit
    LHS = [2*p(1) p(3); p(3) 2*p(2)];
    RHS = -p(4:5);
    
    peak = LHS \ RHS;
    
    x = x_init + peak(1);
    y = y_init + peak(2);
    
end % FOR each scale

if nargout == 0
    subplot 121
    hold off
    imagesc(imgPatch), axis image
    hold on
    plot(x, y, 'r.', 'MarkerSize', 10);
    title('Image Data')
    
    subplot 122
    hold off
    imagesc(abs(H)), axis image
    hold on
    plot(x, y, 'r.', 'MarkerSize', 10);
    plot(x_init, y_init, 'bx');
    title('Harris Corner-ness')
    legend('Final sub-pixel corner location', 'Initial integer-pixel corner')
end

end % FUNCTION findSubpixelCorner()
