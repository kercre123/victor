
% function outString = toArray(in)

% Prints out the array in a format to copy-paste into an array

function outString = toArray(in, useNumpyStyle)
    
    if ~exist('useNumpyStyle', 'var')
        useNumpyStyle = false;
    end
    
    if isa(in, 'double') || isa(in, 'single')
        formatString = '%f, ';
    else
        formatString = '%d, ';
    end
    
    outString = '';
    
    if useNumpyStyle
        outString = [outString, 'np.array(['];
        for y = 1:size(in,1)
            if y == 1
                leftBracket = '[';
            else
                leftBracket = '          [';
            end
            
            numbers = sprintf(formatString, in(y,:));
            
            outString = [outString, leftBracket, numbers(1:(end-2)), sprintf('],\n')];
        end
        outString = [outString(1:(end-2)), sprintf('])\n')];
    else
        for y = 1:size(in,1)
            outString = [outString, sprintf(formatString, in(y,:)), sprintf('\n')];
        end
    end