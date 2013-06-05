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
    
        [~,T,Rmat] = compute_extrinsic_refine(Rvec, T, ...
            p_marker', P_marker', calibration.fc, calibration.cc, calibration.kc, ...
            calibration.alpha_c, 100);
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
    
    % Project the 3D model back into the image:
    % (This follows the projection model in Bouguet's camera calibration
    % toolbox.)
    %  1. Pinhole projection to (a,b) on the plane, where a=x/z and b=y/z.
    %  2. Radial distortion.  Let r^2 = a^2 + b^2.  Then the distorted 
    %     point coordinates (ud,vd) are: 
    %
    %     ud = a * (1 + kc(1)*r^2 + kc(2)*r^4) + 2*kc(3)*a*b + kc(4)*(r^2 + 2*a^2);
    %     vd = b * (1 + kc(1)*r^2 + kc(2)*r^4) + kc(3)*(r^2 + 2*b^2) + 2*kc(4)*a*b;
    %
    %     The left terms correspond to radial distortion, the right terms 
    %     correspond to tangential distortion. 'kc' are the calibrated
    %     radial distortion parameters.
    %
    %  3. Convertion into pixel coordinates, according to focal lengths and 
    %     center of projection:
    %
    %     u = f(1)*ud + c(1)
    %     v = f(2)*vd + c(2)
    %
    if length(calibration.kc)>4 && calibration.kc(5)~=0
        warning('Ignoring fifth-order non-zero radial distortion');
    end        
    k = [calibration.kc(1:4); 1];
    a = P_model(:,1) ./ P_model(:,3);
    b = P_model(:,2) ./ P_model(:,3);
    a2 = a.^2;
    b2 = b.^2;
    ab = a.*b;
    r2 = a2 + b2;
    r4 = r2.^2;
    uDistorted = [a.*r2  a.*r4  2*ab    2*a2+r2 a] * k;
    vDistorted = [b.*r2  b.*r4  r2+2*b2 2*ab    b] * k;
    u = calibration.fc(1) * uDistorted + calibration.cc(1);
    v = calibration.fc(2) * vDistorted + calibration.cc(2);
    
    p_model = [u(:) v(:)];
    
    subplot 212
    patch(P_marker([1 3 4 2],1), P_marker([1 3 4 2], 2), P_marker([1 3 4 2], 3), 'w', 'EdgeColor', 'w');
    patch(X,Y,Z, color);
    plot3(P_marker(1,1), P_marker(1,2), P_marker(1,3), 'k.', 'MarkerSize', 24);
    
    target(i_det,:) = mean(P_model,1);
    %keyboard
    
    subplot 222
    x = reshape(p_model(:,1), 4, []);
    y = reshape(p_model(:,2), 4, []);
    z = zeros(size(x));
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
