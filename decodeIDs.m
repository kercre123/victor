function [blockType, faceType, isValid] = decodeIDs(value)
% Decode 16-bit code into faceType and blockType, plus validity flag.
%
% [blockType, faceType, isValid] = decodeIDs(code)
%
%
% See also: encodeIDs
% ------------
% Andrew Stein
%


totalBits = 16;
%numBlockBits = 7; % 128 possible blocks
%numFaceBits  = 4; % 16 possible faces per block
blockBits = [1 4 6 7 10 13 16];
faceBits  = [2 3 9 11];
checkBits = [5 8 12 14 15];

assert(value < 2^totalBits, 'Input code out of range.');

binaryCode = dec2bin(value, totalBits);
blockType = bin2dec(binaryCode(blockBits));
faceType  = bin2dec(binaryCode(faceBits));

binaryCode = binaryCode == '1'; % make logical

% Compare check bits in the incoming value to what they should be according
% to the decoded block and and face code
checksum = binaryCode(checkBits);

checksumTruth = false(1,5);
checksumTruth(1) = xor(binaryCode(blockBits(1)), binaryCode(blockBits(2)));
checksumTruth(2) = xor(binaryCode(blockBits(3)), binaryCode(blockBits(4)));
checksumTruth(3) = xor(binaryCode(blockBits(5)), binaryCode(blockBits(6)));
checksumTruth(4) = xor(binaryCode(faceBits(1)),  binaryCode(faceBits(2)));
checksumTruth(5) = xor(binaryCode(checkBits(5)), binaryCode(blockBits(7)));

isValid = all(checksum == checksumTruth);

end
