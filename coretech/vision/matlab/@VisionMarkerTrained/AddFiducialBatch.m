function AddFiducialBatch(inputDir, outputDir, varargin)

if ~isdir(outputDir)
    mkdir(outputDir);
end

fnames = getfnames(inputDir, 'images');
for i = 1:length(fnames)
   fprintf('Adding fiducial to "%s"\n', fnames{i});
   img = imread(fullfile(inputDir, fnames{i}));
   imgNew = VisionMarkerTrained.AddFiducial(img, varargin{:});
   imwrite(imgNew, fullfile(outputDir, fnames{i}));
end

end