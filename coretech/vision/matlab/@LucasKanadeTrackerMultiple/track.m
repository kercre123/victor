function converged = track(this, nextImages, varargin)
% Update the tracker position using the next image.

assert(iscell(nextImages));
assert(length(nextImages) == size(this.target,1));

MaxIterations = 25;
ConvergenceTolerance = 1e-3;
ErrorTolerance = [];

parseVarargin(varargin{:});

this.maxIterations = MaxIterations;
this.convergenceTolerance = ConvergenceTolerance;
this.errorTolerance = ErrorTolerance;

imagesBlurred = cell(length(nextImages), this.numScales);

for i_image = 1:length(nextImages)
    nextImages{i_image} = im2double(nextImages{i_image});
    if size(nextImages{i_image},3) > 1
        nextImages{i_image} = mean(nextImages{i_image},3);
    end

    for i_scale = 1:this.numScales
        imagesBlurred{i_image, i_scale} = nextImages{i_image};
    end
end

for i_scale = this.numScales:-1:this.finestScale
    % TODO: just blur previously-blurred image
    %imgBlur = separable_filter(imgBlur, gaussian_kernel(spacing/3));
    %imgBlur = mexGaussianBlur(nextImg, spacing/3, 2);
    
    % Translation only
    converged = this.trackHelper(imagesBlurred(:, i_scale), i_scale, false);
    
    if converged && ~strcmp(this.tformType, 'translation')
        % Affine OR Translation + Scale
        converged = this.trackHelper(imagesBlurred(:, i_scale), i_scale, true);
    end
    
end % FOR each scale

% % Compute final error
% [xi,yi] = this.getImagePoints(this.finestScale);
% 
% imgi = interp2(imgBlur{this.finestScale}, xi(:), yi(:), 'linear');
% inBounds = ~isnan(imgi);
% 
% if this.useNormalization
%     imgi = (imgi - mean(imgi(inBounds)))/std(imgi(inBounds));
% end
% It = this.target{this.finestScale}(:) - imgi;
% 
% this.err = mean(abs(It(inBounds)));
% 
% if isnan(this.err) || (~isempty(this.errorTolerance) && this.err > this.errorTolerance)
%     converged = false;
% end
% 
% if this.debugDisplay
%     hold(this.h_axes, 'off')
%     imagesc(nextImg, 'Parent', this.h_axes)
%     axis(this.h_axes, 'image');
%     hold(this.h_axes, 'on')
%     
%     xx = this.tform(1,1)*this.xgrid{1} + ...
%         this.tform(1,2)*this.ygrid{1} +  ...
%         this.tform(1,3);
%     
%     yy = this.tform(2,1)*this.xgrid{1} + ...
%         this.tform(2,2)*this.ygrid{1} +  ...
%         this.tform(2,3);
%     
%     plot(xx + this.xcen, yy + this.ycen, 'r.', ...
%         'Parent', this.h_axes);
%     
%     %                 [nrows,ncols,~] = size(nextImg);
%     %                 targetMask = false(nrows,ncols);
%     %                 index = sub2ind([nrows ncols], ...
%     %                     round(this.scale*this.ygrid{1} + this.ty), ...
%     %                     round(this.scale*this.xgrid{1} + this.tx));
%     %                 targetMask(index) = true;
%     %                 overlay_image(targetMask, 'r', 0.35);
%     
%     title(this.h_axes, sprintf('Translation = (%.2f,%.2f), Scale = %.2f', ...
%         this.tform(1,3), this.tform(2,3), this.tform(1,1)));
%     
%     drawnow
% end

end % FUNCTION track()