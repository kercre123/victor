% function [labelName, labelID] = TestTree(node, img, tform, threshold, pattern, drawProbes)

% load('C:\Anki\products-cozmo\coretech\vision\matlab\@VisionMarkerTrained\probeTree.mat')
% image = double(imresize(rgb2gray(imread('Z:\Box Sync\Cozmo SE\blockImages\testTrainedCodes.png')), [240,320]));
% tform = maketform('projective', inv([-12.000646 -77.999832 217.999969; 78.999649 -11.999959 21.999990; -0.0 0.0 1.0])');
% [labelName, labelID] = TestTree(probeTree, image, tform, 128, [], true)

function [labelName, labelID] = TestTree(node, img, tform, threshold, pattern, drawProbes)

if nargin < 6
    drawProbes = false;
end

% if nargin < 5 || isempty(pattern)
%     N_angles = 8;
%     probeRadius = .06;
%     angles = linspace(0, 2*pi, N_angles +1);
%     pattern.x = [0 probeRadius*cos(angles)];
%     pattern.y = [0 probeRadius*sin(angles)];   
% end



if isfield(node, 'labelName')
    labelName = node.labelName;
    labelID   = node.labelID;
    
elseif all(isfield(node, {'x', 'y', 'whichProbe'}))
        
    if isvector(img)
        % Raw probe values
        value = img(node.whichProbe);
    else
        assert(isa(img, 'double'));
        if nargin < 3 || isempty(tform)
            tform = maketform('affine', eye(3)); %[size(img,2)-1 0 1; 0 size(img,1)-1 1; 0 0 1]');
        end
        
        %[xi,yi] = tformfwd(tform, node.x+pattern.x, node.y+pattern.y);
        temp = tform.tdata.T' * [node.x+pattern.x; node.y+pattern.y; ones(1,length(pattern.y))];
        xi = temp(1,:)./temp(3,:);
        yi = temp(2,:)./temp(3,:);
        
        % value = mean(interp2(img, xi(:), yi(:), 'linear')); % TODO: just use nearest?
        index = round(yi(:)) + (round(xi(:))-1)*size(img,1);
        value = mean(img(index));
                
        if drawProbes
            %theta = linspace(0,2*pi,24);
            if value > threshold
                color = 'r';
            else
                color = 'g';
            end
            %plot(xi+drawProbes*cos(theta),yi+drawProbes*sin(theta), color);
            plot(xi,yi, [color '.'], 'MarkerSize', 10);
        end
    end
   
   if ~all(isfield(node, {'rightChild', 'leftChild'}))
       labelName = node.remaining;
       warning('Reached leaf with no children and remaining labels:');
   else
       
       %if value > .5
       if value > threshold
           [labelName, labelID] = TestTree(node.rightChild, img, tform, threshold, pattern, drawProbes);
       else
           [labelName, labelID] = TestTree(node.leftChild, img, tform, threshold, pattern, drawProbes);
       end
   end

else
    %warning('Reached abandoned node.');
    labelName = 'UNKNOWN';
    labelID   = -1;
end

end