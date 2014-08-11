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

InputFilePattern = 'images';

addFiducialArgs = parseVarargin(varargin{:});

if isempty(outputDir)
    outputDir = fullfile(inputDir, 'withFiducials');
end

if ~isdir(outputDir)
    mkdir(outputDir);
end

fnames = getfnames(inputDir, InputFilePattern);
for i = 1:length(fnames)
   fprintf('Adding fiducial to "%s"\n', fnames{i});
   [imgNew, alpha] = VisionMarkerTrained.AddFiducial(fullfile(inputDir, fnames{i}), addFiducialArgs{:});
   imwrite(imgNew, fullfile(outputDir, fnames{i}), 'Alpha', alpha);
end

end