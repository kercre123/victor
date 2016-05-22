
% function cellArray = makeCellArray(inArray)

% If inArray is not a cell array, make it one

function cellArray = makeCellArray(inArray)
    
    if ~iscell(inArray)
        cellArray = { inArray };
    else 
        cellArray = inArray;
    end