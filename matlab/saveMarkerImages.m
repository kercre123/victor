function saveMarkerImages(varargin)
% Batch save a set of Block face-marker images.
%
% saveMarkerImages(blocks, faces, varargin)
%
%  Saves an image for each block/face ID pair.
%
%  'savePath' - path to save BlockXXFaceYY.png images
%
%  'blocks' - vector of block IDs
%
%  'faces'  - vector of face IDs
%
%  'markerParams' - cell array of name/value pairs to be passed into
%     generageMarkerImage for creating the markers
%
% See also generateMarkerImage
% ------------
% Andrew Stein
%

blocks = 5:5:25;
faces  = 1:6;
savePath = '~/Documents/Anki/Data/BlockWorldMarkers/';
markerParams = {};
        
parseVarargin(varargin{:});

h_fig = namedFigure('MarkerImage');

for block = row(blocks)
    for face = row(faces)
        temp = generateMarkerImage(block, face, markerParams{:}); 
        imwrite(temp, fullfile(savePath, ...
            sprintf('Block%.3d_Face%.2d.png', block, face))); 
    end
end

end