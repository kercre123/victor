%img = imresize( imread('/Users/andrew/Box Sync/Cozmo SE/systemTestImages/cozmo_date2014_01_29_time11_41_05_frame1.png'), [240 320]);
calib = load('/Users/andrew/Box Sync/Cozmo SE/calibCozmoProto2.mat');

K = [calib.fc(1) 0 calib.cc(1); 0 calib.fc(2) calib.cc(2); 0 0 1];

Rtrue = rodrigues(-10*pi/180*[1 0 0]) * rodrigues(5*pi/180*[0 0 1]) * rodrigues(3*pi/180*[0 0 1]);
Ttrue = [10; 12; 100];
markerSize = 26;

marker3d = (markerSize/2)*[-1 -1 0; -1 1 0; 1 -1 0; 1 1 0]';

% Get ground truth projection
proj = K*[Rtrue Ttrue]*[marker3d; ones(1,4)];
u_true = proj(1,:)./proj(3,:);
v_true = proj(2,:)./proj(3,:);

% Corrupt with a little noise
u = u_true + 0.5*randn(1,4);
v = v_true + 0.5*randn(1,4);

corners = [u;v];

rays2d = K\[corners; ones(1,4)];
rays2d = rays2d ./ (ones(3,1)*sqrt(sum(rays2d.^2,1)));


%%
dMinOuter = inf;
bestPose_matlab = [];
bestValidation = [];
h_axes = zeros(1,4);
for i_validate = 1:4

    estimateIndex = [1:(i_validate-1) (i_validate+1):4];
    
    possiblePoses = p3p(marker3d(:,estimateIndex), rays2d(:,estimateIndex));
    
    pose = cell(1,4);
    d = zeros(1,4);
    for i_solution = 1:4
        index = (i_solution-1)*4 + 1;
        pose{i_solution} = inv(Pose(possiblePoses(:,index+(1:3)), possiblePoses(:,index)));
        
        proj = K*[pose{i_solution}.Rmat pose{i_solution}.T]*[marker3d;ones(1,4)];
        u_possible = proj(1,:) ./ proj(3,:);
        v_possible = proj(2,:) ./ proj(3,:);
        
        d(i_solution) = sqrt((u(i_validate)-u_possible(i_validate))^2 + (v(i_validate)-v_possible(i_validate))^2);
        
    end
    [dMinInner,whichSolution] = min(d);
    if dMinInner < dMinOuter
        dMinOuter = dMinInner;
        bestPose_matlab = pose{whichSolution};
        bestValidation = i_validate;
    end
    
    proj = K*[pose{whichSolution}.Rmat pose{whichSolution}.T]*[marker3d;ones(1,4)];
    u_best = proj(1,:) ./ proj(3,:);
    v_best = proj(2,:) ./ proj(3,:);
    
    h_axes(i_validate) = subplot(2,2,i_validate); hold off
    plot(corners(1,:), corners(2,:), 'rx');
    hold on, grid on, axis image
    plot(u, v, 'bo');
    plot(u_best(i_validate), v_best(i_validate), 'g+');
    title(h_axes(i_validate), dMinInner)
end
    
linkaxes(h_axes)

[R,T] = mexComputePose(corners', marker3d', calib.fc, calib.cc, [240 320]);
bestPose_cpp = Pose(R,T);

proj = K*[bestPose_cpp.Rmat bestPose_cpp.T]*[marker3d;ones(1,4)];
u_cpp = proj(1,:) ./ proj(3,:);
v_cpp = proj(2,:) ./ proj(3,:);

subplot(h_axes(bestValidation))
plot(u_cpp, v_cpp, 'ms');

