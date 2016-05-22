function [centerEdgeScore, symmetryScore] = ...
    scoreWindow(win, centerEdgeLUT, centerEdgeProbes, ...
    symmetryLUT, symmetryProbes)

centerEdgeScore = scoreWindowHelper(win, centerEdgeLUT, centerEdgeProbes);

symmetryScore = scoreWindowHelper(win, symmetryLUT, symmetryProbes);

end


function score = scoreWindowHelper(win, scoreLUT, probes)

% assume win is an appropriately-sized window of binned image values
% can call this twice, once with centerEdgeLUT/Probes, and again with
% symmetryLUT/Probes


binRow = win(probes(:,1));
binCol = win(probes(:,2));
binIndex = binRow + (binCol-1)*size(scoreLUT,1);

score = min(scoreLUT(binIndex));
    
end









