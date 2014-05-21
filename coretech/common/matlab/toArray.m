
% function toArray(in)

% Prints out the array in a format to copy-paste into an array

function toArray(in)
  
  for y = 1:size(in,1)
    disp(sprintf('%d, ', in(y,:)));
  end