function labelName = TestTree(node, img)
assert(isa(img, 'double'));

if isfield(node, 'labelName')
    labelName = node.labelName;
elseif all(isfield(node, {'x', 'y'}))
   xi = round(node.x * size(img,2) + 1);
   yi = round(node.y * size(img,1) + 1);
   
   value = img(yi,xi);
   
   if ~all(isfield(node, {'rightChild', 'leftChild'}))
       labelName = node.remaining;
       warning('Reached leaf with no children and remaining labels:');
   else
       
       if value > .5
           labelName = TestTree(node.rightChild, img);
       else
           labelName = TestTree(node.leftChild, img);
       end
   end
else
    %warning('Reached abandoned node.');
    labelName = 'UNKNOWN';
end

end