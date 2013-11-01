
function [img2_warp, total_warp, valid_region] = EstimateDominantMotion(img1,img2, ...
    max_sigma, do_affine, init_warp, LAB_input, motion_prior)
%
% [img2_warp, total_warp, <valid_region>] = EstimateDominantMotion(img1,img2,...
%    max_sigma, do_affine, init_warp, LAB_input)
% 
% Performs multi-scale dominant motion estimation between two images.
%
%  max_sigma - the maximum scale at which to perform the multi-scale
%              dominant motion estimation
% 
%  do_affine - Estimate full affine motion, return 3x3 affine warp in
%              total_warp.  If set to false, estimate translation component
%              only and return the 2x1 translation vector [u; v] in
%              total_warp.  
%
%  init_warp - The 'total_warp' returned by a previous call.  If provided,
%              the warp for the current call is initialized to this.
%              Otherwise, the initial warp is assumed to be zero.
%
%  LAB_input - If the input images are color, assume they are in LAB color
%              space.  Thus the brightness channel used for the motion
%              estimation is the first channel.  Otherwise, RGB input is
%              assumed, and the brightness data will be obtained by calling
%              RGB2Gray on the input images.  Defaults to false.
%
%  motion_prior - Places a small-motion prior on the warp parameters
%
% 
% Currently returns:
%  img2_warp    - img2 warped to best match img1
%  total_warp   - translation vector or affine transform from img1 to img2 coordinates
%  valid_region - binary map indicating which pixels are valid (from within
%                 the given image data)
%

%  moving_img - abs( img2 - warped version of img1) <optional>
%

%% Input Processing
DEBUG_DISPLAY = true;
convergence_threshold = 0.01;
max_iterations = 20;
dampening = 0.99;

if nargout==0
    DEBUG_DISPLAY = true;
end

if nargin<7
    motion_prior = [];
end

if nargin<6 || isempty(LAB_input)
    LAB_input = false;
end
if nargin<5
    init_warp = [];
end


[nrows,ncols,nchannels] = size(img1);

% originally_uint8 = false;
% if isa(img2, 'uint8')
%     originally_uint8 = true;
% end

original_class = class(img2);

if nchannels==3
    img1_color = img1;
    img2_color = img2;
    if LAB_input
        img1 = img1(:,:,1);
        img2 = img2(:,:,1);
    else
        img1 = rgb2gray(img1);
        img2 = rgb2gray(img2);
    end
end


if isempty(init_warp)
    if do_affine
        % initialize the cumulative affine warp to be the identity (no warp)
        affine_warp = eye(3);
    else
        % initialize to zero translation
        translation = zeros(2,1);
    end
else
    if do_affine
        if all(size(init_warp)~=[3 3])
            error('Initial warp provided was not from an affine estimation, \nbut affine estimation requested for this call!');
        end
        affine_warp = init_warp;
    else
        if all(size(init_warp)~=[2 1])
            error('Initial warp not from a translation estimation, \nbut translation estimation requested for this call!');
        end
        translation = init_warp;
    end
end

%% Motion Estimation
sigma_list = max_sigma;
ctr = 1;
while sigma_list(ctr) > 1
    ctr = ctr + 1;
    sigma_list(ctr) = sigma_list(ctr-1)/2;
end

% Normalize the images to be between 0 and 1 so that the spatial and 
% temporal derivatives are the same order of magnitude
img1 = img_norm(double(img1));
[img2, norm_min, norm_max] = img_norm(double(img2));

