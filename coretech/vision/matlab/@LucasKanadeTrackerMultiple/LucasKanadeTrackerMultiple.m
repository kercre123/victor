classdef LucasKanadeTrackerMultiple < handle

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

        % Full transformation (depending on tformType)
        A_full;

        ridgeWeight;

        % Parameters
        minSize;
        numScales;
        convergenceTolerance;
        errorTolerance;
        maxIterations;

        debugDisplay;
        h_axes;
        h_errPlot;

        useNormalization;
    end

    methods % public methods

        function this = LucasKanadeTrackerMultiple(firstImages, targetMask, varargin)

            assert(iscell(firstImages))
            
            MinSize = 8;
            NumScales = [];
            DebugDisplay = false;
            Type = 'translation';
            UseNormalization = false;
            RidgeWeight = 0;
            TrackingResolution = [size(firstImages{1},2), size(firstImages{1},1)];
            ErrorTolerance = [];

            parseVarargin(varargin{:});

            this.minSize = MinSize;
            this.useNormalization = UseNormalization;
            this.ridgeWeight = RidgeWeight;
            this.errorTolerance = ErrorTolerance;

            this.debugDisplay = DebugDisplay;
            this.tformType = Type;

            numFirstImages = length(firstImages);

            this.target = cell(numFirstImages, this.numScales);

            this.xgrid = cell(1, this.numScales);
            this.ygrid = cell(1, this.numScales);

            [nrows,ncols,nbands] = size(firstImages{1});

            for i = 1:length(firstImages)
                firstImages{i} = im2double(firstImages{i});

                if nbands>1
                    firstImages{i} = mean(firstImages{i},3);
                end

                assert(size(firstImages{i},1) == nrows);
                assert(size(firstImages{i},2) == ncols);
                assert(size(firstImages{i},3) == nbands);
            end

            if isequal(size(targetMask), [4 2])
                % 4 corners provided
                x = targetMask(:,1);
                y = targetMask(:,2);
                %xmin = min(x); xmax = max(x);
                %ymin = min(y); ymax = max(y);
            else
                % binary mask provided
                assert(isequal(size(targetMask), size(firstImages{i})));
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
                subplot 221, imagesc(firstImages{i}), axis image
                overlay_image(targetMask, 'r', 0, 0.35);

                this.h_axes = subplot(2,2,2);
                imagesc(firstImages{i}); axis image

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
                    if(ncols < nrows)
                        disp('Warning: ncols < nrows. These may be swapped');
                    end

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

            this.A_trans = cell(numFirstImages, this.numScales);
            this.A_full  = cell(numFirstImages, this.numScales);
            this.W       = cell(1, this.numScales);

            this.tform = eye(3);

            for i_scale = this.finestScale:this.numScales

                spacing = 2^(i_scale-1);

                [this.xgrid{i_scale}, this.ygrid{i_scale}] = meshgrid( ...
                    linspace(-this.width/2,  this.width/2,  this.width/spacing), ...
                    linspace(-this.height/2, this.height/2, this.height/spacing));

                [xi, yi] = this.getImagePoints(i_scale);

                W_mask = interp2(double(targetMask), xi, yi, 'linear', 0);
                
                % Gaussian weighting function to give more weight to center of target
                W_ = W_mask .* exp(-((this.xgrid{i_scale}).^2 + ...
                    (this.ygrid{i_scale}).^2) / (2*(W_sigma)^2));
                this.W{i_scale} = W_(:);
                                
                for i_image = 1:length(firstImages)
                    this.target{i_image, i_scale} = interp2(firstImages{i_image}, xi, yi, 'linear');
                    
                    if this.useNormalization
                        this.target{i_image, i_scale} = (this.target{i_image, i_scale} - ...
                            mean(this.target{i_image, i_scale}(:))) / ...
                            std(this.target{i_image, i_scale}(:));
                    end
                    
                    targetBlur = this.target{i_image, i_scale};

                    Ix = zeros(size(targetBlur));
                    Ix(2:(end-1), 2:(end-1)) = (targetBlur(2:(end-1), 3:end) - targetBlur(2:(end-1), 1:(end-2)))/2 * spacing;

                    Iy = zeros(size(targetBlur));
                    Iy(2:(end-1), 2:(end-1)) = (targetBlur(3:end, 2:(end-1)) - targetBlur(1:(end-2), 2:(end-1)))/2 * spacing;

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
                    this.A_trans{i_image, i_scale} = [Ix(:) Iy(:)];

                    switch(this.tformType)
                        case 'translation'
                            % translation only: nothing to do

                        case 'rotation'
                            % Image-plane rotation + translation (no
                            % skew/scale)
                            X = this.xgrid{i_scale}(:);
                            Y = this.ygrid{i_scale}(:);
                            this.A_full{i_image, i_scale} = [ ...
                                Ix(:) Iy(:) (-Y.*Ix(:) + X.*Iy(:))];

                        case 'affine'
                            % Full affine transformation (rotation, scale,
                            % skew, translation)
                            this.A_full{i_image, i_scale} = [ ...
                                this.xgrid{i_scale}(:).*Ix(:) this.ygrid{i_scale}(:).*Ix(:) Ix(:) ...
                                this.xgrid{i_scale}(:).*Iy(:) this.ygrid{i_scale}(:).*Iy(:) Iy(:)];

                        case 'scale'
                            % Translation + scaling only
                            this.A_full{i_image, i_scale} = [this.A_trans{i_image, i_scale} ...
                                (this.xgrid{i_scale}(:).*Ix(:) + this.ygrid{i_scale}(:).*Iy(:))];

                        case 'homography'
                            % Full perspective homography
                            X = this.xgrid{i_scale}(:);
                            Y = this.ygrid{i_scale}(:);
                            this.A_full{i_image, i_scale} = [ ...
                                X.*Ix(:) Y.*Ix(:) Ix(:) ...
                                X.*Iy(:) Y.*Iy(:) Iy(:) ...
                                -X.^2.*Ix(:)-X.*Y.*Iy(:) -X.*Y.*Ix(:)-Y.^2.*Iy(:)];

                        otherwise
                            error('Unecognized transformation type "%s".', ...
                                this.tformType);
                    end % switch(this.tformType)
                end % for i_image = 1:length(firstImages)
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

    end % public methods

    methods(Access = 'protected')

        converged = trackHelper(this, img, i_scale, translationDone);

    end % protected methods

end % CLASSDEF LucasKanadeTrackerMultiple
