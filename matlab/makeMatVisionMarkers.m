markerLibrary = VisionMarkerLibrary.Load();

numCorners = [3 2 3; 2 1 2; 3 2 3];
angles = [-90   -180   -180;    -90     0   90;    0     0   90];
[xgrid, ygrid] = meshgrid([-250 0 250]);

anki = imread('~/Box Sync/Cozmo SE/VisionMarkers/ankiLogo.png');

createWebotsMat(anki, numCorners', xgrid', ygrid', angles', 90, markerLibrary);

markerLibrary.Save();