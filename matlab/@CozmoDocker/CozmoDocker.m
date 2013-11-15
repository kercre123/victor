classdef CozmoDocker < handle

    properties
        h_fig;
        h_axes;
        h_pip;
        h_img;
        h_target;
        
        escapePressed;
        camera;
        calibration;
    end
    
    
    methods
        function this = CozmoDocker(varargin)
            Port = 'COM4';
            Calibration = []; % at 320x240 (QVGA)
            
            parseVarargin(varargin{:});
           
            this.calibration = Calibration;
            this.escapePressed = false;

            this.camera = SerialCamera(Port);
            
            this.h_fig = namedFigure('Docking', 'CurrentCharacter', '~', ...
                'KeyPressFcn', @(src,edata)this.keyPressCallback(src,edata));
            
            this.h_axes = subplot(1,1,1, 'Parent', this.h_fig);
            this.h_pip = axes('Position', [0 0 .2 .2], 'Parent', this.h_fig);
            this.h_img = imagesc(zeros(240,320), 'Parent', this.h_axes);
            axis(this.h_axes, 'image');
            colormap(this.h_fig, gray);
            hold(this.h_axes, 'on');
            this.h_target = plot(nan, nan, 'r', 'LineWidth', 2, 'Parent', this.h_axes);
            hold(this.h_axes, 'off');
        end
        
        function run(this)
            
            this.escapePressed = false;
            dockingDone = false;
            set(this.h_target, 'XData', nan, 'YData', nan);
            
            while ~this.escapePressed && ~dockingDone
                 
                % Put the camera in QVGA mode for detection
                this.camera.changeResolution('QVGA');
                set(this.h_axes, 'XLim', [.5 320.5], 'YLim', [.5 240.5]);
                set(this.h_img, 'CData', zeros(240,320));
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
                    
                    % Initialize the tracker with the full resolution first
                    % image, but tell it we're going to do tracking at
                    % QQQVGA.
                    LKtracker = LucasKanadeTracker(img, marker{1}.corners, ...
                        'Type', 'affine', 'RidgeWeight', 1e-3, ...
                        'DebugDisplay', false, 'UseBlurring', false, ...
                        'UseNormalization', true, 'TrackingResolution', [80 60]);
                    
                    % Show the target we'll be tracking
                    imagesc(LKtracker.target{1}, 'Parent', this.h_pip);
                    axis(this.h_pip, 'image', 'off');
                    
                    this.camera.changeResolution('QQQVGA');
                    %pause(1)
                    % Put the first image, the corners, and the camera in
                    % QQQVGA resolution:
                    %img = imresize(img, [60 80]);
                    %corners = LKtracker.corners;
                    set(this.h_axes, 'XLim', [.5 80.5], 'YLim', [.5 60.5]);
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
                                set(this.h_target, 'XData', corners([1 2 4 3 1],1), ...
                                    'YData', corners([1 2 4 3 1],2));   
                                title(this.h_pip, sprintf('Error = %.2f', LKtracker.err));
                            else
                                lost = lost + 1;
                            end
                            drawnow
                        end
                    end % WHILE not lost
                    
                end % IF escape not pressed
                
            end % while escape not pressed and not done docking
            
        end % FUNCTION run()
       
        
        function keyPressCallback(this, ~,edata)
            if isempty(edata.Modifier)
                switch(edata.Key)
                    case 'escape'
                        this.escapePressed = true;
                end
            end
        end % keyPressCallback()
        
    end % methods

end % classdef CozmoDocker

