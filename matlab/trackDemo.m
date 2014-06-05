function trackDemo(varargin)

TrackerType = 'affine';
Calibration = [];
Downsample = 1;
NumSamples = 0;
NumFiducialSamples = 0;
CalibrationMatrix = [];
MarkerWidth = [];
ConvergenceTolerance = 0.25;
ConvergenceMultiplier = 1;
MaxIterations = 25;
IntensityErrorTolerance = .5;
IntensityErrorFraction = 0.25;
TemplateRegionPaddingFraction = 0.05;
%FilterFcn = @(img, width) repmat(mean(im2double(img),3)./separable_filter(mean(im2double(img),3), ones(1,width)/width), [1 1 3]); %@(img)smoothgradient(img, 0.5)/25; %@(img)abs(imfilter(img, fspecial('log', 5, 2)));
FilterFcn = []; %@(img,width)mean(im2double(img ./ separable_filter(img, ones(1,width)/width)),3);

% Example settings for testing planar6dof_embedded tracker
% calib = load('~/Documents/Anki/Data/CameraCalibrationImages/CameraA1/2014-03-26/Calib_Results.mat');
% K = [calib.fc(1) 0 calib.cc(1); 0 calib.fc(2) calib.cc(2); 0 0 1];
% trackDemo('resolution', [320 240], 'TrackerType', 'planar6dof_embedded', 'CalibrationMatrix', K, 'NumSamples', 500, 'MarkerWidth', 26, 'ConvergenceTolerance', struct('angle', 0.1, 'distance', 0.1));

CamCaptureArgs = parseVarargin(varargin{:});

if strcmp(TrackerType, 'homography') 
    calibration = Calibration;
    
    if isempty(calibration)
        warning(['No calibration data provided with a homography ' ...
            'tracker. Normal will not be estimated.']);
    else
        %camera = Camera('calibration', calibration);
        calibration.fc = calibration.fc / Downsample;
        calibration.cc = calibration.cc / Downsample;
    end
    
elseif strncmp(TrackerType, 'planar6dof', 10)
    assert(~isempty(CalibrationMatrix), 'CalibrationMatrix required.');
    assert(~isempty(MarkerWidth) && MarkerWidth > 0, ...
        'MarkerWidth must be non-empty and greater than zero.');
end
   
LKtracker = [];

h_target = [];
h_samples = [];
h_3d = [];
h_cam = [];
marker3d = [];
corners = [];
cen = [];

resetTracker = false;

H_init = [];

initialSamples = [];

currentParams = [];

h_fig = [];

keypressID = [];

CameraCapture('processFcn', @trackHelper, ...
    'doContinuousProcessing', true, CamCaptureArgs{:});

if ~isempty(keypressID) && ~isempty(h_fig)
    iptremovecallback(h_fig, 'KeyPressFcn', keypressID);
end

if strcmp(TrackerType, 'planar6dof_embedded')
    % This will close any OpenCV windows created by the tracker
    clear mexPlanar6dofTrack
end

    function trackHelper(img, h_axes, h_img)
        
        if isempty(h_fig)
            h_fig = get(h_axes, 'Parent');
        end
        
        if isempty(LKtracker)
            keypressID = iptaddcallback(h_fig, 'KeyPressFcn', @keypressFcn);                
            
            % Detect at full resolution
            marker = simpleDetector(img);
            
            if isempty(marker) || ~marker{1}.isValid
%                 if size(img,3)>1
%                     img = img(:,:,[3 2 1]);
%                 end
                set(h_img, 'CData', img);
                
                return
            end
            
            corners = marker{1}.corners;
            
