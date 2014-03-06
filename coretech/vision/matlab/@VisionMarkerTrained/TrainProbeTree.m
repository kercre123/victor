function TrainProbeTree(varargin)

% TODO: move these parameters to derived classes
sqFrac = VisionMarkerTrained.SquareWidthFraction;
padFrac = VisionMarkerTrained.FiducialPaddingFraction;

probeTree = TrainProbes('maxDepth', 50, 'addRotations', true, ...
    'markerImageDir', VisionMarkerTrained.TrainingImageDir, ...
    'imageCoords', [-padFrac 1+padFrac], ...
    'probeRegion', [sqFrac+padFrac 1-sqFrac-padFrac], ...
    'probeRadius', 0.8, 'numPerturbations', 250, 'perturbSigma', 1, 'workingResolution', 30, 'DEBUG_DISPLAY', true); %#ok<NASGU>

if isempty(probeTree)
    error('Training failed!');
else
    savePath = fileparts(mfilename('fullpath'));
    save(fullfile(savePath, 'probeTree.mat'), 'probeTree');
    
    fprintf('Probe tree re-trained and saved.  You will need to clear any existing VisionMarkers.\n');
end

end

