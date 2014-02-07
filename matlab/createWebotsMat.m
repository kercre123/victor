function createWebotsMat(images, numCorners, xgrid, ygrid, angles, sizes, markerLibrary, varargin)

ForegroundColor = [ 14 108 184]/255; % Anki light blue
%BackgroundColor = [107 107 107]/255; % Anki light gray
BackgroundColor = [1 1 .7]; % Light yellow
ProtoDir = fullfile(fileparts(mfilename('fullpath')), '../robot/simulator/protos');
WorldDir = fullfile(fileparts(mfilename('fullpath')), '../robot/simulator/worlds');

parseVarargin(varargin{:});

if ~iscell(images)
    img = images;
    images = cell(size(xgrid));
    [images{:}] = deal(img);
end

if isscalar(angles)
    angles = angles*ones(size(images));
end

if isscalar(sizes)
    sizes = sizes*ones(size(images));
end

if isscalar(numCorners)
    numCorners = numCorners*ones(size(images));
end

fid = fopen(fullfile(ProtoDir, 'CozmoMat.proto'), 'wt');

if fid == -1
    error('Unable to create proto file "%s".', fullfile(ProtoDir, 'CozomMat.proto'));
end

C = onCleanup(@()fclose(fid));

fprintf(fid, '#VRML_SIM V7.3.0 utf8\n\n');
fprintf(fid, 'PROTO CozmoMat [\n');
fprintf(fid, '  field SFColor Color %.4f %.4f %.4f\n', BackgroundColor(1), BackgroundColor(2), BackgroundColor(3));
fprintf(fid, ']\n');
fprintf(fid, '{\n');
fprintf(fid, 'Solid {\n');
fprintf(fid, '  rotation 0 1 0 0\n');
fprintf(fid, '  translation 0 -0.0025 0\n');
fprintf(fid, '  children [\n');
fprintf(fid, '    Shape {\n');
fprintf(fid, '      appearance Appearance {\n');
fprintf(fid, '         material Material {\n');
fprintf(fid, '		   diffuseColor IS Color\n');
fprintf(fid, '         }\n');
fprintf(fid, '      }\n');
fprintf(fid, '      geometry Box {\n');
fprintf(fid, '        size 1 0.005 1\n');
fprintf(fid, '      }\n');
fprintf(fid, '    }\n');


set(gcf, 'Pos', [100 100 800 650]);

for i = 1:numel(images)
        
    markerImg = VisionMarker.DrawFiducial( ...
        'Image', imrotate(images{i}, -angles(i)), ...
        'NumCorners', numCorners(i), ...
        'AddPadding', false, ...
        'ForegroundColor', ForegroundColor, ...
        'BackgroundColor', BackgroundColor);
    
    % Need this initial rotation b/c canonical VisionMarker orientation in
    % 3D is vertical, i.e. in the X-Z plane, for historical reasons.
    R_to_flat = rodrigues(-pi/2*[1 0 0]);
    
    pose = Pose(rodrigues(angles(i)*pi/180*[0 0 1])*R_to_flat, ...
        [xgrid(i) ygrid(i) -CozmoVisionProcessor.WHEEL_RADIUS]');
    
    marker = VisionMarker(markerImg, 'Name', sprintf('ANKI-MAT-%d', i), ...
        'Size', sizes(i), 'Pose', pose);
        
    markerLibrary.AddMarker(marker);
    
    filename = sprintf('ankiMat%d.png', i);
    imwrite(imresize(markerImg, [512 512]), fullfile(WorldDir, filename));
    
    fprintf(fid, '    Solid {\n');
    fprintf(fid, '	    translation %.4f 0.0025 %.4f\n', xgrid(i)/1000, -ygrid(i)/1000);
    fprintf(fid, '	    rotation 0 1 0 %f\n', angles(i)*pi/180);
    fprintf(fid, '	    children [\n');
    fprintf(fid, '		  Shape {\n');
    fprintf(fid, '		    appearance Appearance {\n');
    fprintf(fid, '			  texture ImageTexture { url ["%s"] }\n', filename);
    fprintf(fid, '			  material Material { diffuseColor 1 1 1 }\n');
    fprintf(fid, '		    }\n');
    fprintf(fid, '		    geometry Box {\n');
    fprintf(fid, '            size %f 0.00001 %f\n', sizes(i)/1000, sizes(i)/1000 );
    fprintf(fid, '  		}\n');
    fprintf(fid, '		  }\n');
    fprintf(fid, '	    ]\n');
    fprintf(fid, '    }\n');
    
end % FOR each marker

fprintf(fid, '  ] # Solid children\n\n');
fprintf(fid, '  contactMaterial "cmat_floor"\n');
fprintf(fid, '  boundingObject Transform {\n');
fprintf(fid, '    translation 0.5 0 0.5\n');
fprintf(fid, '    children [\n');
fprintf(fid, '      Plane {\n');
fprintf(fid, '      }\n');
fprintf(fid, '    ]\n');
fprintf(fid, '  }\n');
fprintf(fid, '} # Solid\n\n');
fprintf(fid, '} # Proto\n');

end % function createWebotsMat()

