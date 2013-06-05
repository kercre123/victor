function computeBlockPose(detections, calibration, varargin)

markerSize = 35;
maxRefineIterations = 100;
thresh_cond = 1000000;

parseVarargin(varargin{:});

img = [];
if ~iscell(detections)
    img = detections;
    detections = simpleDetector(img);
    fprintf('Found %d blocks.\n', length(detections))
end
  

numDetections = length(detections);
if numDetections == 0 
    return
end

namedFigure('3D Reconstruction');
if ~isempty(img)
    h_axes(1) = subplot(221); hold off, imagesc(img), axis image off
    hold on
    for i = 1:numDetections
        detections{i}.draw(gca, 'drawTextLabels', false);
    end
    h_axes(2) = subplot(222);
    hold off, imagesc(img), axis image off, hold on
    
    linkaxes(h_axes);
    
    subplot 212
end
hold off
plot3(0,0,0, 'k*', 'MarkerSize', 20);
hold on
grid on

target = zeros(numDetections,3);
for i_det = 1:numDetections
    p_marker = detections{i_det}.corners;% + .25*randn(4,2);
    P_marker = markerSize*[detections{i_det}.marker zeros(4,1)];
       
    %     % Refine corners?
    %     p_marker = cornerfinder2(p_marker',img, 1, 1)';
    
    % TODO: write my own version?
    [Rvec,T] = compute_extrinsic_init(p_marker', P_marker', ...
        calibration.fc, calibration.cc, calibration.kc, calibration.alpha_c);
        
    if maxRefineIterations > 0
        %Y_kk = (rodrigues(Rvec)*X_kk' + Tckk(:,ones(1,size(X_kk,1))))';
        %plot3(Y_kk([1 3 4 2 1],1), Y_kk([1 3 4 2 1], 3), -Y_kk([1 3 4 2 1], 2), 'r--');
    
        % TODO: write my own version?
        [~,T,Rmat] = compute_extrinsic_refine(Rvec, T, ...
            p_marker', P_marker', calibration.fc, calibration.cc, calibration.kc, ...
            calibration.alpha_c, maxRefineIterations);
    else 
        Rmat = rodrigues(Rvec);
    end
    
    % Figure out where the marker is in 3D space:
    P_marker = (Rmat*P_marker' + T(:,ones(1,size(P_marker,1))))';
    
    % Get the 3D model for this marker:
    [X,Y,Z,color] = getModel(detections{i_det}.blockType, ...
        detections{i_det}.faceType, detections{i_det}.topOrient);
    P_model = [X(:) Y(:) Z(:)];
    
    % Rotate the model and line it up with the marker's origin:
    P_model = (Rmat*P_model' + P_marker(ones(size(P_model,1),1),:)')'; %T(:,ones(1,size(P_model,1))))';
    X = reshape(P_model(:,1), 4, []);
    Y = reshape(P_model(:,2), 4, []);
    Z = reshape(P_model(:,3), 4, []);
        
    subplot 212
    patch(P_marker([1 3 4 2],1), P_marker([1 3 4 2], 2), P_marker([1 3 4 2], 3), 'w', 'EdgeColor', 'w');
    patch(X,Y,Z, color);
    plot3(P_marker(1,1), P_marker(1,2), P_marker(1,3), 'k.', 'MarkerSize', 24);
    
    target(i_det,:) = mean(P_model,1);
    if any(isinf(target(i_det,:)))
        disp('Target contains inf values.');
        keyboard
    end
    
    % Reproject the model and the marker onto the image:
    p_model = projectPoints(P_model, calibration);
    p_marker2 = projectPoints(P_marker, calibration);
        
    subplot 222
    x = reshape(p_model(:,1), 4, []);
    y = reshape(p_model(:,2), 4, []);
    z = zeros(size(x));
    patch(x,y,z, color, 'FaceAlpha', 0.3, 'LineWidth', 3);
    plot(p_marker2([1 3 4 2 1],1), p_marker2([1 3 4 2 1],2), 'w', 'LineWidth', 2)
    
end

subplot 212
% xlabel('X Axis')
% ylabel('Y Axis')
% zlabel('Z Axis')
axis equal

camup([0 -1 0]);
campos([0 0 0]);
camtarget(mean(target,1))

%camva(2*atan(size(img,2) / (2*calibration.fc(1))))

camorbit(-45, 20, 'camera', 'y');

%keyboard
