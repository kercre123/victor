function computeBlockPose(detections, calibration)

img = [];
if ~iscell(detections)
    img = detections;
    detections = simpleDetector(img);
    fprintf('Found %d blocks.\n', length(detections))
end
   
markerSize = 35;
doRefine = true;
thresh_cond = 1000000;

namedFigure('3D Reconstruction');
if ~isempty(img)
    subplot 221, hold off, imagesc(img), axis image off
    hold on
    for i = 1:length(detections)
        detections{i}.draw(gca, 'drawTextLabels', false);
    end
    subplot 222
    hold off, imagesc(img), axis image off, hold on
    
    subplot 212
end
hold off
plot3(0,0,0, 'k*', 'MarkerSize', 20);
hold on
grid on

target = zeros(length(detections),3);
for i_det = 1:length(detections)
    p_marker = detections{i_det}.corners;
    P_marker = markerSize*[detections{i_det}.marker zeros(4,1)];
       
    [Rvec,T] = compute_extrinsic_init(p_marker', P_marker', ...
        calibration.fc, calibration.cc, calibration.kc, calibration.alpha_c);
        
    if doRefine
        %Y_kk = (rodrigues(Rvec)*X_kk' + Tckk(:,ones(1,size(X_kk,1))))';
        %plot3(Y_kk([1 3 4 2 1],1), Y_kk([1 3 4 2 1], 3), -Y_kk([1 3 4 2 1], 2), 'r--');
    
        [Rvec,T,Rmat] = compute_extrinsic_refine(Rvec, T, ...
            p_marker', P_marker', calibration.fc, calibration.cc, calibration.kc, ...
            calibration.alpha_c, 100);
    end
    
    P_marker = (Rmat*P_marker' + T(:,ones(1,size(P_marker,1))))';
    
    [X,Y,Z,color] = getModel(detections{i_det}.blockType, detections{i_det}.faceType);
        
    P_model = [X(:) Y(:) Z(:)];
    
%     switch(detections{i_det}.topOrient)
%         case 'left'
%             Rtop = eye(3);
%         case 'down'
%             Rtop = rodrigues(-pi/2*[0 0 1]);
%         case 'right'
%             Rtop = rodrigues(pi*[0 0 1]);
%         case 'up'
%             Rtop = rodrigues(pi/2*[0 0 1]);
%     end
%     P_model = (Rtop*P_model')';
    
    p_model = project_points2(P_model', Rvec, T, ...
        calibration.fc, calibration.cc, calibration.kc, ...
        calibration.alpha_c)';    
    
    P_model = (Rmat*P_model' + T(:,ones(1,size(P_model,1))))';
    X = reshape(P_model(:,1), 4, []);
    Y = reshape(P_model(:,2), 4, []);
    Z = reshape(P_model(:,3), 4, []);
    
    subplot 212
    patch(P_marker([1 3 4 2],1), P_marker([1 3 4 2], 2), P_marker([1 3 4 2], 3), 'w', 'EdgeColor', 'w');
    patch(X,Y,Z, color);
    
    target(i_det,:) = mean(P_model,1);
    %keyboard
    
    subplot 222
    x = reshape(p_model(:,1), 4, []);
    y = reshape(p_model(:,2), 4, []);
    z = zeros(size(x));
    %plot(p_model(:,1), p_model(:,2), 'gx');
    patch(x,y,z, color, 'FaceAlpha', 0.3, 'LineWidth', 3);
    
    
end

subplot 212
% xlabel('X Axis')
% ylabel('Y Axis')
% zlabel('Z Axis')
axis equal

camup([0 -1 0]);
campos([0 0 0]);
camtarget(mean(target,1))

camorbit(-60, 20, 'camera', 'y');

%keyboard