for i_sigma = 1:length(sigma_list)
    sigma = sigma_list(i_sigma);
    
    % Recompute coordinate grid at this scale
    [xgrid,ygrid] = meshgrid(linspace(1,ncols, ncols/sigma), ...
                             linspace(1,nrows, nrows/sigma));
    
    % Get the reference image at this scale
    g = myGaussian2D(sigma/2);
    img1_smoothed = double(imfilter(single(img1), g));
    img2_smoothed = double(imfilter(single(img2), g));
    img1_scaled = mexInterp2( img1_smoothed , xgrid, ygrid );
                                
    % Compute derivatives at this scale
    %     [Ix,Iy] = smoothgradient(img1_scaled, 1);
    [Ix,Iy] = gradient(img1_scaled);  Ix = -Ix; Iy = -Iy;  % evidently smoothgradient and gradient produce negative gradients of each other!
    Ix = Ix * sigma;
    Iy = Iy * sigma;
    
    % Construct A matrix
    Ix = Ix(:); 
    Iy = Iy(:);
    x = xgrid(:);
    y = ygrid(:);
    
    if do_affine 
        x_cen = mean(x);
        y_cen = mean(y);
        x = x - x_cen;
        y = y - y_cen;
        A = [x.*Ix y.*Ix Ix x.*Iy y.*Iy Iy];
    else
        A = [Ix Iy];
    end
       
    if DEBUG_DISPLAY 
        figure(gcf)
        subplot(1,3,1), imagesc(img1_scaled), axis image
        subplot(1,3,2), h2 = imagesc(img1_scaled); axis image
        subplot(1,3,3), h_diff = imagesc(img1_scaled); axis image
        set(h2, 'EraseMode', 'none');
        set(h_diff, 'EraseMode', 'none');
        
        fprintf('Sigma = %.1f, Iteration 000', sigma);
    end
    
    update = convergence_threshold+1;
    iterations = 0;
    
    while any(abs(update)>convergence_threshold) && iterations<max_iterations
        iterations = iterations + 1;        
        
        % Get img2_warp
        if do_affine
            xgrid_warp = affine_warp(1,1)*x + affine_warp(1,2)*y + affine_warp(1,3) + x_cen;
            ygrid_warp = affine_warp(2,1)*x + affine_warp(2,2)*y + affine_warp(2,3) + y_cen;
        else
            xgrid_warp = x + translation(1);
            ygrid_warp = y + translation(2);
        end
        [img2_warp, out_of_bounds] = mexInterp2( img2_smoothed, xgrid_warp, ygrid_warp );
        weights = column(double(~out_of_bounds));

        It = img2_warp - img1_scaled(:);

        if DEBUG_DISPLAY
            fprintf('\b\b\b%3d', iterations);
            set(h2, 'CData', reshape(img2_warp, size(img1_scaled)));
            set(h_diff, 'CData', reshape(abs(It), size(img1_scaled)));
            drawnow
        end
        
        update = least_squares_norm(A, It(:), weights, motion_prior);

        % Incorporate the update into the current warp
        if do_affine
            warp_update = eye(3) + dampening*[update(1:3)'; update(4:6)'; zeros(1,3)];
            affine_warp = warp_update*affine_warp;
        else
            translation = translation + dampening*update;
        end

    end % until motion converged
    if DEBUG_DISPLAY
        fprintf('\n');
    end
    
end % loop over scales


%% Output

% Get final img2_warp
% [xgrid,ygrid] = meshgrid(1:ncols, 1:nrows);
if do_affine 
%     xcen = mean(x);
%     ycen = mean(y);
%     x = x - xcen;
%     y = y - ycen;
    xgrid_warp = affine_warp(1,1)*x + affine_warp(1,2)*y + affine_warp(1,3) + x_cen;
    ygrid_warp = affine_warp(2,1)*x + affine_warp(2,2)*y + affine_warp(2,3) + y_cen;
    total_warp = affine_warp;
else
    xgrid_warp = x + translation(1);
    ygrid_warp = y + translation(2);
    total_warp = translation;
end

if nchannels==3
    [img2_warp, out_of_bounds] = mexInterp2(img2_color, xgrid_warp, ygrid_warp);
    img2_warp = reshape(img2_warp, [nrows ncols 3]);
else
    [img2_warp, out_of_bounds] = mexInterp2(img2_smoothed, xgrid_warp, ygrid_warp);
    img2_warp = reshape(img2_warp, [nrows ncols] );
end

if nargout==3
    valid_region = reshape(~out_of_bounds, [nrows ncols nchannels]);
    valid_region = valid_region(:,:,1);
end

% De-normalize the data
img2_warp = img_norm(img2_warp, norm_max, norm_min);

% if originally_uint8 && isa(img2_warp, 'double')
%     img2_warp = uint8(img_norm(img2_warp, 0, 255)); 
% end

% Convert back to the original class of data
if ~strcmp(original_class, 'double')
    eval(['img2_warp = ' original_class '(img2_warp);']);
end

