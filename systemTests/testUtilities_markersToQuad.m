
function quads = testUtilities_markersToQuad(markers)
    markers = makeCellArray(markers);
    
    quads = cell(length(markers), 1);
    
    for iQuad = 1:length(markers)
        quads{iQuad} = markers{iQuad}.corners;
    end
    