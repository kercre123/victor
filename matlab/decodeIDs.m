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

if nargin < 3
    layout = ['BCOFB';
              'FBBBC';
              'OCXCO';
              'FBCFC';
              'BCOCB'];
end
layout = layout(:);

blockBits = find(layout == 'B');
numBlockBits = length(blockBits);
faceBits  = find(layout == 'F');
numFaceBits = length(faceBits);
checkBits = find(layout == 'C');
numCheckBits = length(checkBits);

totalBits = length(layout);
encodingBits = [blockBits; faceBits; checkBits];

assert(value < 2^totalBits, 'Input code out of range.');

binaryCode = repmat('0', [1 totalBits]);
binaryCode(encodingBits) = dec2bin(value, length(encodingBits));
blockType = bin2dec(binaryCode(blockBits)) + 1;
faceType  = bin2dec(binaryCode(faceBits)) + 1;

if blockType == 0 || faceType == 0
    isValid = false;
else
    binaryCode = binaryCode == '1'; % make logical
    
    % Compare check bits in the incoming value to what they should be according
    % to the decoded block and and face code
    checksum = binaryCode(checkBits);
       
    checksumTruth = false(1, numCheckBits);
    i_block1 = 1;
    for i_check = 1:numCheckBits
        
        i_block2 = mod(i_block1, numBlockBits) + 1;
        i_face = mod(i_check-1, numFaceBits) + 1;
        
        checksumTruth(i_check) = xor( binaryCode(faceBits(i_face)), ...
            xor(binaryCode(blockBits(i_block1)), binaryCode(blockBits(i_block2))) );
        
        i_block1 = mod(i_block1, numBlockBits) + 1;
    end

    isValid = all(checksum == checksumTruth);
end

end
