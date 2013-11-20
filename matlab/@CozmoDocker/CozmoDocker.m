classdef CozmoDocker < handle

    properties
        h_fig;
        h_axes;
        h_pip;
        h_angleError;
        h_distError;
        h_leftRightError;
        h_img;
        h_target;
        
        escapePressed;
        camera;
        calibration;
        
        detectionResolution;
        trackingResolution;
        
        trackerType;
        
        dockingDistance; % in mm, from camera
        
        % For homography-based pose estimation
        marker3d;
        H_init;
        K; % calibration matrix
        h_cam;
        h_poseAxes;
        
    end
    
    
    methods
        function this = CozmoDocker(varargin)
            
            Port = 'COM4';
            BaudRate = 1500000;
            DetectionResolution = 'QVGA';
            TrackingResolution  = 'QQQVGA';
            Calibration = []; % at Tracking resolution!
            TrackerType = 'affine';
            DockDistanceMM = 30; % distance to top bar. this depends on head angle!
                        
            parseVarargin(varargin{:});
           
            this.calibration         = Calibration;
            this.detectionResolution = DetectionResolution;
            this.trackingResolution  = TrackingResolution;
            this.trackerType         = TrackerType;
            
            this.camera              = SerialCamera(Port, 'BaudRate', BaudRate);
            this.escapePressed       = false;
            
            this.dockingDistance = DockDistanceMM;
                        
            % Set up display figure/axes:
            this.h_fig = namedFigure('Docking', 'CurrentCharacter', '~', ...
                'KeyPressFcn', @(src,edata)this.keyPressCallback(src,edata));
            
            this.h_axes = subplot(1,1,1, 'Parent', this.h_fig);
            this.h_pip = axes('Position', [0 .75 .2 .2], 'Parent', this.h_fig);
            title(this.h_pip, 'Target');
            
            % Angle Error Plot
            ERRORBAR_LINEWIDTH = 25;
            h = axes('Position', [0 0 .2 .2], 'Parent', this.h_fig);
            title(h, 'Angle Error')
            this.h_angleError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'YTick', [], 'XTick', -45:5:45, ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', pi/4*[-1 1], 'YLim', .25*[-1 1], 'XGrid', 'on');
            
            % Distance Error Plot
            h = axes('Position', [.8 0 .2 .2], 'Parent', this.h_fig);
            title(h, 'Distance');
            this.h_distError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'XTick', [], 'YTick', 0:20:150, ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', .25*[-1 1], 'YLim', [0 150], 'YGrid', 'on');
            
            % Horizontal Error Plot
            h = axes('Position', [.8 .75 .2 .2], 'Parent', this.h_fig);
            title(h, 'Horizontal Error');
            %this.h_leftRightError = barh(0, 'Parent', h);
            this.h_leftRightError = plot([0 0], [0 0], 'r', 'Parent', h, 'LineWidth', ERRORBAR_LINEWIDTH);
            set(h, 'XTick', -20:5:20, 'YTick', [], ...
                'XTickLabel', [], 'YTickLabel', [], ...
                'XLim', [-20 20], 'YLim', .25*[-1 1], 'XGrid', 'on');
           
            this.h_img = imagesc(zeros(240,320), 'Parent', this.h_axes);
            axis(this.h_axes, 'image');
            colormap(this.h_fig, gray);
            hold(this.h_axes, 'on');
            this.h_target = plot(nan, nan, 'r', 'LineWidth', 2, 'Parent', this.h_axes);
            hold(this.h_axes, 'off');
            
            if strcmp(this.trackerType, 'homography')
                % For convenience, create a 3x3 camera calibration matrix
                if ~isempty(this.calibration)
                this.K = [this.calibration.fc(1) 0 this.calibration.cc(1); ...
                    0 this.calibration.fc(2) this.calibration.cc(2); ...
                    0 0 1];
                end
                        
                WIDTH = BlockMarker3D.ReferenceWidth;
                this.marker3d = WIDTH/2 * [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
                
                h_poseFig = namedFigure('Docking Pose', 'KeyPressFcn', ...
                    @(src,edata)this.keyPressCallback(src,edata));
                this.h_poseAxes = subplot(1,1,1, 'Parent', h_poseFig);
                
                hold(this.h_poseAxes, 'off')
                order = [1 2 4 3 1];
                plot3(this.marker3d(order,1), this.marker3d(order,2), ...
                    this.marker3d(order,3), 'b', 'Parent', this.h_poseAxes);
                hold(this.h_poseAxes, 'on')
                block3d = 30*[-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
                plot3(block3d(order,1), block3d(order,2), block3d(order,3), ...
                    'g', 'Parent', this.h_poseAxes);
                
                axis(this.h_poseAxes, 'equal');
                grid(this.h_poseAxes, 'on');
                
                set(this.h_poseAxes, 'XLim', [-50 50], ...
                    'YLim', [-50 50], 'ZLim', [-200 10], ...
                    'View', [0 0]);
                
                xlabel(this.h_poseAxes, 'X Error')
                ylabel(this.h_poseAxes, 'Y Error')
                zlabel(this.h_poseAxes, 'Z Error')
                
            end
            
        end % CONSTRUCTOR CozmoDocker()
        
        function run(this)
            
            this.escapePressed = false;
            dockingDone = false;
            
            while ~this.escapePressed && ~dockingDone
                 
                % Set the camera's resolution for detection.
                this.camera.changeResolution(this.detectionResolution);
                dims = this.camera.framesize;
                set(this.h_axes, 'XLim', [.5 dims(1)+.5], 'YLim', [.5 dims(2)+.5]);
                set(this.h_img, 'CData', zeros(dims(2),dims(1)));
                set(this.h_target, 'XData', nan, 'YData', nan);
                drawnow
                
                % Wait until we get a valid marker\
                % TODO: wait until we get the marker for the block we want
                % to pick up!
                marker = [];
                while ~this.escapePressed && (isempty(marker) || ~marker{1}.isValid)
                    %pause(1/10);
                    img = this.camera.getFrame();
                    if ~isempty(img)
                        set(this.h_img, 'CData', img); drawnow;
                        marker = simpleDetector(img); 
                    end
                end
                
                if ~this.escapePressed
                    this.camera.changeResolution(this.trackingResolution);
                    
                    % Initialize the tracker with the full resolution first
                    % image, but tell it we're going to do tracking at
                    % QQQVGA.
                    LKtracker = LucasKanadeTracker(img, marker{1}.unorderedCorners([3 1 4 2],:), ...
                        'Type', this.trackerType, 'RidgeWeight', 1e-3, ...
                        'DebugDisplay', false, 'UseBlurring', false, ...
                        'UseNormalization', true, ...
                        'TrackingResolution', this.camera.framesize);
                    
                    if strcmp(this.trackerType, 'homography') && ...
                            ~isempty(this.calibration)
                        
                        % Compute the planar homography:
                        %{
                        Kfull = this.K * 4;
                        this.H_init = compute_homography( ...
                            Kfull\[marker{1}.corners';ones(1,4)], ...
                            this.marker3d(:,1:2)');
                        this.H_init = .25 * this.H_init;
                        %}
                        
                        this.H_init = compute_homography( ...
                            this.K\[LKtracker.corners';ones(1,4)], ...
                            this.marker3d(:,1:2)');
                        
                        [Rmat, T] = this.getCameraPose(this.H_init);
                        
                        %[Rmat, T] = this.getCameraPose(LKtracker.corners, this.marker3d);
                         
                        this.drawCamera(Rmat, T);
                        
                    end
                    
                    % Show the target we'll be tracking
                    imagesc(LKtracker.target{LKtracker.finestScale}, 'Parent', this.h_pip);
                    axis(this.h_pip, 'image', 'off');
                    
                    set(this.h_axes, ...
                        'XLim', [.5 this.camera.framesize(1)+.5], ...
                        'YLim', [.5 this.camera.framesize(2)+.5]);
                    %set(this.h_img, 'CData', imresize(img, [60 80], 'nearest'));
                    %set(this.h_target, 'XData', corners([1 2 4 3 1],1), ...
                    %    'YData', corners([1 2 4 3 1],2));
                    %drawnow;
                    
                    lost = 0;
                    numEmpty = 0;
                    while lost < 5 && ~this.escapePressed
                        %pause(1/30);
                        img = this.camera.getFrame();
                        if isempty(img)
                            numEmpty = numEmpty + 1;
                            
                            if numEmpty > 5
                                % Too many empty frames in a row
                                warning('Too many empty frames received. Resetting camera.');
                                this.camera.reset();
                            end
                        else
                            numEmpty = 0;
                            set(this.h_img, 'CData', img); 
                            converged = LKtracker.track(img, ...
                                'MaxIterations', 50, ...
                                'ConvergenceTolerance', .25);
                            
                            if converged
                                lost = 0;
                                corners = LKtracker.corners;
                                
                                if ~isempty(this.calibration)
                                    this.sendError(LKtracker);
                                end
                                
                                set(this.h_target, ...
                                    'XData', corners([1 2 4 3 1],1), ...
                                    'YData', corners([1 2 4 3 1],2));   
                                title(this.h_pip, sprintf('TargetError = %.2f', LKtracker.err), 'Back', 'w');
                            else
                                lost = lost + 1;
                            end
                            drawnow
                        end
                    end % WHILE not lost
                    
                end % IF escape not pressed
                
            end % while escape not pressed and not done docking
            
        end % FUNCTION run()
       
        
        function sendError(this, LKtracker)
            
            % Compute the error signal according to the current tracking result
            switch(this.trackerType)
                case 'affine'
                    corners = LKtracker.corners;
                    
                    % TODO: why do i still need this rotation fix code? I'm
                    % using the unordered marker corners above and i
                    % thought that would fix it...
                    %
                    % Block may be rotated with top side of marker not 
                    % facing up, so reorient to make sure we got top
                    % corners
                    [th,~] = cart2pol(corners(:,1)-mean(corners(:,1)), corners(:,2)-mean(corners(:,2)));
                    [~,sortIndex] = sort(th);
                    
                    upperLeft = corners(sortIndex(1),:);
                    upperRight = corners(sortIndex(2),:);
                    
                    %upperLeft = corners(1,:);
                    %upperRight = corners(2,:);
                    
                    assert(upperRight(1) > upperLeft(1), ...%if upperRight(1) < upperLeft(1)
                        ['UpperRight corner should be to the right ' ...
                            'of the UpperLeft corner.']);
                        %keyboard
                    %end
                    
                    % Get the angle from vertical of the top bar of the 
                    % marker we're tracking
                    L = sqrt(sum(upperRight-upperLeft).^2);
                    angleError = asin( (upperRight(2)-upperLeft(2)) / L);
                    
                    currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
                                       
                    % TODO: should i be comparing to ncols/2 or calibration center?
                    midPointErr = (upperRight(1)+upperLeft(1))/2 - ...
                        this.trackingResolution(1)/2;
                    midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
                                        
                case 'homography'

                    C = [1 0 -LKtracker.xcen; 0 1 -LKtracker.ycen; 0 0 1]; 
                    H = this.K\(C\LKtracker.tform*C*this.K*this.H_init);
                    
                    % This computes the equivalent H, but is more expensive
                    % since it involves an SVD internally to compute the
                    % homography instead of just chaining together the
                    % initial homography with the current one already found
                    % by the tracker, as above.
                    %H = compute_homography(this.K\[LKtracker.corners';ones(1,4)], ...
                    %        this.marker3d(:,1:2)');
                        
                    [Rmat, T] = this.getCameraPose(H);
                    
                    this.drawCamera(Rmat, T);
                    
                    currentDistance = -T(3);
                    midPointErr = T(1);
                    angleError = 0;
                    
                otherwise
                    error(['Not sure how to compute an error signal ' ...
                        'when using a %s tracker.'], this.trackerType);
            end % SWITCH(trackerType)
            
            distError = currentDistance - this.dockingDistance;
             
            set(this.h_angleError, 'XData', [0 angleError*180/pi]);
            h = get(this.h_angleError, 'Parent');
            %set(h, 'XLim', pi/4*[-1 1], 'YLim', [.25 1.75], 'XGrid', 'on');
            title(h, sprintf('AngleErr = %.1fdeg', angleError*180/pi), 'Back', 'w');
            
            set(this.h_distError, 'YData', [0 distError]);
            h = get(this.h_distError, 'Parent');
            %set(h, 'XLim', [.25 1.75], 'YLim', [0 150], 'YGrid', 'on');
            title(h, sprintf('DistToGo = %.1fmm', distError), 'Back', 'w');
            
            set(this.h_leftRightError, 'XData', [0 midPointErr]);
            h = get(this.h_leftRightError, 'Parent');
            %set(h, 'XLim', [-20 20], 'YLim', [.25 1.75], 'XGrid', 'on');
            title(h, sprintf('LeftRightErr = %.1fmm', midPointErr), 'Back', 'w');
            
            % TODO: send the error signals back over the serial connection
            
        end % FUNCTION sendError()
        
        function keyPressCallback(this, ~,edata)
            if isempty(edata.Modifier)
                switch(edata.Key)
                    case 'space'
                        this.run();
                        
                    case 'escape'
                        this.escapePressed = true;
                end
            end
        end % keyPressCallback()
        
        function drawCamera(this, R, t)
            
            scale = 5;
            verts = scale*[-1 -1 0; -1 1 0; 1 1 0; 1 -1 0; 0 0 -.5];
            verts = (R*verts' + t(:,ones(1,5)))';
            order = [1 2; 2 3; 3 4; 4 1; 1 5; 2 5; 3 5; 4 5]';
            if isempty(this.h_cam)
                this.h_cam = plot3(verts(order,1), verts(order,2), verts(order,3), 'Parent', this.h_poseAxes);
            else
                set(this.h_cam, 'XData', verts(order,1), 'YData', verts(order,2), 'ZData', verts(order,3));
            end
            
        end % FUNCTION drawCamera()

    end % methods
    
    methods
        function [Rmat, T] = getCameraPose(this, varargin)
            
            if nargin == 2
                H = varargin{1};
                
                % De-embed the initial 3D pose from the homography:
                scale = mean([norm(H(:,1));norm(H(:,2))]);
                %scale = H(3,3);
                H = H/scale;
                
                u1 = H(:,1);
                u1 = u1 / norm(u1);
                u2 = H(:,2) - dot(u1,H(:,2)) * u1;
                u2 = u2 / norm(u2);
                u3 = cross(u1,u2);
                Rmat = [u1 u2 u3];
                %Rvec = rodrigues(Rmat);
                T    = H(:,3);
                
            else
                pts2d = varargin{1};
                pts3d = varargin{2};
                
                [~,T,Rmat] = compute_extrinsic_init(pts2d', pts3d', ...
                    this.calibration.fc, this.calibration.cc, zeros(4,1), 0);
            end
            
            Rmat = Rmat';
            T    = -Rmat*T;
            
        end % getCameraPose()
    end

end % classdef CozmoDocker

