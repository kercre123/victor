function MirrorImages(inputDir)
% MirrorImages(inputDir)
%
% Mirror all jpg/png images in all first-level subdirectories of inputDir.
% TODO: Also mirror images in inputDir.
%
% Creates <originalFilename>_mirrord.<ext> for each image. If the image
% already exists, does not recreate it. Also does not re-mirror existing 
% *_mirrored files, so this is safe to re-run on a directory.
%

subDirs = getdirnames(inputDir);

pbar = ProgressBar;

for iSub = 1:length(subDirs)
  fprintf('Subdir: %s\n', subDirs{iSub});

  files = getfnames(fullfile(inputDir, subDirs{iSub}), {'*.png', '*.jpg'}, 'useFullPath', false, 'excludePattern', '*_mirrored.*');
  
  pbar.set_message(subDirs{iSub});
  pbar.set(0);
  pbar.set_increment(1/length(files));
  
  for iFile = 1:length(files)
    
    [~,filename,ext] = fileparts(files{iFile});
    
    outFile = fullfile(inputDir, subDirs{iSub}, [filename '_mirrored' ext]);
    
    if ~exist(outFile, 'file')
      img = imread(fullfile(inputDir, subDirs{iSub}, files{iFile}));
      imwrite(fliplr(img), outFile, 'Quality', 95);
    end
    pbar.increment();
  end
  
end

delete(pbar);

end