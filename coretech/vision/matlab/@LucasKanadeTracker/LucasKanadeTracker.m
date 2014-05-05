classdef LucasKanadeTracker < handle
    
    properties(SetAccess = 'protected', Dependent = true)
        corners;
        poseString;
    end
    
    properties(Dependent = true)
        rotationMatrix;
        translation;
    end
    
    properties(GetAccess = 'public', SetAccess = 'protected')
        tform;
        xcen;
        ycen;
                
        xgrid;
        ygrid;
        
        xgridFull; % unsampled
        ygridFull; % unsampled
        targetFull;
        
        initCorners;
        
        err = Inf;
        target;
        finestScale;
        
        % Available in planar6dof mode
        theta_x; theta_y; theta_z;
        tx; ty; tz;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        tformType;

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
        
        ridgeWeight;
        
        % for planar6dof tracker
        K; % calibration matrix
        h_pose;
        headPosition;
        headRotation;
        neckPosition;
                
        % Parameters
        minSize;
        numScales;
        convergenceTolerance;
        %errorTolerance;
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
            TemplateRegionPaddingFraction = 0;
            ApproximateGradientMargins = false;
            %SampleNearEdges = false;
            NumSamples = [];
            SampleGradientMagnitude = true; % if false, samples directly form image data
            MarkerWidth = [];
            CalibrationMatrix = [];
            HeadPosition = [11.45 0 -6]'; % relative to neck
            HeadRotation = [0 0 1; -1 0 0; 0 -1 0]; % relative to neck
            NeckPosition = [-13 0 33.5+14.2]; % relative to robot origin (which is on the ground)
            
            parseVarargin(varargin{:});
            
            this.minSize = MinSize;
            this.useNormalization = UseNormalization;
            this.ridgeWeight = RidgeWeight;
                        
            this.debugDisplay = DebugDisplay;
            this.tformType = Type;
           
            this.useBlurring = UseBlurring;
            
            this.approximateGradientMargins = ApproximateGradientMargins;
               
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
                       
                        
            if strcmp(this.tformType, 'planar6dof')
                assert(~isempty(CalibrationMatrix), 'Calibration is required when using Planar6DoF tracking.');
                assert(~isempty(MarkerWidth), 'Marker3D is required when using Planar6DoF tracking.');
                
                this.headPosition = HeadPosition;
                this.headRotation = HeadRotation;
                this.neckPosition = NeckPosition;
                
                % Defining canonical 3D marker in camera coordinates:
                Marker3D = (MarkerWidth/2)*[-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
                
                camera = Camera('calibration', struct( ...
                    'fc', [CalibrationMatrix(1,1) CalibrationMatrix(2,2)], ...
                    'cc', [0 0], ...
                    'alpha_c', 0, ...
                    'kc', zeros(5,1)));
                
                this.K = CalibrationMatrix;
                
                % Compute the initial homography mapping the 3D points to
                % the image plane
                pose = camera.computeExtrinsics([x-this.K(1,3) y-this.K(2,3)], Marker3D);
                
                
                % Compute Euler angles from R
                this.rotationMatrix = pose.Rmat;
                                
                % Pre-compute some repeatedly-used trig values
                cx = cos(this.theta_x); cy = cos(this.theta_y); cz = cos(this.theta_z);
                sx = sin(this.theta_x); sy = sin(this.theta_y); sz = sin(this.theta_z);
                                
                % Sanity check that Euler angle extraction worked:
                Rcheck = [cy*cz cx*sz+sx*sy*cz sx*sz-cx*sy*cz; -cy*sz cx*cz-sx*sy*sz sx*cz+cx*sy*sz; sy -sx*cy cx*cy];
                if any(abs(pose.Rmat(:)-Rcheck(:)) > 1e-2)
                    warning('Possibly inaccurate RotationMatrix->EulerAngle conversion (Note cos(theta_y)=%f).', cy);
                end
                
                % The translation parameters are just straight from the
                % pose
                this.tx = pose.T(1);
                this.ty = pose.T(2);
                this.tz = pose.T(3);
                
                % Need the partial derivatives at the initial conditions of
                % the rotation parameters
                dr11_dthetaX = 0;
                dr12_dthetaX = -sx*sz + cx*sy*cz;
                dr21_dthetaX = 0;
                dr22_dthetaX = -sx*cz - cx*sy*sz;
                dr31_dthetaX = 0;
                dr32_dthetaX = -cx*cy;
                
                dr11_dthetaY = -sy*cz;
                dr12_dthetaY = sx*cy*cz;
                dr21_dthetaY = sy*sz;
                dr22_dthetaY = -sx*cy*sz;
                dr31_dthetaY = cy;
                dr32_dthetaY = sx*sy;
                
                dr11_dthetaZ = -cy*sz;
                dr12_dthetaZ = cx*cz - sx*sy*sz;
                dr21_dthetaZ = -cy*cz;
                dr22_dthetaZ = -cx*sz - sx*sy*cz;
                dr31_dthetaZ = 0;
                dr32_dthetaZ = 0;
                
                % Compute the initial transform (i.e. homography) from the
                % initial 6DoF parameters
                this.tform = this.Compute6dofTform();
                
                this.initCorners = Marker3D(:,1:2);
                
                %                 % DEBUG DISPLAY
                %                 order = [1 2 4 3 1];
                %                 h_fig = namedFigure('PoseDisplay');
                %                 h_poseAxes = subplot(1,1,1, 'Parent', h_fig);
                %                 hold(h_poseAxes, 'off')
                %                 plot3(Marker3D(order,1), Marker3D(order,2), Marker3D(order,3), ...
                %                     'r', 'LineWidth', 2, 'Parent', h_poseAxes);
                %                 hold(h_poseAxes, 'on')
                %                 this.h_pose(1) = plot3(nan, nan, nan, 'r', 'Parent', h_poseAxes);
                %                 this.h_pose(2) = plot3(nan, nan, nan, 'g', 'Parent', h_poseAxes);
                %                 this.h_pose(3) = plot3(nan, nan, nan, 'b', 'Parent', h_poseAxes);
                %
                %                 axis(h_poseAxes, 'equal')
                %                 grid(h_poseAxes, 'on')
                    
            else
                this.initCorners = [x y];
            
                this.tform = eye(3);
            end
            
            xmin = max(1, floor(min(x))); xmax = min(ncols, ceil(max(x)));
            ymin = max(1, floor(min(y))); ymax = min(nrows, ceil(max(y)));
            
            this.height = ymax - ymin + 1;
            this.width  = xmax - xmin + 1;
           
            if TemplateRegionPaddingFraction > 0
               this.height = this.height * (1 + TemplateRegionPaddingFraction);
               this.width  = this.width  * (1 + TemplateRegionPaddingFraction);
               
               xmid = (xmax+xmin)/2;
               xmin = max(1, round(xmid - this.width/2));
               xmax = min(ncols, round(xmid + this.width/2));
               
               ymid = (ymax+ymin)/2;
               ymin = max(1, round(ymid - this.height/2));
               ymax = min(nrows, round(ymid + this.height/2));
            end
            
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
                     
            this.target = cell(1, this.numScales);
            this.targetFull = cell(1, this.numScales);
            this.xgrid = cell(1, this.numScales);
            this.ygrid = cell(1, this.numScales);
            this.xgridFull = cell(1, this.numScales);
            this.ygridFull = cell(1, this.numScales);
            
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
                        
            if strcmp(this.tformType, 'planar6dof')
                this.xcen = this.K(1,3);
                this.ycen = this.K(2,3);
            else
                this.xcen = (xmax + xmin)/2;
                this.ycen = (ymax + ymin)/2;
            end
            
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
            
            for i_scale = this.finestScale:this.numScales
                
                spacing = 2^(i_scale-1);
                
                if strcmp(this.tformType, 'planar6dof')
                    % Coordinates are 3D plane coordinates
                    M = (1 + TemplateRegionPaddingFraction)*MarkerWidth/2;
                    [this.xgrid{i_scale}, this.ygrid{i_scale}] = meshgrid( ...
                        linspace(-M, M, this.width/spacing), ...
                        linspace(-M, M, this.height/spacing));
                else
                    [this.xgrid{i_scale}, this.ygrid{i_scale}] = meshgrid( ...
                        linspace(-this.width/2,  this.width/2,  this.width/spacing), ...
                        linspace(-this.height/2, this.height/2, this.height/spacing));
                end
                
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
                    %W_ = W_mask .* exp(-((this.xgrid{i_scale}).^2 + ...
                    %    (this.ygrid{i_scale}).^2) / (2*(W_sigma)^2));
                    W_ = W_mask;
                else
                    W_ = W_mask .* interp2(Weights, xi, yi, 'linear', 0);
                end
                this.W{i_scale} = W_(:);
                
                if Downsample ~= 1
                    assert(~strcmp(this.tformType, 'planar6dof'), ...
                        'Planar6DoF tracking and downsampling don''t really make sense...');
                    
                    % Adjust the target coordinates from original
                    % resolution to tracking resolution. Note that we don't
                    % have to scale around (1,1) here because these
                    % coordinates are already referenced to the target's
                    % center (0,0), i.e. they get shifted by (xcen,ycen),
                    % which gets scaled below.
                    this.xgrid{i_scale} = this.xgrid{i_scale}/Downsample;
                    this.ygrid{i_scale} = this.ygrid{i_scale}/Downsample;
                end
                
                if ~isempty(NumSamples) && NumSamples > 0
                    
                    % Compute the gradient magnitude and then choose pixels
                    % which are "edges", i.e. that have non-trivial
                    % gradient magnitude and are local maxima when
                    % comparing to neighbors in the +/- direction of that
                    % gradient.
                    numSamplesCurrent = round(NumSamples/spacing);
                    
                    if numSamplesCurrent < numel(this.xgrid{i_scale})
                        
                        if SampleGradientMagnitude
                            mag = sqrt(Ix.^2 + Iy.^2);
                        else
                            mag = targetBlur;
                        end
                        
                        % Suppress non-local maxima
                        NLMS = (mag > image_up(mag)      & mag > image_down(mag))      | ...
                               (mag > image_left(mag)    & mag > image_right(mag))     | ...
                               (mag > image_upleft(mag)  & mag > image_downright(mag)) | ...
                               (mag > image_upright(mag) & mag > image_downleft(mag));
                        mag(~NLMS) = 0;
                        
                        %{
                    Ix_norm = Ix./max(eps,mag);
                    Iy_norm = Iy./max(eps,mag);
                    left  = interp2(xi, yi, mag, xi+spacing*Ix_norm, yi+spacing*Iy_norm, 'nearest', 0);
                    right = interp2(xi, yi, mag, xi-spacing*Ix_norm, yi-spacing*Iy_norm, 'nearest', 0);
                    sampleIndex = mag > left & mag > right & mag > 0.01;
                    %sampleIndex = imdilate(sampleIndex, [0 1 0; 1 1 1; 0 1 0]);
                        %}
                        
                        [~,sortIndex] = sort(mag(:), 'descend');
                        sampleIndex = sortIndex(1:numSamplesCurrent);
                        
                        % Save the "full" coordinates for final
                        % verification and normalization
                        this.xgridFull{i_scale} = this.xgrid{i_scale};
                        this.ygridFull{i_scale} = this.ygrid{i_scale};
                        if i_scale == this.finestScale
                            this.targetFull{i_scale} = this.target{i_scale};
                        end
                        
                        this.xgrid{i_scale} = this.xgrid{i_scale}(sampleIndex);
                        this.ygrid{i_scale} = this.ygrid{i_scale}(sampleIndex);
                        
                        Ix = Ix(sampleIndex);
                        Iy = Iy(sampleIndex);
                        
                        this.target{i_scale} = this.target{i_scale}(sampleIndex);
                        
                        this.W{i_scale} = this.W{i_scale}(sampleIndex);
                        
                        %fprintf('Sampled %.1f%% of the template pixels at scale %d.\n', ...
                        %    sum(sampleIndex(:))/length(sampleIndex(:))*100, i_scale);
                    end
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
                        
                    case 'planar6dof'
                        % For whatever reason, the canonical 3D plane is 
                        % defined in the X-Z plane rather than X-Y
                        % Params ordered as:
                        % [thetaX thetaY thetaZ tX tY tZ]
                        
                        % Map the image coordinates into 3D coordinates for
                        % computing the template Jacobian
                        X = this.xgrid{i_scale}(:);
                        Y = this.ygrid{i_scale}(:);
                        
                        % three components of the projection (this is Nx3)
                        p = (this.tform*[X Y ones(length(X),1)]')';
                        assert(~any(p(:,3)==0));
                        
                        denomSq = p(:,3).^2;
                        
                        fx = CalibrationMatrix(1,1);
                        fy = CalibrationMatrix(2,2);
                        
                        % Compute all the partial derivatives for computing
                        % the Jacobian w.r.t. each parameter
                        
                        dWu_dtx = fx./p(:,3);
                        dWu_dty = 0;
                        dWu_dtz = -p(:,1) ./ denomSq;
                        
                        dWv_dtx = 0;
                        dWv_dty = fy./p(:,3);
                        dWv_dtz = -p(:,2) ./ denomSq;
                        
                        dWu_dthetaX = (fx*p(:,3).*(dr11_dthetaX*X + dr12_dthetaX*Y) - ...
                            (dr31_dthetaX*X + dr32_dthetaX*Y).*p(:,1)) ./ denomSq;
                        
                        dWu_dthetaY = (fx*p(:,3).*(dr11_dthetaY*X + dr12_dthetaY*Y) - ...
                            (dr31_dthetaY*X + dr32_dthetaY*Y).*p(:,1)) ./ denomSq;
                        
                        dWu_dthetaZ = (fx*p(:,3).*(dr11_dthetaZ*X + dr12_dthetaZ*Y) - ...
                            (dr31_dthetaZ*X + dr32_dthetaZ*Y).*p(:,1)) ./ denomSq;
                        
                        dWv_dthetaX = (fy*p(:,3).*(dr21_dthetaX*X + dr22_dthetaX*Y) - ...
                            (dr31_dthetaX*X + dr32_dthetaX*Y).*p(:,2)) ./ denomSq;
                        
                        dWv_dthetaY = (fy*p(:,3).*(dr21_dthetaY*X + dr22_dthetaY*Y) - ...
                            (dr31_dthetaY*X + dr32_dthetaY*Y).*p(:,2)) ./ denomSq;
                        
                        dWv_dthetaZ = (fy*p(:,3).*(dr21_dthetaZ*X + dr22_dthetaZ*Y) - ...
                            (dr31_dthetaZ*X + dr32_dthetaZ*Y).*p(:,2)) ./ denomSq;
                                               
                        Ix = Ix(:); 
                        Iy = Iy(:);
                        
                        this.A_full{i_scale} = [ ...
                            Ix.*dWu_dthetaX + Iy.*dWv_dthetaX ...
                            Ix.*dWu_dthetaY + Iy.*dWv_dthetaY ...
                            Ix.*dWu_dthetaZ + Iy.*dWv_dthetaZ ...
                            Ix.*dWu_dtx + Iy.*dWv_dtx ...
                            Ix.*dWu_dty + Iy.*dWv_dty ...
                            Ix.*dWu_dtz + Iy.*dWv_dtz ];
                        
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
        
        UpdatePose(this, varargin);
        
        UpdatePoseFromRobotMotion(this, robotTx, robotTy, robotTheta, headAngle);
        
        
        %
        % Set / Get Methods for Dependent Properties
        %
        function c = get.corners(this)
            [x,y] = this.getImagePoints(this.initCorners(:,1), this.initCorners(:,2));
            c = [x y];
        end
        
        function set_tform(this, tformIn)
           assert(isequal(size(tformIn), [3 3]), 'Transformation must be 3x3.');
           this.tform = tformIn;
        end
        
        function str = get.poseString(this)
            str = sprintf('\\theta_x = %.1f, \\theta_y = %.1f, \\theta_z = %.1f, t_x = %.3f, t_y = %.3f, t_z = %.3f', ...
                this.theta_x*180/pi, this.theta_y*180/pi, this.theta_z*180/pi, ...
                this.tx, this.ty, this.tz);
        end
        
        function R = get.rotationMatrix(this)
            cx = cos(this.theta_x);
            cy = cos(this.theta_y);
            cz = cos(this.theta_z);
            
            sx = sin(this.theta_x);
            sy = sin(this.theta_y);
            sz = sin(this.theta_z);
            R = [cy*cz cx*sz+sx*sy*cz sx*sz-cx*sy*cz; ...
                -cy*sz cx*cz-sx*sy*sz sx*cz+cx*sy*sz; ...
                sy -sx*cy cx*cy];
        end
        
        % Set the rotation parameters from a given rotation matrix
        function set.rotationMatrix(this, R)
            if abs(1-R(3,1)) < 1e-6
                this.theta_z = 0;
                if R(3,1) == 1
                    this.theta_y = pi/2;
                    this.theta_x = atan2(R(1,2), R(2,2));
                else
                    this.theta_y = -pi/2;
                    this.theta_x = atan2(-R(1,2), R(2,2));
                end
            else
                this.theta_y = asin(R(3,1));
                cy = cos(this.theta_y);
                this.theta_x = atan2(-R(3,2)/cy, R(3,3)/cy);
                this.theta_z = atan2(-R(2,1)/cy, R(1,1)/cy);
            end
        end
        
        function T = get.translation(this)
            T = [this.tx; this.ty; this.tz];
        end
        
        function set.translation(this, T) 
            this.tx = T(1);
            this.ty = T(2);
            this.tz = T(3);
        end
        
    end % public methods
    
    methods(Access = 'protected')
        
        [converged, numIterations] = trackHelper(this, img, i_scale, translationDone);           
        
        tform = Compute6dofTform(this, R);
        
        plotPose(this);
        
    end % protected methods
    
end % CLASSDEF LucasKanadeTracker
