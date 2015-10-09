function faceDetectDemo

clear drawDetectedFaces

CameraCapture('resolution', [640 480], ...
    'processFcn', @drawDetectedFaces, 'doContinuousProcessing', true);

end

function drawDetectedFaces(img, h_axes, h_imgProc)

img = im2uint8(mean(im2double(img),3));

delete(findobj(h_axes, 'Tag', 'FaceRect'));

[faceRects, ~, hands] = mexFaceDetect(img);
numFaces = size(faceRects,1);

set(h_imgProc, 'CData', img);

title(h_axes, sprintf('%d Faces', numFaces));

if numFaces > 0
    faceRects(:,1:2) = faceRects(:,1:2) + 1;
    
    for i = 1:numFaces
        rectangle('Pos', faceRects(i,1:4), 'Tag', 'FaceRect', ...
            'EdgeColor', 'r', 'LineWidth', 2, 'Parent', h_axes);
        
%         eye1Rect = faceRects(i,5:8);
%         if all(eye1Rect(1:2) > 0)
%             rectangle('Pos', eye1Rect+[1 1 0 0], 'Curv', [1 1], 'Tag', 'FaceRect', ...
%                 'EdgeColor', 'b', 'LineWidth', 2, 'Parent', h_axes);
%         end
%         
%         eye2Rect = faceRects(i,9:12);
%         if all(eye2Rect(1:2) > 0)
%             rectangle('Pos', eye2Rect+[1 1 0 0], 'Curv', [1 1], 'Tag', 'FaceRect', ...
%                 'EdgeColor', 'b', 'LineWidth', 2, 'Parent', h_axes);
%         end
    end
    
end

numHands = size(hands,1);
if numHands > 0
  hands(:,1:2) = hands(:,1:2) + 1;
  
  for i = 1:numHands
    rectangle('Pos', hands(i,:), 'Tag', 'FaceRect', ...
      'EdgeColor', 'g', 'LineWidth', 2, 'Parent', h_axes);
  end
end
 
% persistent ctr;
% 
% if isempty(ctr)
%     ctr = 0;
% end
% 
% ctr = ctr + 1;
%
% temp = getframe(h_axes);
% outputDir = '~/temp/faceDetectTest';
% if ~isdir(outputDir)
%     mkdir(outputDir);
% end
% imwrite(temp.cdata, fullfile(outputDir, sprintf('frame%.5d.png', ctr)));


end

