function quads = testUtilities_jsonToQuad(jsonQuads)
    jsonQuads = makeCellArray(jsonQuads);
    
    quads = cell(length(jsonQuads), 1);
    
    for iQuad = 1:length(jsonQuads)
        quads{iQuad} = [...
            jsonQuads{iQuad}.x_imgUpperLeft, jsonQuads{iQuad}.y_imgUpperLeft;
            jsonQuads{iQuad}.x_imgLowerLeft, jsonQuads{iQuad}.y_imgLowerLeft;
            jsonQuads{iQuad}.x_imgUpperRight, jsonQuads{iQuad}.y_imgUpperRight;
            jsonQuads{iQuad}.x_imgLowerRight, jsonQuads{iQuad}.y_imgLowerRight];
    end