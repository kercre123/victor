function planeMotionDemo(varargin)

%CamCaptureArgs = parseVarargin(varargin{:});

imgPrev = [];
tracker = [];
xcen = [];
ycen = [];
h_translation = [];
h_rotation = [];
t_fps = 0;
orientation = 0;

h_fig = namedFigure('Path');
h_pathAxes = subplot(1,2,1, 'Parent', h_fig);
hold(h_pathAxes, 'off')
h_path = plot(0, 0, 'bx-', 'Parent', h_pathAxes);
hold(h_pathAxes, 'on');
plot([0 cos(orientation+pi/2)], [0 sin(orientation+pi/2)], 'g', 'Parent', h_pathAxes);
h_orientAxes = subplot(1,2,2, 'Parent', h_fig);
h_orient = polar([0 pi/2], [0 1], 'r', 'Parent', h_orientAxes);
set(h_orient, 'LineWidth', 4);

grid(h_pathAxes, 'on');

CameraCapture('processFcn', @trackHelper, ...
    'doContinuousProcessing', true, varargin{:});


    function trackHelper(img, h_axes, h_img)
        
        img = mean(im2double(img),3);
        
        if isempty(imgPrev)
            tracker = PlaneMotionTracker('Resolution', [size(img,2) size(img,1)]);
            
            hold(h_axes, 'on');
            xcen = size(img,2)/2 + 1;
            ycen = size(img,1)/2 + 1;
            h_translation = plot(nan, nan, 'r', ...
                'LineWidth', 4, 'Parent', h_axes);
            h_rotation = plot(nan, nan, 'y', 'LineWidth', 4, 'Parent', h_axes);
            
        else
            converged = tracker.track(imgPrev, img);
            
            errImg = abs(tracker.It);
            set(h_img, 'CData', errImg);
            set(h_axes, 'XLim', [0.5 size(errImg,2)+0.5], ...
                'YLim', [0.5 size(errImg,1)+0.5]);
            
            if converged
                
                xpath = get(h_path, 'XData'); 
                ypath = get(h_path, 'YData');
                set(h_path, ...
                    'XData', [xpath xpath(end)+tracker.tx], ...
                    'YData', [ypath ypath(end)+tracker.ty]);
                
                orientation = orientation + tracker.theta;
                %plot(xpath(end)+tx + [0 cos(orientation + pi/2)], ...
                %    ypath(end)+ty  + [0 sin(orientation + pi/2)], ...
                %    'g', 'Parent', h_pathAxes);
                set(h_orient, 'XData', [0 cos(orientation+pi/2)], ...
                    'YData', [0 sin(orientation+pi/2)]);
                
                title(h_axes, sprintf('Tx = %.2f, Ty = %.2f, \\theta = %.2fdeg', ...
                    tracker.tx, tracker.ty, tracker.theta * 180/pi));
                
                xcen = size(errImg,2)/2 + 1;
                ycen = size(errImg,1)/2 + 1;
                
                scale = .05*size(errImg,2);
                set(h_translation, ...
                    'XData', xcen + [0 scale*tracker.tx], ...
                    'YData', ycen + [0 scale*tracker.ty]);
                
                rad = 2.5*scale;
                if tracker.theta > 0
                    th = 0:2*pi/120:(3*tracker.theta);
                else
                    th = (3*tracker.theta):2*pi/120:0;
                end
                xr = rad*cos(th-pi/2) + xcen;
                yr = rad*sin(th-pi/2) + ycen;
                
                set(h_rotation, 'XData', xr, 'YData', yr);
                
                
            else
                title(h_axes, 'Did not converge');
            end
        end
        
        imgPrev = img;
    end


end