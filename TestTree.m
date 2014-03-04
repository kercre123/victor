function labelName = TestTree(node, img, probeRadius)
assert(isa(img, 'double'));
if nargin < 3
    probeRadius = 0;
end

if isfield(node, 'labelName')
    labelName = node.labelName;
elseif all(isfield(node, {'x', 'y'}))
   xi = round(node.x * size(img,2) + 1);
   yi = round(node.y * size(img,1) + 1);
   
   value = img(yi,xi);
   
   if probeRadius > 0
       theta = linspace(0,2*pi,32);
       if value > .5
           color = 'r';
       else
           color = 'g';
       end
       plot(xi+probeRadius*cos(theta),yi+probeRadius*sin(theta), color);
   end
   
   if ~all(isfield(node, {'rightChild', 'leftChild'}))
       labelName = node.remaining;
       warning('Reached leaf with no children and remaining labels:');
   else
       
       if value > .5
           labelName = TestTree(node.rightChild, img, probeRadius);
       else
           labelName = TestTree(node.leftChild, img, probeRadius);
       end
   end

else
    %warning('Reached abandoned node.');
    labelName = 'UNKNOWN';
end

end