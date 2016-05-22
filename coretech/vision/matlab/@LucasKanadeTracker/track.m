function [converged, reason] = track(this, nextImg, varargin)
% Update the tracker position using the next image.

MaxIterations = 25;
ConvergenceTolerance = 1e-3;
IntensityErrorTolerance = [];
IntensityErrorFraction = 0.25;

parseVarargin(varargin{:});

reason = '';

this.maxIterations = MaxIterations;
this.convergenceTolerance = ConvergenceTolerance;
% this.errorTolerance = ErrorTolerance;

nextImg = im2double(nextImg);
if size(nextImg,3) > 1
    nextImg = mean(nextImg,3);
end

imgBlur = cell(1, this.numScales);
imgBlur{1} = nextImg;
prevSigma = 0;

if this.useBlurring
    for i_scale = 2:this.numScales
        % Pixel spacing for this scale
        spacing = 2^(i_scale-1);
        
        sigma = spacing/3;
        addlSigma = sqrt(sigma^2 - prevSigma^2);
        
        imgBlur{i_scale} = mexGaussianBlur(imgBlur{i_scale-1}, addlSigma, 2);
    end
else
    [imgBlur{:}] = deal(nextImg);
end

for i_scale = this.numScales:-1:this.finestScale
    % TODO: just blur previously-blurred image
    %imgBlur = separable_filter(imgBlur, gaussian_kernel(spacing/3));
    %imgBlur = mexGaussianBlur(nextImg, spacing/3, 2);
    
    if strcmp(this.tformType, 'planar6dof')
        [converged, numIterations] = this.trackHelper(imgBlur{i_scale}, i_scale, true);
    else
        % First do translation only
        [converged, numIterations] = this.trackHelper(imgBlur{i_scale}, i_scale, false);
        
        if converged && ~strcmp(this.tformType, 'translation')
            % Non-translation-only trackers
            [converged, numIterations] = this.trackHelper(imgBlur{i_scale}, i_scale, true);
        end
    end
    
end % FOR each scale

% Compute final error for verification
if ~isempty(IntensityErrorTolerance)
    if ~isempty(this.xgridFull{i_scale})
        % For sampled trackers
        [xi, yi] = this.getImagePoints(this.xgridFull{i_scale}, ...
            this.ygridFull{i_scale});
        template = this.targetFull{this.finestScale}(:);
    else
        [xi,yi] = this.getImagePoints(this.finestScale);
        template = this.target{this.finestScale}(:);
    end
    
    imgi = interp2(imgBlur{this.finestScale}, xi(:), yi(:), 'nearest');
    inBounds = ~isnan(imgi);
    
    if this.useNormalization
        stddev = std(imgi(inBounds));
        imgi = (imgi - mean(imgi(inBounds))) ./ stddev;
        IntensityErrorTolerance = IntensityErrorTolerance / stddev;
    end
    
    It = template - imgi;
    %this.err = mean(abs(It(inBounds)));
    this.err = sum(abs(It(inBounds)) > IntensityErrorTolerance) / sum(inBounds);
    
else
    this.err = -1;
end

if ~converged
    reason = 'Tracker failed to converge due to parameter/quad change.';
elseif isnan(this.err) || this.err > IntensityErrorFraction
    reason = 'Tracker failed to converge due to intensity error.';
    converged = false;
else
    reason = sprintf('Tracker converged in %d iterations.', numIterations);
end

% if converged && strcmp(this.tformType, 'planar6dof')
%     this.plotPose();    
% end

if this.debugDisplay
    hold(this.h_axes, 'off')
    imagesc(nextImg, 'Parent', this.h_axes)
    axis(this.h_axes, 'image');
    hold(this.h_axes, 'on')
    
    xx = this.tform(1,1)*this.xgrid{1} + ...
        this.tform(1,2)*this.ygrid{1} +  ...
        this.tform(1,3);
    
    yy = this.tform(2,1)*this.xgrid{1} + ...
        this.tform(2,2)*this.ygrid{1} +  ...
        this.tform(2,3);
    
    plot(xx + this.xcen, yy + this.ycen, 'r.', ...
        'Parent', this.h_axes);
    
    %                 [nrows,ncols,~] = size(nextImg);
    %                 targetMask = false(nrows,ncols);
    %                 index = sub2ind([nrows ncols], ...
    %                     round(this.scale*this.ygrid{1} + this.ty), ...
    %                     round(this.scale*this.xgrid{1} + this.tx));
    %                 targetMask(index) = true;
    %                 overlay_image(targetMask, 'r', 0.35);
    
    title(this.h_axes, sprintf('Translation = (%.2f,%.2f), Scale = %.2f', ...
        this.tform(1,3), this.tform(2,3), this.tform(1,1)));
    
    drawnow
end

end % FUNCTION track()