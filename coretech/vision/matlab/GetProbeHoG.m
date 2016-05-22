function HoG = GetProbeHoG(probeValues, varargin)
% Encode probe values as a Histogram of Gradients (HoG) feature.
%
% ------------
% Andrew Stein
%

numOrientBins = 8;
numScales = 4;
useLog = true;
doBlurring = false;

parseVarargin(varargin{:});

if isstruct(probeValues)
  assert(isfield(probeValues, 'probeValues'));
  
  probeValues = probeValues.probeValues;
  numExamples = size(probeValues,2);
  
  pBar = ProgressBar('HoG Encoding');
  pBar.set_increment(1/numExamples);
  pBar.showTimingInfo = true;
  pBar.set(0);
  
  HoG = cell(numExamples,1);
  for iExample = 1:numExamples
    HoG{iExample} = column(GetProbeHoG(reshape(probeValues(:,iExample), 32, 32), varargin{:}));
    pBar.increment();
  end
  delete(pBar);
  
  HoG = [HoG{:}];
  return
end

assert(size(probeValues,1) == 32 && size(probeValues,2)==32, ...
  'Expecting 32x32 probeValues array.')

probeValues = im2double(probeValues);

if useLog
  probeValues = log(max(eps, probeValues));
end

persistent whichHist

% TODO: make each probe contribute to nearest 4 histograms via bilinear interp
if isempty(whichHist)
  [xgrid,ygrid] = meshgrid(ceil(4*linspace(eps,1,32)));
  whichHist = ygrid + (xgrid-1)*4;
  assert(all(whichHist(:) > 0) && all(whichHist(:) <= 16));
  whichHist = [whichHist(:); whichHist(:)];
end


  
HoG = cell(numScales,1);
for iScale = 1:numScales
  scale = 2^(iScale-1);
  if doBlurring && scale > 1
    scaledProbes = separable_filter(probeValues, gaussian_kernel(scale));
  else
    scaledProbes = probeValues;
  end
  
  Ix = (image_right(scaledProbes,scale) - image_left(scaledProbes,scale));%/(2*scale);
  Iy = (image_down(scaledProbes,scale)  - image_up(scaledProbes,scale));%/(2*scale);
  
  mag = sqrt(Ix.^2 + Iy.^2); % TODO: try without (slow) sqrt?
  orient = atan2(double(Iy), double(Ix)); 
  
  orient(abs(-pi - orient) < eps) = pi; % from [-pi, pi] to (-pi, pi]
  
  % From (-pi, pi] to (0, 2pi] and then to (0, 1] and finally to (0, numBins] and 1:numBins
  scaledOrient = (orient + pi)/(2*pi) * numOrientBins;
  binnedOrient_R = ceil(scaledOrient);
  weight_L = binnedOrient_R - scaledOrient;
  binnedOrient_L = binnedOrient_R - 1;
  binnedOrient_L(binnedOrient_L == 0) = numOrientBins;
  weight_R = 1 - weight_L;
  
  %     % Sanity checks
  %     assert(all(binnedOrient_L(:) > 0) && all(binnedOrient_L(:) <= numOrientBins));
  %     assert(all(binnedOrient_R(:) > 0) && all(binnedOrient_R(:) <= numOrientBins));
  %     assert(all(weight_L(:) >= 0) && all(weight_L(:) <= 1))
  %     assert(all(weight_R(:) >= 0) && all(weight_R(:) <= 1))
  
  HoG{iScale} = accumarray([whichHist [binnedOrient_L(:); binnedOrient_R(:)]], ...
    [mag(:).*weight_L(:); mag(:).*weight_R(:)], [16 numOrientBins]);
  
  % Normalize each histogram individually
  HoG{iScale} = im2uint8(HoG{iScale} ./ (sum(HoG{iScale},2)*ones(1,numOrientBins)));
  
%   % Normalize all histograms together
%   HoG{iScale} = HoG{iScale} / sum(HoG{iScale}(:));
end % FOR each scale

HoG = [HoG{:}];

end
