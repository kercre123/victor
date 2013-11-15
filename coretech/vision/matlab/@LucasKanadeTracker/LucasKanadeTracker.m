classdef LucasKanadeTracker < handle
    
    properties(SetAccess = 'protected', Dependent = true)
        corners;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        tform;
        err = Inf;
        target;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        tformType;
        
        xgrid;
        ygrid;
        initCorners;
        
        xcen;
        ycen;
        width;
        height;
            
        W; % weights, per scale
        
        % Translation Only
        A_trans;
        %AtW_trans;
        %AtWA_trans;
        
        % Full transformation (depending on tformType)
        A_full;
        
        ridgeWeight = 1e-6;
        
        % Parameters
        minSize;
        numScales;
        finestScale;
        convergenceTolerance;
        maxIterations;
        
        debugDisplay;
        h_axes;
        h_errPlot;
        
        useBlurring;
        useNormalization;
    end
    
    methods % public methods
        
        function this = LucasKanadeTracker(firstImg, targetMask, varargin)
            
            MinSize = 8;
            NumScales = [];
            DebugDisplay = false;
            Type = 'translation';
            UseBlurring = true;
            UseNormalization = false;
            RidgeWeight = 0;
            TrackingResolution = size(firstImg(:,:,1));
            
            parseVarargin(varargin{:});
            
            this.minSize = MinSize;
            this.useNormalization = UseNormalization;
            this.ridgeWeight = RidgeWeight;
            
            this.debugDisplay = DebugDisplay;
            this.tformType = Type;
            
            this.useBlurring = UseBlurring;
            
            this.target = cell(1, this.numScales);
            
            this.xgrid = cell(1, this.numScales);
            this.ygrid = cell(1, this.numScales);
            
            [nrows,ncols,nbands] = size(firstImg);
            firstImg = im2double(firstImg);
            if nbands>1
                firstImg = mean(firstImg,3);
            end
            
            %this.numScales = ceil( log2(min(nrows,ncols)/this.minSize) );
            
            if isequal(size(targetMask), [4 2])
                % 4 corners provided
                x = targetMask(:,1);
                y = targetMask(:,2);
                %xmin = min(x); xmax = max(x);
                %ymin = min(y); ymax = max(y);
            else
                % binary mask provided
                assert(isequal(size(targetMask), size(firstImg)));
                [y,x] = find(targetMask);
                
                % Convert to corners
                xmin = min(x); xmax = max(x);
                ymin = min(y); ymax = max(y);
                x = [xmin xmin xmax xmax]';
                y = [ymin ymax ymin ymax]';
            end
            
            %{
            origResWidth  = xmax - xmin + 1;
            origResHeight = ymax - ymin + 1;
            
            
            % Downsample the first image so that the target is roughly the
            % desired tracking resolution.  Adjust x,y coordinates of the
            % target accordingly.
            if ~isempty(TrackingResolution)
                if isscalar(TrackingResolution)
                    % Downsample factor provided
                   TrackingResolution = [ncols nrows]/TrackingResolution;
                end
                desiredDiagonal = sqrt(sum(TrackingResolution.^2));
                currentDiagonal = sqrt( (xmax-xmin)^2 + (ymax-ymin)^2 );
                
                if currentDiagonal > desiredDiagonal
                    % Downsample, being careful to scale everything around
                    % the image center so the scaled x,y coordinates end up
                    % in the corresponding place at lower resolution.
                    scale = desiredDiagonal/currentDiagonal;
                    
                    X = linspace(-TrackingResolution(1)/2, TrackingResolution(1)/2, ncols);
                    Y = linspace(-TrackingResolution(2)/2, TrackingResolution(2)/2, nrows);
                    [xi,yi] = meshgrid( ...
                        linspace(-TrackingResolution(1)/2, TrackingResolution(1)/2, scale*ncols), ...
                        linspace(-TrackingResolution(2)/2, TrackingResolution(2)/2, scale*nrows));
                    
                    firstImg = interp2(X,Y, firstImg, xi, yi, 'linear');
                    origResWidth  = scale*origResWidth;
                    origResHeight = scale*origResHeight;
                    
                    %[xi,yi] = meshgrid(1:1/scale:ncols, 1:1/scale:nrows);
                    %firstImg = interp2(firstImg, xi, yi, 'nearest');
                end
                
                x = (x-1)/ncols * TrackingResolution(1) + 1;
                y = (y-1)/nrows * TrackingResolution(2) + 1;
                
                Ximg = linspace(1, TrackingResolution(1), size(firstImg,2));
                Yimg = linspace(1, TrackingResolution(2), size(firstImg,1));
                
            else
                TrackingResolution = [ncols nrows];
                Ximg = 1:ncols;
                Yimg = 1:nrows;
            end
            %}
            
            this.initCorners = [x y];
            xmin = floor(min(x)); xmax = ceil(max(x));
            ymin = floor(min(y)); ymax = ceil(max(y));
            
            this.height = ymax - ymin + 1;
            this.width  = xmax - xmin + 1;
            
            maskBBox = [xmin ymin xmax-xmin ymax-ymin];
            
            %targetMask = false(TrackingResolution(2), TrackingResolution(1));
            targetMask = false(nrows,ncols);
            targetMask(maskBBox(2):(maskBBox(2)+maskBBox(4)), ...
                maskBBox(1):(maskBBox(1)+maskBBox(3))) = true;
            
            if this.debugDisplay
                namedFigure('LK Debug')
                subplot 221, imagesc(firstImg), axis image
                overlay_image(targetMask, 'r', 0, 0.35);
                
                this.h_axes = subplot(2,2,2);
                imagesc(firstImg); axis image
                
                subplot(2,2,[3 4]), cla
                this.h_errPlot = plot(nan,nan, 'r-x');
            end
                      
            %[this.ygrid{1},this.xgrid{1}] = find(targetMask);
 
            if isempty(NumScales)
                %this.numScales = ceil( log2(min(TrackingResolution)/this.minSize) );
                this.numScales = ceil( log2(min(this.width,this.height)/this.minSize) );
            else
                assert(isscalar(NumScales));
                this.numScales = NumScales;
            end
            
            this.finestScale = 1;
            Downsample = 1;
            if ~isempty(TrackingResolution)
               
                if isscalar(TrackingResolution)
                    % Scalar downsampling factor provided.
                    Downsample = TrackingResolution;
                    TrackingResolution = [ncols nrows]/Downsample;
                else
                    Downsample = mean([ncols nrows]./TrackingResolution);
                end
                
                this.finestScale = floor(log2(min(nrows,ncols)/min(TrackingResolution)));
                
            end
            
            this.xcen = (xmax + xmin)/2;
            this.ycen = (ymax + ymin)/2;
            
            %this.xgrid{1} = this.xgrid{1} - this.xcen;
            %this.ygrid{1} = this.ygrid{1} - this.ycen;
            
            %width = max(this.xgrid{1}(:)) - min(this.xgrid{1}(:));
            %height = max(this.ygrid{1}(:)) - min(this.ygrid{1}(:));
            W_sigma = sqrt(this.width^2 + this.height^2)/2;
            %W_sigma = 1/2;
            
            this.A_trans = cell(1, this.numScales);
            this.A_full  = cell(1, this.numScales);
            this.W       = cell(1, this.numScales);
            %{
            this.AtW_trans  = cell(1, this.numScales);
            this.AtWA_trans = cell(1, this.numScales);
            this.AtW_scale  = cell(1, this.numScales);
            this.AtWA_scale = cell(1, this.numScales);
            %}
            
            %imgBlur = separable_filter(firstImg, gaussian_kernel(1/3));
            
            this.tform = eye(3);
            
            for i_scale = this.finestScale:this.numScales
                
                spacing = 2^(i_scale-1);
                
                [this.xgrid{i_scale}, this.ygrid{i_scale}] = meshgrid( ...
                    linspace(-this.width/2,  this.width/2,  this.width/spacing), ...
                    linspace(-this.height/2, this.height/2, this.height/spacing));
                
                if this.useBlurring
                    imgBlur = separable_filter(firstImg, gaussian_kernel(spacing/3));
                else
                    imgBlur = firstImg;
                end
                
                [xi, yi] = this.getImagePoints(i_scale);
                this.target{i_scale} = interp2(imgBlur, xi, yi, 'linear');
                
                if this.useNormalization
                    this.target{i_scale} = (this.target{i_scale} - ...
                        mean(this.target{i_scale}(:))) / ...
                        std(this.target{i_scale}(:));
                end
                
                %targetBlur = separable_filter(this.target{i_scale}, gaussian_kernel(i_scale));
                targetBlur = this.target{i_scale};
                
                %Ix = (image_right(targetBlur) - targetBlur) * spacing;
                %Iy = (image_down(targetBlur) - targetBlur) * spacing;
                Ix = (image_right(targetBlur) - image_left(targetBlur))/2 * spacing;
                Iy = (image_down(targetBlur) - image_up(targetBlur))/2 * spacing;
                
                
                %                 targetRight = interp2(imgBlur, ...
                %                     this.xgrid{i_scale} + this.xcen + spacing, ...
                %                     this.ygrid{i_scale} + this.ycen, 'linear');
                %
                %                 targetDown = interp2(imgBlur, ...
                %                     this.xgrid{i_scale} + this.xcen, ...
                %                     this.ygrid{i_scale} + this.ycen + spacing, 'linear');
                %
                %                 Ix = (targetRight - this.target{i_scale});% * spacing;
                %                 Iy = (targetDown  - this.target{i_scale});% * spacing;
                
                %W_mask = 1;
                % Don't need to use Ximg, Yimg here because the mask is
                % already defined in TrackingResolution coordinates.
                W_mask = interp2(double(targetMask), xi, yi, 'linear', 0);
                
                % Gaussian weighting function to give more weight to center of target
                W_ = W_mask .* exp(-((this.xgrid{i_scale}).^2 + ...
                    (this.ygrid{i_scale}).^2) / (2*(W_sigma)^2));
                this.W{i_scale} = W_(:);
                
                if Downsample ~= 1
                    % Adjust the target coordinates from original
                    % resolution to tracking resolution. Note that we don't
                    % have to scale around (1,1) here because these
                    % coordinates are already referenced to the target's
                    % center (0,0), i.e. they get shifted by (xcen,ycen),
                    % which gets scaled below.
                    this.xgrid{i_scale} = this.xgrid{i_scale}/Downsample;
                    this.ygrid{i_scale} = this.ygrid{i_scale}/Downsample;
                end
                
                % System of Equations for Translation Only
                this.A_trans{i_scale} = [Ix(:) Iy(:)];
                
                switch(this.tformType)
                    case 'translation'
                        % translation only: nothing to do
                        
                    case 'rotation'
                        % Image-plane rotation + translation (no
                        % skew/scale)
                        X = this.xgrid{i_scale}(:);
                        Y = this.ygrid{i_scale}(:);
                        this.A_full{i_scale} = [ ...
                            X.*Ix(:) Y.*Iy(:) (-Y.*Ix(:) + X.*Iy(:))];
                            
                    case 'affine'
                        % Full affine transformation (rotation, scale,
                        % skew, translation)                        
                        this.A_full{i_scale} = [ ...
                            this.xgrid{i_scale}(:).*Ix(:) this.ygrid{i_scale}(:).*Ix(:) Ix(:) ...
                            this.xgrid{i_scale}(:).*Iy(:) this.ygrid{i_scale}(:).*Iy(:) Iy(:)];
                        
                    case 'scale'
                        % Translation + scaling only
                        this.A_full{i_scale} = [this.A_trans{i_scale} ...
                            (this.xgrid{i_scale}(:).*Ix(:) + this.ygrid{i_scale}(:).*Iy(:))];
                        
                    case 'homography'
                        % Full perspective homography
                        X = this.xgrid{i_scale}(:);
                        Y = this.ygrid{i_scale}(:);
                        this.A_full{i_scale} = [ ...
                            X.*Ix(:) Y.*Ix(:) Ix(:) ...
                            X.*Iy(:) Y.*Iy(:) Iy(:) ...
                            -X.^2.*Ix(:)-X.*Y.*Ix(:) -X.*Y.*Ix(:)-Y.^2.*Iy(:)];
                        
                    otherwise
                        error('Unecognized transformation type "%s".', ...
                            this.tformType);
                end
                
                %                 if i_scale < this.numScales
                %                     % Downsample
                %
                %
                % %                     %targetMask = targetMask(1:2:end,1:2:end);
                % %                     temp = false(size(targetMask));
                % %                     inc = 2^(i_scale);
                % %                     temp(1:inc:end,1:inc:end) = targetMask(1:inc:end,1:inc:end);
                % %                     [this.ygrid{i_scale+1},this.xgrid{i_scale+1}] = find(temp);
                % %
                % %                     this.xgrid{i_scale+1} = this.xgrid{i_scale+1} - this.xcen;
                % %                     this.ygrid{i_scale+1} = this.ygrid{i_scale+1} - this.ycen;
                %
                %                     %imgBlur = separable_filter(firstImg, gaussian_kernel(inc/3));
                %
                %                     this.target{i_scale+1} = interp2(img, ... imgBlur, ...
                %                         this.xgrid{i_scale+1} + this.xcen, ...
                %                         this.ygrid{i_scale+1} + this.ycen, 'linear');
                %                 end
            end % FOR each scale
            
            if Downsample ~= 1
                % Adjust the target center point and the original corner
                % locations from original resolution to tracking resolution
                % coordinates
                this.xcen = (this.xcen-1)/Downsample + 1;
                this.ycen = (this.ycen-1)/Downsample + 1;
                this.initCorners = (this.initCorners-1)/Downsample + 1;
            end
            
            return;
            
        end % CONSTRUCTOR
        
        function [xi, yi] = getImagePoints(this, varargin)
            
            if nargin==2
                i_scale = varargin{1};
                if i_scale < this.finestScale
                    warning(['Finest scale available is %d. Returning ' ...
                        'that instead of requested scale %d.'], ...
                        this.finestScale, i_scale);
                    i_scale = this.finestScale;
                end
                
                x = this.xgrid{i_scale};
                y = this.ygrid{i_scale};
            else
                assert(nargin==3, 'Expecting i_scale or (x,y).');
                x = varargin{1} - this.xcen;
                y = varargin{2} - this.ycen;
            end
            
            xi = this.tform(1,1)*x + this.tform(1,2)*y + this.tform(1,3);
            
            yi = this.tform(2,1)*x + this.tform(2,2)*y + this.tform(2,3);
            
            if strcmp(this.tformType, 'homography')
                zi = this.tform(3,1)*x + this.tform(3,2)*y + this.tform(3,3);
                xi = xi ./ zi;
                yi = yi ./ zi;
            end
            
            xi = xi + this.xcen;
            yi = yi + this.ycen;
            
        end
        
        function converged = track(this, nextImg, varargin)
            
            MaxIterations = 25;
            ConvergenceTolerance = 1e-3;
            
            parseVarargin(varargin{:});
            
            this.maxIterations = MaxIterations;
            this.convergenceTolerance = ConvergenceTolerance;
            
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
                
                % Translation only
                converged = this.trackHelper(imgBlur{i_scale}, i_scale, false);
                
                if converged && ~strcmp(this.tformType, 'translation')
                    % Affine OR Translation + Scale
                    converged = this.trackHelper(imgBlur{i_scale}, i_scale, true);
                end
                
            end % FOR each scale
            
            % Compute final error
            [xi,yi] = this.getImagePoints(this.finestScale);
            
            imgi = interp2(imgBlur{this.finestScale}, xi(:), yi(:), 'linear');
            inBounds = ~isnan(imgi);
            
            if this.useNormalization
                imgi = (imgi - mean(imgi(inBounds)))/std(imgi(inBounds));
            end
            It = this.target{this.finestScale}(:) - imgi;
            
            this.err = mean(abs(It(inBounds)));
            
            
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
        
        function c = get.corners(this)
            [x,y] = this.getImagePoints(this.initCorners(:,1), this.initCorners(:,2));
            c = [x y];
        end
        
    end % public methods
    
    methods(Access = 'protected')
        
        function converged = trackHelper(this, img, i_scale, translationDone)
            
            spacing = 2^(i_scale-1);
            
            xPrev = 0; %this.xgrid{i_scale};
            yPrev = 0; %this.ygrid{i_scale};
            
            iteration = 1;
            tform_orig = this.tform;
            
            converged = false;
            while iteration < this.maxIterations
                
                [xi, yi] = this.getImagePoints(i_scale);
                
                % RMS error between pixel locations according to current
                % transformation and previous locations. 
                change = sqrt(mean((xPrev(:)-xi(:)).^2 + (yPrev(:)-yi(:)).^2));
                if change < this.convergenceTolerance*spacing
                    converged = true;
                    break;
                end
                
                imgi = interp2(img, xi(:), yi(:), 'linear');
                inBounds = ~isnan(imgi);
                
                if this.useNormalization
                    imgi = (imgi - mean(imgi(inBounds)))/std(imgi(inBounds));
                end
                
                It = imgi - this.target{i_scale}(:);
                
                %It(isnan(It)) = 0;
                %inBounds = true(size(It));
                
                if numel(inBounds) < 16
                    warning('Template drifted too far out of image.');
                    break;
                end
                
                this.err = mean(abs(It(inBounds)));
                
                
                %namedFigure('It')
                %imagesc(reshape(It, size(this.xgrid{i_scale}))), axis image,
                %title(sprintf('Error=%.3f, Previous=%.3f, AbsDiff=%.3f', ...
                %    this.err, prevErr, abs(err-prevErr))), pause(.1)
               
                
                %hold off, imagesc(img), axis image, hold on
                %plot(xi, yi, 'rx'); drawnow
                % %                 [nrows,ncols] = size(img);
                % %                 temp = zeros(nrows,ncols);
                % %                 temp(sub2ind([nrows ncols], ...
                % %                     max(1,min(nrows,round(yi))), ...
                % %                     max(1,min(ncols,round(xi))))) = It;
                % %                 hold off, imagesc(temp), axis image, drawnow
                
                
                if translationDone
                    A = this.A_full{i_scale};
                    %{
                    AtWA = this.A_scale{i_scale}(inBounds,:)'*(this.W{i_scale}(:,ones(1,3)).*this.A_scale{i_scale}(inBo
                    AtWA = this.AtWA_scale{i_scale};
                    AtW = this.AtW_scale{i_scale};
                    %}
                else
                    A = this.A_trans{i_scale};
                    %{
                    AtWA = this.AtWA_trans{i_scale};
                    AtW = this.AtW_trans{i_scale};
                    %}
                end
                
                AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';
                AtWA = AtW*A(inBounds,:) + diag(this.ridgeWeight*ones(1,size(A,2)));
                
                b = AtW*It(inBounds);
                
                update = AtWA\b;
                %update = least_squares_norm(AtWA, b);
                %update = robust_least_squares(AtWA, b);
                
                %err = norm(b-AtWA*update);
                
                %                 if this.debugDisplay
                %                     edata = [get(this.h_errPlot, 'YData') err];
                %                     xdata = 1:(length(edata));
                %                     set(this.h_errPlot, 'XData', xdata, 'YData', edata);
                %                     set(get(this.h_errPlot, 'Parent'), ...
                %                         'XLim', [1 length(edata)], ...
                %                         'YLim', [0 1.1*max(edata)]);
                %                 end
                
                %update = spacing * update;
                
                % Compose the update with the current transformation
                if translationDone
                    
                    switch(this.tformType)
                        case 'rotation'
                            cosTheta = cos(update(3));
                            sinTheta = sin(update(3));
                            tformUpdate = [cosTheta -sinTheta update(1);
                                sinTheta cosTheta update(2); 
                                0 0 1];
                            
                        case 'affine'
                            tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; zeros(1,3)];
                            
                        case 'scale'
                            tformUpdate = [(1+update(3)) 0 update(1);
                                0 (1+update(3)) update(2);
                                0 0 1];
                            
                        case 'homography'
                            tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
                            
                        otherwise
                            error('Should not get here.');
                    end
                    
                    % TODO: hardcode this inverse
                    %this.tform = this.tform*inv(tformUpdate);
                    this.tform = this.tform / tformUpdate;
                else
                    %this.tx = this.tx + update(1);
                    %this.ty = this.ty + update(2);
                    this.tform(1:2,3) = this.tform(1:2,3) - update;
                end
                
                iteration = iteration + 1;
                
                %change = abs(err - prevErr);
                %change = err;
                %change = abs(update - prevUpdate) ./ (abs(prevUpdate)+eps);
                
                
                if this.debugDisplay
                    edata = [get(this.h_errPlot, 'YData') change];
                    xdata = 1:(length(edata));
                    set(this.h_errPlot, 'XData', xdata, 'YData', edata);
                    set(get(this.h_errPlot, 'Parent'), ...
                        'XLim', [1 length(edata)], ...
                        'YLim', [0 max(eps,1.1*max(edata))]);
                    
                    if converged
                        h = get(this.h_errPlot, 'Parent');
                        hold(h, 'on')
                        plot(xdata(end), edata(end), 'go', 'Parent', h);
                        hold(h, 'off');
                    end
                end
                
                %prevErr = err;
                xPrev = xi;
                yPrev = yi;
                
            end % WHILE not converged
            
            if ~converged
                this.tform = tform_orig;
            end
            
        end % function trackHelper
        
    end % protected methods
    
end % CLASSDEF LucasKanadeTracker
