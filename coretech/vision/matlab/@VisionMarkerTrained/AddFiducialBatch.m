function AddFiducialBatch(inputDir, outputDir, varargin)
% Add fiducials to all the images in a directory.
%
% AddFiducialBatch(inputDir, outputDir, <AddFiducialParams...>)
%
%
% See also: VisionMarkerTrained/AddFiducial
% -----------
% Andrew Stein
%

if ~isdir(outputDir)
    mkdir(outputDir);
end

fnames = getfnames(inputDir, 'images');
for i = 1:length(fnames)
   fprintf('Adding fiducial to "%s"\n', fnames{i});
   [imgNew, alpha] = VisionMarkerTrained.AddFiducial(fullfile(inputDir, fnames{i}), varargin{:});
   imwrite(imgNew, fullfile(outputDir, fnames{i}), 'Alpha', alpha);
end

end