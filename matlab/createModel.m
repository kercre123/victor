function object = createModel(img, name)

useEdgeFoci = true;
minScale = [];

img = im2single(img);

if size(img,3) > 1 
  img = rgb2gray(img);
end

% if useEdgeFoci
%   scales = 8*2.^[-1/3 0 1/3 2/3 1];
%   [h,m] = edgeFoci(img, scales);
%   [my,mx,mscale] = ind2sub(size(h), find(m & h > 0.001));
%   edgeFociFrames = [mx(:) my(:) column(scales(mscale)) zeros(length(mx),1)]';
%   
%   [frames, descriptors] = vl_sift(img, 'Frames', edgeFociFrames, 'Orientations', 'Magnif', 4);
% else
%   [frames, descriptors] = vl_sift(img);
% end

if useEdgeFoci
  numOctaves = 1;
  frames = cell(1,numOctaves);
  descriptors = cell(1, numOctaves);
  for iOctave = 1:2
    if iOctave > 1
      imgScaled = imresize(img, 1/iOctave, 'bilinear');
    else
      imgScaled = img;
    end
    
    scales = 8*2.^[-1/3 0 1/3 2/3 1];
    [h,m] = edgeFoci(imgScaled, scales);
    [my,mx,mscale] = ind2sub(size(h), find(m & h > 0.001));
    edgeFociFrames = [mx(:) my(:) column(scales(mscale)) zeros(length(mx),1)]';
    [frames{iOctave}, descriptors{iOctave}] = vl_covdet(img, ...
      'Frames', edgeFociFrames, ...
      'Descriptor', 'SIFT', ...
      'EstimateAffineShape', false, ...
      'EstimateOrientation', true, ...
      'DoubleImage', false);
    
    if iOctave > 1
      frames{iOctave}(1:2,:) = (frames{iOctave}(1:2,:)-1)*iOctave + 1;
      frames{iOctave}(3,:) = iOctave*frames{iOctave}(3,:);
    end
  end
  frames = [frames{:}];
  descriptors = [descriptors{:}];
else
  [frames, descriptors] = vl_covdet(img, ...
    'Method', 'DoG', ...
    'Descriptor', 'SIFT', ...
    'EstimateAffineShape', false, ...
    'EstimateOrientation', true, ...
    'DoubleImage', false);
end

if ~isempty(minScale)
  toRemove = frames(3,:) < minScale;
  frames(:,toRemove) = [];
  descriptors(:,toRemove) = [];
end

object = struct(...
  'name', name, ...
  'image', img, ...
  'frames', frames, ...
  'descriptors', descriptors);

end