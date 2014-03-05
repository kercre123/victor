function [labelName, labelID] = TestTree(node, img, tform, threshold, pattern, drawProbes)
assert(isa(img, 'double'));
if nargin < 6
    drawProbes = false;
end

if nargin < 5 || isempty(pattern)
    N_angles = 8;
    probeRadius = .06;
    angles = linspace(0, 2*pi, N_angles +1);
    pattern.x = [0 probeRadius*cos(angles)];
    pattern.y = [0 probeRadius*sin(angles)];   
end

if nargin < 3 || isempty(tform)
    tform = maketform('affine', eye(3)); %[size(img,2)-1 0 1; 0 size(img,1)-1 1; 0 0 1]');
end

if isfield(node, 'labelName')
    labelName = node.labelName;
    labelID   = node.labelID;
    
elseif all(isfield(node, {'x', 'y'}))
        
    [xi,yi] = tformfwd(tform, node.x+pattern.x, node.y+pattern.y);
    
    value = mean(interp2(img, xi(:), yi(:), 'linear')); % TODO: just use nearest?
    
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
   
   if ~all(isfield(node, {'rightChild', 'leftChild'}))
       labelName = node.remaining;
       warning('Reached leaf with no children and remaining labels:');
   else
       
       if value > .5
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