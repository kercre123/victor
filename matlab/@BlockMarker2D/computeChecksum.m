function checksumTruth = computeChecksum(binaryBlock, binaryFace)

numBlockBits = length(binaryBlock);
numFaceBits  = length(binaryFace);

numCheckBits = length(BlockMarker2D.CheckBits);

checksumTruth = false(1, numCheckBits);
i_block1 = 1;
for i_check = 1:numCheckBits
    
    i_block2 = mod(i_block1, numBlockBits) + 1;
    i_face = mod(i_check-1, numFaceBits) + 1;
    
    checksumTruth(i_check) = xor( binaryFace(i_face), ...
        xor(binaryBlock(i_block1), binaryBlock(i_block2)) );
    
    i_block1 = mod(i_block1, numBlockBits) + 1;
    
end

end