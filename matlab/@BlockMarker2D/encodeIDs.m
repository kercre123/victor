 function binaryCode = encodeIDs(blockType, faceType)
% Encode faceType and blockType into a 2D code with checksum.
%
% binaryCode = encodeIDs(blockType, faceType)
%
%    Call with no outputs to display the code.
% 
% See also: BlockMarker2D/decodeIDs
% ------------
% Andrew Stein
%

assert(blockType > 0, 'blockType must be > 0.');
assert(faceType > 0,  'faceType must be > 0.');
  
blockBits = BlockMarker2D.IdBits('B');
numBlockBits = length(blockBits);
faceBits  = BlockMarker2D.IdBits('F');
numFaceBits = length(faceBits);
checkBits = BlockMarker2D.CheckBits;

totalBits = numel(BlockMarker2D.Layout);

assert(blockType <= 2^numBlockBits, 'blockType too large');
assert(faceType <= 2^numFaceBits, 'faceType too large');

binaryCode = false(1,totalBits);

binaryCode(blockBits) = dec2bin(blockType-1, numBlockBits) == '1';
binaryCode(faceBits)  = dec2bin(faceType-1,  numFaceBits) == '1';
binaryCode(checkBits) = BlockMarker2D.computeChecksum( ...
    binaryCode(blockBits), binaryCode(faceBits));

binaryCode(BlockMarker2D.UpBit) = false;
binaryCode([BlockMarker2D.LeftBit BlockMarker2D.RightBit ...
    BlockMarker2D.DownBit]) = true;

binaryCode = reshape(binaryCode, size(BlockMarker2D.Layout));

if nargout == 0
    imagesc(~binaryCode), axis image
    title(sprintf('Block %d, Face %d', blockType, faceType));
    clear binaryCode
end

end