%             if Downsample ~= 1
%                 img = imresize(img, 1/Downsample, 'nearest');
%                 corners = corners/Downsample;
%             end
            
            order = [1 2 4 3 1];
            [nrows,ncols,~] = size(img);
            %mask = poly2mask(corners(order,1), corners(order,2), nrows, ncols);
            
            %cen = mean(corners,1);
            
            axis(h_axes, 'off')
            set(h_axes, 'XColor', 'r', 'YColor', 'r', 'LineWidth', 2, ...
                'Box', 'on', 'XTick', [], 'YTick', [], ...
                'XLim', [0.5 ncols+.5], 'YLim', [.5 nrows+.5]);
            
            if isempty(FilterFcn)
                imgFiltered = img;
            else
                filterWidth = round((max(corners(:,1)) - min(corners(:,1)))/2);
                imgFiltered = FilterFcn(img, filterWidth, corners);
            end
            
            set(h_img, 'CData', im2uint8(imgFiltered)); %(:,:,[3 2 1]));
            hold(h_axes, 'on');
            
            h_target = plot(corners(order,1), corners(order,2), 'r', ...
                'LineWidth', 2, 'Tag', 'TrackRect', 'Parent', h_axes);
            
            if NumSamples > 0
                h_samples = plot(nan, nan, 'g.', 'MarkerSize', 10, ...
                    'Tag', 'TrackSamples', 'Parent', h_axes);
            end
            
            %LKtracker = LucasKanadeTracker(img, corners, ...
            %    'Type', TrackerType, 'RidgeWeight', 1e-3, ...
            %    'DebugDisplay', false, 'UseBlurring', false, ...
            %    'UseNormalization', true, 'TrackingResolution', Downsample);

            if strcmp(TrackerType, 'planar6dof_embedded')
                try
                    imgFiltered = im2uint8(imgFiltered);
                    if size(imgFiltered,3) > 1
                        imgFiltered = rgb2gray(imgFiltered);
                    end
                    
                    if NumFiducialSamples > 0
                        regionWidth = 1 - (2*VisionMarkerTrained.SquareWidthFraction) - TemplateRegionPaddingFraction;
                    else
                        regionWidth = 1 + TemplateRegionPaddingFraction;
                    end
                    
                    [initialSamples, angleX, angleY, angleZ, tX, tY, tZ] = mexPlanar6dofTrack(imgFiltered, corners, ...
                        CalibrationMatrix(1,1), CalibrationMatrix(2,2), ...
                        CalibrationMatrix(1,3), CalibrationMatrix(2,3), ...
                        MarkerWidth, regionWidth, ...
                        3, NumSamples, 5, NumFiducialSamples, VisionMarkerTrained.SquareWidthFraction);
                    
                    currentParams = [angleX, angleY, angleZ, tX, tY, tZ];
                    
                catch E
                    warning(E.message);
                    return;
                end
                
                if isempty(initialSamples)
                    error('No initialSamples found.');
                else
                    fprintf('%d initial samples.\n', size(initialSamples,1));
                end
                LKtracker = 0;
            else
                LKtracker = LucasKanadeTracker(imgFiltered, corners, ...
                    'Type', TrackerType, 'RidgeWeight', 0, ...
                    'DebugDisplay', false, 'UseBlurring', false, ...
                    'UseNormalization', false, 'NumSamples', NumSamples, ...
                    'TemplateRegionPaddingFraction', TemplateRegionPaddingFraction, ...
                    'MarkerWidth', MarkerWidth, 'CalibrationMatrix', CalibrationMatrix);
            end
            
            if strcmp(TrackerType, 'homography') && ~isempty(calibration)
                % Draw the marker in 3D when we first see it
                
                WIDTH = BlockMarker3D.ReferenceWidth;
                marker3d = WIDTH/2 * [-1 -1 0; -1 1 0; 1 -1 0; 1 1 0];
                
                 % Compute the planar homography:
                 %pts2d = normalize_pixel(pts2d',fc,cc,zeros(1,4),0);
                 K = [calibration.fc(1) 0 calibration.cc(1); 0 calibration.fc(2) calibration.cc(2); 0 0 1];
                 H_init = compute_homography(K\[corners';ones(1,4)], marker3d(:,1:2)');

                 [Rmat, T] = getCameraPose(H_init);
                                  
                 h_fig = namedFigure('Homography Geometry');
                 h_3d  = subplot(1,1,1, 'Parent', h_fig);
                 hold(h_3d, 'off')
                 plot3(marker3d(order,1), marker3d(order,2), marker3d(order,3), 'b', 'Parent', h_3d);
                 hold(h_3d, 'on')
                 h_cam = drawCamera(h_cam, Rmat, T, h_3d);
                                  
                 axis(h_3d, 'equal');
                 grid(h_3d, 'on');
                 
                 set(h_3d, 'XLim', [-50 50], 'YLim', [-50 50], 'ZLim', [-200 10]);
                 
            end
            
            %pause
            
        else
            
            if isempty(img) || any(size(img)==0)
                warning('Empty image passed in!');
                return
            end
            
            if Downsample ~= 1
                img = imresize(img, 1/Downsample, 'bilinear');
                set(h_axes, 'XLim', [.5 size(img,2)+.5], 'YLim', [.5 size(img,1)+.5]);
            end
            
            %t = tic;
            if isempty(FilterFcn)
                imgFiltered = img;
            else
                filterWidth = round((max(corners(:,1)) - min(corners(:,1)))/2);
                imgFiltered = FilterFcn(img, filterWidth, corners);
            end
            if strcmp(TrackerType, 'planar6dof_embedded')
                imgFiltered = im2uint8(imgFiltered);
                if size(imgFiltered,3) > 1
                    imgFiltered = rgb2gray(imgFiltered);
                end
               
                % Compute the convergence tolerance for the current distance
                minAngleTol = ConvergenceTolerance.angle;
                maxAngleTol = ConvergenceMultiplier*ConvergenceTolerance.angle;
                minDistTol  = ConvergenceTolerance.distance;
                maxDistTol  = ConvergenceMultiplier*ConvergenceTolerance.distance;
                
                minDist  = 25;
                maxDist  = 100;
                
                tZ = currentParams(end);
                angleTol = min(maxAngleTol, max(minAngleTol, ...
                    maxAngleTol + (minAngleTol-maxAngleTol)/(maxDist-minDist) * (tZ - minDist)));
                distTol = min(maxDistTol, max(minDistTol, ...
                    maxDistTol + (minDistTol-maxDistTol)/(maxDist-minDist) * (tZ - minDist)));
                
                initialParams = currentParams;
                
                [converged, verify_meanAbsDiff, verify_numInBounds, verify_numSimilarPixels, ...
                    corners, angleX, angleY, angleZ, tX, tY, tZ, ...
                    tform] = ...
                    mexPlanar6dofTrack(imgFiltered, ...
                    MaxIterations, angleTol, distTol, ...
                    IntensityErrorTolerance);
                
                currentParams = [angleX angleY angleZ tX tY tZ];
                
                if ~converged
                    reason = 'Did not converge.';
%                 elseif verify_numSimilarPixels / verify_numInBounds < IntensityErrorFraction
%                     reason = 'Too few pixels below IntensityErrorThreshold.';
%                     converged = false;
                elseif verify_meanAbsDiff > IntensityErrorTolerance
                    reason = 'Mean absolute difference above IntensityErrorThreshold.';
                    converged = false;
                else 
                    reason = '';
                end
                    
                %poseString = num2str([180/pi*[angleX, angleY, angleZ], tX, tY, tZ]);
                poseString = sprintf('[\\theta_X: %.2f\\circ \\theta_Y: %.2f\\circ \\theta_Z: %.2f\\circ], [x: %.2fmm  y: %.2fmm  z: %.2fmm]', ...
                    180/pi*[angleX, angleY, angleZ], tX, tY, tZ);
                
                temp = tform * [initialSamples ones(size(initialSamples,1),1)]';
                xsamples = temp(1,:)./temp(3,:) + CalibrationMatrix(1,3);
                ysamples = temp(2,:)./temp(3,:) + CalibrationMatrix(2,3);
                
                if any(abs(currentParams(1:3)-initialParams(1:3)) > 30*pi/180)
                    trackerSucceeded = false;
                    reason = 'Angle changed too much.';
                elseif any(abs(currentParams(4:6)-initialParams(4:6)) > 20)
                    trackerSucceeded = false;
                    reason = 'Position changed too much.';
                elseif verify_numSimilarPixels / verify_numInBounds < IntensityErrorFraction
                    trackerSucceeded = false;
                    reason = 'Too many verification pixels above error tolerance.';
                else
                    trackerSucceeded = true;
                end
                
            else
                [converged, reason] = LKtracker.track(imgFiltered, ...
                    'MaxIterations', MaxIterations, ...
                    'ConvergenceTolerance', ConvergenceTolerance, ...
                    'IntensityErrorTolerance', IntensityErrorTolerance, ...
                    'IntensityErrorFraction', IntensityErrorFraction);
                
                trackerSucceeded = converged;
                
                corners = LKtracker.corners;
                
                if strcmp(TrackerType, 'planar6dof')
                    poseString = LKtracker.poseString;
                end
                
                if NumSamples > 0
                    [xsamples,ysamples] = LKtracker.getImagePoints(1);
                end
                
            end
               
            if ~trackerSucceeded
                resetTracker = true;
            end
            %fprintf('Tracking took %.2f seconds.\n', toc(t));
            
            if resetTracker
                
                fprintf('Lost Track! (%s)\n', reason)
                LKtracker = [];
                h_cam = [];
                delete(h_target)
                if NumSamples > 0
                    delete(h_samples)
                end
                
                resetTracker = false;
                
                return
            end
            
%             if size(img,3)>1
%                 img = img(:,:,[3 2 1]);
%             end
            set(h_img, 'CData',im2uint8(imgFiltered));
            
            if strncmp(TrackerType, 'planar6dof', 10)
                title(h_axes, sprintf('          Block Pose: %s', poseString), 'FontSize', 14);
            else
                title(h_axes, sprintf('Error: %.3f', LKtracker.err));
            end
%             if LKtracker.err < 0.3
%                 set(h_axes, 'XColor', 'g', 'YColor', 'g');
%             else
%                 set(h_axes, 'XColor', 'r', 'YColor', 'r');
%             end
            %temp = LKtracker.scale*(corners - cen(ones(4,1),:)) + cen(ones(4,1),:) + ...
            %    ones(4,1)*[LKtracker.tx LKtracker.ty];
            %[tempx, tempy] = LKtracker.getImagePoints(corners(:,1), corners(:,2));
            
           
            set(h_target, 'XData', corners([1 2 4 3 1],1), ...
                'YData', corners([1 2 4 3 1],2));
            
            if NumSamples > 0
                %[xsamples,ysamples] = LKtracker.getImagePoints(1);
                set(h_samples, 'XData', xsamples, 'YData', ysamples);
            end
                     
            if strcmp(TrackerType, 'homography') && ~isempty(calibration)

                K = [calibration.fc(1) 0 calibration.cc(1); 
                    0 calibration.fc(2) calibration.cc(2);
                    0 0 1];
                
                H = K \ (LKtracker.tform * (K  * H_init));
                
                [Rmat, T] = getCameraPose(H);
                 
                h_cam = drawCamera(h_cam, Rmat, T, h_3d);
                
                %{
                [Rvec,T] = compute_extrinsic_init(marker2d', marker3d', ...
                    calibration.fc, calibration.cc, zeros(1,4), 0);
   
                R = rodrigues(Rvec);
%                 R = R';
%                 T = -R*T;
                
                normal = [0 0 0; 0 0 -WIDTH/2];
                normal = (R*normal' + T(:,ones(1,2)))';
                
                marker = (R*marker3d' + T(:,ones(1,4)))';
                
                h_normal = findobj(0, 'Tag', 'normal');
                if isempty(h_normal)
                    h_fig = namedFigure('Homography Geometry');
                    h_normalAxes = subplot(1,1,1, 'Parent', h_fig);
                    hold(h_normalAxes, 'off');
                    plot3(normal(:,1), normal(:,2), normal(:,3), 'r', ...
                        'LineWidth', 2, 'Parent', h_normalAxes, 'Tag', 'normal');
                    hold(h_normalAxes, 'on');
                    plot3(marker([1 2 4 3 1],1), marker([1 2 4 3 1],2), marker([1 2 4 3 1],3), 'b', 'Tag', 'marker', 'Parent', h_normalAxes);
                    axis(h_normalAxes , 'equal');
                    grid(h_normalAxes , 'on')
                    %zoom(h_normalAxes, .5);
                    xlim = get(h_normalAxes, 'XLim'); xcen = mean(xlim);
                    ylim = get(h_normalAxes, 'YLim'); ycen = mean(ylim);
                    zlim = get(h_normalAxes, 'ZLim'); zcen = mean(zlim);
                    set(h_normalAxes, 'XLim', [0.3 3].*(xlim-xcen)+xcen, 'YLim', [0.3 3].*(ylim-ycen)+ycen, 'ZLim', [0.3 3].*(zlim-zcen)+zcen);
                        
                else
                   set(h_normal, 'XData', normal(:,1), 'YData', normal(:,2), 'ZData', normal(:,3));
                   set(findobj(0, 'Tag', 'marker'), 'XData', marker([1 2 4 3 1],1), ...
                       'YData', marker([1 2 4 3 1],2), 'ZData', marker([1 2 4 3 1],3));
                end
                    %}
                    
   
                %{
                G = LKtracker.tform;
                
                % [Malis & Vargas] page 7
                K = [calibration.fc(1) 0 calibration.cc(1);
                    0 calibration.fc(2) calibration.cc(2);
                    0 0 1];
                Hhat = (K\G)*K;
                gamma = getHomographyScale(Hhat); % Appendix B
                H = Hhat/gamma;
                
                % [Malis & Vargas] page 16
                S = H'*H - eye(3);
                
                [~,whichMinor] = max(diag(S));
                
                M11 = sqrt(getOppMinor(S,1,1));
                M22 = sqrt(getOppMinor(S,2,2));
                M33 = sqrt(getOppMinor(S,3,3));
                
                switch(whichMinor)
                    case 1
                        e23 = epsSign(S,2,3);
                        na = [S(1,1); S(1,2)+M33; S(1,3) + e23*M22];
                        nb = [S(1,1); S(1,2)-M33; S(1,3) - e23*M22];
                        
                    case 2
                        e13 = epsSign(S,1,3);
                        na = [S(1,2) + M33; S(2,2); S(2,3)-e13*M11];
                        nb = [S(1,2) - M33; S(2,2); S(2,3)+e13*M11];
                        
                    case 3
                        e12 = epsSign(S,1,2);
                        na = [S(1,3) + e12*M22; S(2,3) + M11; S(3,3)];
                        nb = [S(1,3) - e12*M22; S(2,3) - M11; S(3,3)];
                end
                       
                na = na/norm(na);
                nb = nb/norm(nb);
                
                nu = 2*sqrt(1 + trace(S) - M11 - M22 - M33);
                mag_te = sqrt(2 + trace(S) - nu);
                rho = sqrt(2 + trace(S) + nu);
                
                ta = mag_te/2 * (posSign(S(whichMinor,whichMinor))*rho*nb - mag_te*na);
                tb = mag_te/2 * (posSign(S(whichMinor,whichMinor))*rho*na - mag_te*nb);
                        
                Ra = H*(eye(3) - 2/nu*ta*na');
                Rb = H*(eye(3) - 2/nu*tb*nb');
                
                ta = Ra*ta;
                tb = Rb*tb;
                
                h_ta = findobj(0, 'Tag', 'ta');
                L = 1;
                if isempty(h_ta)
                    h_fig = namedFigure('Homography Geometry');
                    h_axes = subplot(1,1,1, 'Parent', h_fig);
                    hold(h_axes, 'off');
                    plot3(ta(1), ta(2), ta(3), 'ro', 'Tag', 'ta', 'Parent', h_axes);
                    hold(h_axes, 'on');
                    plot3(tb(1), tb(2), tb(3), 'bo', 'Tag', 'tb', 'Parent', h_axes);
                    plot3(ta(1)+[0 -L*na(1)], ta(2)+[0 -L*na(2)], ta(3)+[0 -L*na(3)], 'r', 'Parent', h_axes, 'Tag', 'na');
                    plot3(tb(1)+[0 -L*nb(1)], tb(2)+[0 -L*nb(2)], tb(3)+[0 -L*nb(3)], 'b', 'Parent', h_axes, 'Tag', 'nb');
                    axis(h_axes, 'equal');
                    grid(h_axes, 'on')
                else
                    set(h_ta, 'XData', ta(1), 'YData', ta(2), 'ZData', ta(3));
                    set(findobj(0, 'Tag', 'tb'), 'XData', tb(1), 'YData', tb(2), 'ZData', tb(3));
                    set(findobj(0, 'Tag', 'na'), 'XData', ta(1)+[0 -L*na(1)], 'YData', ta(2)+[0 -L*na(2)], 'ZData', ta(3)+[0 -L*na(3)]);
                    set(findobj(0, 'Tag', 'nb'), 'XData', tb(1)+[0 -L*nb(1)], 'YData', tb(2)+[0 -L*nb(2)], 'ZData', tb(3)+[0 -L*nb(3)]);
                end
                %}
                
%                 if get(gcbf, 'CurrentChar') == 'n'
%                     keyboard
%                 end
                

% 
%                 [nrows,ncols,~] = size(img);
%                 delete(findobj(h_axes, 'Tag', 'normal'));
%                 L = 10;
%                 P = [0 0 0; -L*na'; -L*nb'];
%                 P(:,3) = P(:,3) + 50;
%                 pts = camera.projectPoints(P);
%                 %plot3(ncols/2+[0 L*na(1)], nrows/2+[0 L*na(2)], ...
%                 %    [0 L*na(3)], 'r', 'LineWidth', 2, 'Tag', 'normal');
%                 %plot3(ncols/2+[0 L*nb(1)], nrows/2+[0 L*nb(2)], ...
%                 %    [0 L*nb(3)], 'b', 'LineWidth', 2, 'Tag', 'normal');
%                 plot(pts([1 2],1), pts([1 2], 2), 'r', 'LineWidth', 2, ...
%                     'Parent', h_axes, 'Tag', 'normal');
%                 plot(pts([1 3],1), pts([1 3], 2), 'b', 'LineWidth', 2, ...
%                     'Parent', h_axes, 'Tag', 'normal');
            end
                
%             
%             h_track = findobj(h_axes, 'Tag', 'TrackRect');
%             set(h_track, 'Pos', ...
%                 [LKtracker.tx + targetXcen - LKtracker.scale*targetWidth/2 ...
%                  LKtracker.ty + targetYcen - LKtracker.scale*targetWidth/2 ...
%                  LKtracker.scale*targetWidth*[1 1]]);
             
%             if ~isempty(h_track)
%                                %set(h_track, 'XData', LKtracker.tx, 'YData', LKtracker.ty);
%             else
%                 %plot(LKtracker.tx, LKtracker.ty, 'rx', ...
%                 rectangle('Pos', targetRect + [LKtracker.tx LKtracker.ty 0 0], ...
%                     'EdgeColor', 'r', 'LineWidth', 3, ...
%                     'Parent', h_axes, 'Tag', 'TrackRect');
%             end
            
        end

    end % trackHelper 

        
    function keypressFcn(~, edata)
        if isempty(edata.Modifier)
            switch(edata.Key)
                case 'r'
                    resetTracker = true;
                    
            end
        end
    end

end % trackDemo

%function [Rmat, T] = getCameraPose(pts2d, pts3d, fc, cc)
function [Rmat, T] = getCameraPose(H)

% De-embed the initial 3D pose from the homography:
scale = mean([norm(H(:,1));norm(H(:,2))]);
H = H/scale;

u1 = H(:,1);
u1 = u1 / norm(u1);
u2 = H(:,2) - dot(u1,H(:,2)) * u1;
u2 = u2 / norm(u2);
u3 = cross(u1,u2);
Rmat = [u1 u2 u3];
%Rvec = rodrigues(Rmat);
T    = H(:,3);

Rmat = Rmat';
T    = -Rmat*T;

end % getCameraPose()

function h_cam = drawCamera(h_cam, R, t, h_axes)

scale = 5;
verts = scale*[-1 -1 0; -1 1 0; 1 1 0; 1 -1 0; 0 0 -.5];
verts = (R*verts' + t(:,ones(1,5)))';
order = [1 2; 2 3; 3 4; 4 1; 1 5; 2 5; 3 5; 4 5]';
if isempty(h_cam)
    h_cam = plot3(verts(order,1), verts(order,2), verts(order,3), 'Parent', h_axes);
else
    set(h_cam, 'XData', verts(order,1), 'YData', verts(order,2), 'ZData', verts(order,3));
end

end

function gamma = getHomographyScale(Hhat)
% [Malis & Vargas], Appendix B
M = Hhat'*Hhat;
a0 = M(1,2)^2*M(3,3) + M(1,3)^2*M(2,2) + M(2,3)^2*M(1,1) - ...
    M(1,1)*M(2,2)*M(3,3) - 2*M(1,2)*M(1,3)*M(2,3);
a1 = M(1,1)*M(2,2) + M(1,1)*M(3,3) + M(2,2)*M(3,3) - ...
    (M(1,2)^2  + M(1,3)^2 + M(2,3)^2);
a2 = -trace(M);

Q = (3*a1-a2^2)/9;
R = (9*a2*a1-27*a0-2*a2^3)/54;

temp = sqrt(Q^3 + R^2);
S = (R+temp)^(1/3);
T = (R-temp)^(1/3);

lambda2 = -a2/3 - .5*(S+T) - sqrt(3)/2*(S-T)*1i;

gamma = sqrt(lambda2);

% Sanity check
assert(abs(gamma - median(svd(Hhat))) < 1e-6);

end




function M = getOppMinor(S, i, j)

S(i,:) = [];
S(:,j) = [];
M = -det(S);

end % getMinor

function s = epsSign(S,i,j)

M = getOppMinor(S,i,j);
s = posSign(M);

end

function s = posSign(x)
if x >= 0
    s = 1;
else
    s = -1;
end
end



