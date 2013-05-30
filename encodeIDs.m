function value = encodeIDs(blockType, faceType)
% Encode faceType and blockType into a 16-bit code with checksum.
%
% value = encodeIDs(blockType, faceType)
%
% See also: decodeIDs
% ------------
% Andrew Stein
%

% Bits are distributed so that all checksum bits aren't at end.
%   Here's the key: [B = block bit, F = face bit, C = checksum bit]
%     B C | F B
%     F B | B C
%     ----+----
%     F B | F C
%     B C | C B
% [Does this really matter though?]

totalBits = 16;
numBlockBits = 7; % 128 possible blocks
numFaceBits  = 4; % 16 possible faces per block
blockBits = [1 4 6 7 10 13 16];
faceBits  = [2 3 9 11];
checkBits = [5 8 12 14 15];

assert(blockType < 2^numBlockBits, 'blockType too large');
assert(faceType < 2^numFaceBits, 'faceType too large');

binaryCode = false(1,totalBits);

binaryCode(blockBits) = dec2bin(blockType, numBlockBits) == '1';
binaryCode(faceBits)  = dec2bin(faceType,  numFaceBits) == '1';

% Compute check bits:
%   checkBit 1 = xor(B1, B2);
%   checkBit 2 = xor(B3, B4);
%   checkBit 3 = xor(B5, B6);
%   checkBit 4 = xor(F1, F2);
%   checkBit 5 = xor(xor(F3, F4), B7);
binaryCode(checkBits(1)) = xor(binaryCode(blockBits(1)), binaryCode(blockBits(2)));
binaryCode(checkBits(2)) = xor(binaryCode(blockBits(3)), binaryCode(blockBits(4)));
binaryCode(checkBits(3)) = xor(binaryCode(blockBits(5)), binaryCode(blockBits(6)));
binaryCode(checkBits(4)) = xor(binaryCode(faceBits(1)),  binaryCode(faceBits(2)));
binaryCode(checkBits(5)) = xor(binaryCode(faceBits(3)),  binaryCode(faceBits(4)));
binaryCode(checkBits(5)) = xor(binaryCode(checkBits(5)), binaryCode(blockBits(7)));

value = bin2dec(num2str(binaryCode));

end


