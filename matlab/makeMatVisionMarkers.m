markerLibrary = VisionMarkerLibrary.Load();

numCorners = [3 2 3; 
              2 1 2; 
              3 2 3];
          
angles = [-90   -180   -180;    
          -90     0      90;
            0     0      90];
        
xgrid  = [-250 0 250;
          -250 0 250;
          -250 0 250];
      
ygrid  = [ 250  250  250;
             0    0    0;
          -250 -250 -250];
        
%[xgrid, ygrid] = meshgrid([-250 0 250]);

anki = imread('~/Box Sync/Cozmo SE/VisionMarkers/ankiLogo.png');

createWebotsMat(anki, numCorners', xgrid', ygrid', angles', 90, markerLibrary);

markerLibrary.Save();

%% 
% This will display code you can paste into BlockWorld constructor to
% create the markers for the mat with the correct pose

for i = 1:9
    name = sprintf('ANKI-MAT-%d',i);
    code = markerLibrary.GetMarkerByName(name).byteArray;
    
    fprintf('mat->AddMarker({%d', code(1));
    for j = 2:length(code)
        fprintf(', %d', code(j));
    end
    fprintf('},\n')
    
    axis = markerLibrary.GetMarkerByName(name).pose.axis;
    angle = markerLibrary.GetMarkerByName(name).pose.angle;
    T = markerLibrary.GetMarkerByName(name).pose.T;
    T(3)= 0;
    fprintf('Pose3d(%f, {%f,%f,%f}, {%f,%f,%f}),\n%f);\n', angle, ...
        axis(1), axis(2), axis(3), T(1), T(2), T(3), ...
        markerLibrary.GetMarkerByName(name).size);
end
