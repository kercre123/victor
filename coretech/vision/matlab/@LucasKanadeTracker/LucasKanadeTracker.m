classdef LucasKanadeTracker < handle
    
    properties(SetAccess = 'protected', Dependent = true)
        corners;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        tform;
        xcen;
        ycen;
        
        initCorners;
        
        err = Inf;
        target;
        finestScale;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        tformType;
        
        xgrid;
        ygrid;
        %initCorners;
        
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
        convergenceTolerance;
        errorTolerance;
        maxIterations;
        
        debugDisplay;
        h_axes;
        h_errPlot;
        
        useBlurring;
        useNormalization;
        
        approximateGradientMargins;
    end
    
    methods % public methods
        
        function this = LucasKanadeTracker(firstImg, targetMask, varargin)
            
            MinSize = 8;
            NumScales = [];
            Weights = [];
            DebugDisplay = false;
            Type = 'translation';
            UseBlurring = true;
            UseNormalization = false;
            RidgeWeight = 0;
            TrackingResolution = [size(firstImg,2), size(firstImg,1)];
            ErrorTolerance = [];
            ApproximateGradientMargins = true;
            SampleNearEdges = false;
            
            parseVarargin(varargin{:});
            
            this.minSize = MinSize;
            this.useNormalization = UseNormalization;
            this.ridgeWeight = RidgeWeight;
            this.errorTolerance = ErrorTolerance;
            
            this.debugDisplay = DebugDisplay;
            this.tformType = Type;
            
            this.useBlurring = UseBlurring;
            
            this.approximateGradientMargins = ApproximateGradientMargins;
            
            this.target = cell(1, this.numScales);
            
            this.xgrid = cell(1, this.numScales);
            this.ygrid = cell(1, this.numScales);
            
            [nrows,ncols,nbands] = size(firstImg);
            firstImg = im2double(firstImg);
            if nbands>1
                firstImg = mean(firstImg,3);
            end
                       
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
                       
            this.initCorners = [x y];
            xmin = floor(min(x)); xmax = ceil(max(x));
            ymin = floor(min(y)); ymax = ceil(max(y));
            
            this.height = ymax - ymin + 1;
            this.width  = xmax - xmin + 1;
            
            maskBBox = [xmin ymin xmax-xmin ymax-ymin];
            
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
                       
            if isempty(NumScales)
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
                    % [ncols nrows] tracking resolution provided, compute
                    % the corresponding downsampling factor
                    Downsample = mean([ncols nrows]./TrackingResolution);
                end
                
                % Figure out the finest scale we need to use to track at the
                % specified resolution
                this.finestScale = max(1, floor(log2(min(nrows,ncols)/min(TrackingResolution))));
                
            end % IF TrackingResolution not empty
                        
            this.xcen = (xmax + xmin)/2;
            this.ycen = (ymax + ymin)/2;
            
            W_sigma = sqrt(this.width^2 + this.height^2)/2;
                        
            this.A_trans = cell(1, this.numScales);
            this.A_full  = cell(1, this.numScales);
            this.W       = cell(1, this.numScales);
            %{
            this.AtW_trans  = cell(1, this.numScales);
            this.AtWA_trans = cell(1, this.numScales);
            this.AtW_scale  = cell(1, this.numScales);
            this.AtWA_scale = cell(1, this.numScales);
            %}
            
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
                
                if this.approximateGradientMargins
                    % Using centered differences instead of forward differences
                    % for the derivatives seems to make a big difference in
                    % performance!
                
                    %Ix = (image_right(targetBlur) - targetBlur) * spacing;
                    %Iy = (image_down(targetBlur) - targetBlur) * spacing;
                    Ix = (image_right(targetBlur) - image_left(targetBlur))/2 * spacing;
                    Iy = (image_down(targetBlur) - image_up(targetBlur))/2 * spacing;
                else 
                    Ix = zeros(size(targetBlur));
                    Ix(2:(end-1), 2:(end-1)) = (targetBlur(2:(end-1), 3:end) - targetBlur(2:(end-1), 1:(end-2)))/2 * spacing;
                    
                    Iy = zeros(size(targetBlur));
                    Iy(2:(end-1), 2:(end-1)) = (targetBlur(3:end, 2:(end-1)) - targetBlur(1:(end-2), 2:(end-1)))/2 * spacing;
                end
                
                W_mask = interp2(double(targetMask), xi, yi, 'linear', 0);
                
                % Gaussian weighting function to give more weight to center of target
                if isempty(Weights)
                    W_ = W_mask .* exp(-((this.xgrid{i_scale}).^2 + ...
                        (this.ygrid{i_scale}).^2) / (2*(W_sigma)^2));
                else
                    W_ = W_mask .* interp2(Weights, xi, yi, 'linear', 0);
                end
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
                
                if SampleNearEdges 
                    
                    % Compute the gradient magnitude and then choose pixels
                    % which are "edges", i.e. that have non-trivial
                    % gradient magnitude and are local maxima when
                    % comparing to neighbors in the +/- direction of that
                    % gradient.
                    mag = sqrt(Ix.^2 + Iy.^2);
                    Ix_norm = Ix./max(eps,mag);
                    Iy_norm = Iy./max(eps,mag);
                    left  = interp2(xi, yi, mag, xi+spacing*Ix_norm, yi+spacing*Iy_norm, 'nearest', 0);
                    right = interp2(xi, yi, mag, xi-spacing*Ix_norm, yi-spacing*Iy_norm, 'nearest', 0);
                    sampleIndex = mag > left & mag > right & mag > 0.01;
                    %sampleIndex = imdilate(sampleIndex, [0 1 0; 1 1 1; 0 1 0]);
                    
                    this.xgrid{i_scale} = this.xgrid{i_scale}(sampleIndex);
                    this.ygrid{i_scale} = this.ygrid{i_scale}(sampleIndex);
                    
                    Ix = Ix(sampleIndex);
                    Iy = Iy(sampleIndex);
                    
                    this.target{i_scale} = this.target{i_scale}(sampleIndex);
                    
                    this.W{i_scale} = this.W{i_scale}(sampleIndex);
                    
                    fprintf('Sampled %.1f%% of the template pixels at scale %d.\n', ...
                        sum(sampleIndex(:))/length(sampleIndex(:))*100, i_scale);
                        
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
                            Ix(:) Iy(:) (-Y.*Ix(:) + X.*Iy(:))];
                            
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
                            -X.^2.*Ix(:)-X.*Y.*Iy(:) -X.*Y.*Ix(:)-Y.^2.*Iy(:)];
                        
                    otherwise
                        error('Unecognized transformation type "%s".', ...
                            this.tformType);
                end
                
            end % FOR each scale
            
            if Downsample ~= 1
                % Adjust the target center point and the original corner
                % locations from original resolution to tracking resolution
                % coordinates
                
                % Scale around (1,1)
                this.xcen = (this.xcen-1)/Downsample + 1;
                this.ycen = (this.ycen-1)/Downsample + 1;
                this.initCorners = (this.initCorners-1)/Downsample + 1;
                
                % Scale around image center
                %this.xcen = (this.xcen - ncols/2)/Downsample + ncols/Downsample/2;
                %this.ycen = (this.ycen - nrows/2)/Downsample + nrows/Downsample/2;
                %this.initCorners(:,1) = (this.initCorners(:,1) - ncols/2)/Downsample + ncols/Downsample/2;
                %this.initCorners(:,2) = (this.initCorners(:,2) - nrows/2)/Downsample + nrows/Downsample/2;
            end
            
            return;
            
        end % CONSTRUCTOR
        
        function c = get.corners(this)
            [x,y] = this.getImagePoints(this.initCorners(:,1), this.initCorners(:,2));
            c = [x y];
        end
        
        function set_tform(this, tformIn)
           assert(isequal(size(tformIn), [3 3]), 'Transformation must be 3x3.');
           this.tform = tformIn;
        end
    end % public methods
    
    methods(Access = 'protected')
        
        converged = trackHelper(this, img, i_scale, translationDone);           
        
    end % protected methods
    
end % CLASSDEF LucasKanadeTracker
