ForegroundColor = [ 14 108 184]/255; % Anki light blue
%BackgroundColor = [107 107 107]/255; % Anki light gray
BackgroundColor = [1 1 .7]; % Light yellow

% We will create all mat markers with the lower left corner marker in the
% actual lower left corner of the image -- with the code/icon rotated
% instead.  We can then rotate these codes when we "print" them on the mat
% and keep track of their printed orientation so we can take into account
% when the robot seems them.
ankiMat = cell(1,9);
angles = [-90   -180   -180    -90     0   90     0     0   90];

commonArgs = {'AddPadding', false, 'ForegroundColor', ForegroundColor, ...
    'BackgroundColor', BackgroundColor};

ankiMat{1} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(1)), 'NumCorners', 3, commonArgs{:});
ankiMat{2} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(2)), 'NumCorners', 2, commonArgs{:});
ankiMat{3} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(3)), 'NumCorners', 3, commonArgs{:});
ankiMat{4} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(4)), 'NumCorners', 2, commonArgs{:});
ankiMat{5} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(5)), 'NumCorners', 1, commonArgs{:});
ankiMat{6} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(6)), 'NumCorners', 2, commonArgs{:});
ankiMat{7} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(7)), 'NumCorners', 3, commonArgs{:});
ankiMat{8} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(8)), 'NumCorners', 2, commonArgs{:});
ankiMat{9} = VisionMarker.DrawFiducial('Image', imrotate(anki, -angles(9)), 'NumCorners', 3, commonArgs{:});

%%
[xgrid, ygrid] = meshgrid([-250 0 250]);
xgrid = xgrid';
ygrid = ygrid';

markerLibrary = VisionMarkerLibrary();
for i = 1:9
    subplot(3,3,i), imshow(imrotate(ankiMat{i}, angles(i)))
    
    markerLibrary.AddMarker(ankiMat{i}, 'Name', sprintf('ANKI-MAT-%d', i), ...
        'Size', 90, 'Origin', [xgrid(i) ygrid(i) 0], 'Angle', angles(i));
    imwrite(imresize(ankiMat{i}, [512 512]), fullfile('~/Code/products-cozmo/robot/simulator/worlds', sprintf('ankiMat%d.png', i)));
end

%%
markerLibrary = VisionMarkerLibrary();

numCorners = [3 2 3; 2 1 2; 3 2 3];
angles = [-90   -180   -180;    -90     0   90;    0     0   90];
[xgrid, ygrid] = meshgrid([-250 0 250]);

createWebotsMat(anki, numCorners', xgrid', ygrid', angles', 90, markerLibrary);

save(fullfile(fileparts(mfilename('fullpath')), 'cozmoMarkerLibrary.mat'), 'markerLibrary');