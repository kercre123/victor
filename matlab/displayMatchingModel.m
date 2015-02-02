function displayMatchingModel(frame, models, h_axesProc, h_imgProc)

[matchingModel, ~, score, matches] = findModelHomography(frame, models);

if score > .03
  %   set(h_imgProc, 'CData', matchingModel.image);
  
  cla(h_axesProc)
  imagesc(matchingModel.image, 'Parent', h_axesProc);
  axis(h_axesProc, 'image', 'off');
  subplot(h_axesProc)
  vl_plotframe(matchingModel.frames(:,matches));
  title(h_axesProc, sprintf('Score = %.1f%%', score*100));
  
else
 % set(h_imgProc, 'CData', zeros(size(frame)));
 imagesc(zeros(size(frame)), 'Parent', h_axesProc);
  title('Not Found');
end

end


