function checksumTruth = computeChecksum(binaryCode)

faceBits = BlockMarker2D.IdBits('F');
blockBits = BlockMarker2D.IdBits('B');

numBlockBits = length(blockBits);
numFaceBits  = length(faceBits);

numCheckBits = length(BlockMarker2D.CheckBits);

checksumTruth = false(1, numCheckBits);
i_block1 = 1;
for i_check = 1:numCheckBits
    
    i_block2 = mod(i_block1, numBlockBits) + 1;
    i_face = mod(i_check-1, numFaceBits) + 1;
    
    checksumTruth(i_check) = xor( binaryCode(faceBits(i_face)), ...
        xor(binaryCode(blockBits(i_block1)), binaryCode(blockBits(i_block2))) );
    
    i_block1 = mod(i_block1, numBlockBits) + 1;
    
end

end