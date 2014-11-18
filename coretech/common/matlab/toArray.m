
% function outString = toArray(in)

% Prints out the array in a format to copy-paste into an array

function outString = toArray(in)
    
    if isa(in, 'double') || isa(in, 'single')
        formatString = '%f, ';
    else
        formatString = '%d, ';
    end
    
    outString = '';
    
    for y = 1:size(in,1)
        outString = [outString, sprintf(formatString, in(y,:)), sprintf('\n')];
    end