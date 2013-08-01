function value = encodeIDs(blockType, faceType, layout)
% Encode faceType and blockType into a code with checksum.
%
% value = encodeIDs(blockType, faceType)
%
% See also: decodeIDs
% ------------
% Andrew Stein
%

assert(blockType > 0, 'blockType must be > 0.');
assert(faceType > 0,  'faceType must be > 0.');

% Bits are distributed so that all checksum bits aren't all at end.
%   Here's the key: 
%    [B = block bit, F = face bit, C = checksum bit, 
%     O = orientation bit, X = center]
%
%     B C O F B
%     F B B B C
%     O-C-X-C-O
%     F B C F C
%     B C O C B
% [Does this really matter though?]

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

encodingBits = [blockBits; faceBits; checkBits];
totalBits = length(layout);

assert(blockType <= 2^numBlockBits, 'blockType too large');
assert(faceType <= 2^numFaceBits, 'faceType too large');

binaryCode = false(1,length(layout));

binaryCode(blockBits) = dec2bin(blockType-1, numBlockBits) == '1';
binaryCode(faceBits)  = dec2bin(faceType-1,  numFaceBits) == '1';

% Compute check bits:
assert(numCheckBits > numFaceBits, ...
    'Number of check bits should be greater than number of faces bits.');
assert(2*numCheckBits > numBlockBits, ...
    'Twice number of check bits should be greater than number of block bits.');
used = false(1, totalBits);
i_block1 = 1;
for i_check = 1:numCheckBits
    
    %i_block1 = mod(2*(i_check-1), numBlockBits) + 1;
    %i_block2 = mod(2*(i_check-1)+1, numBlockBits) + 1;
    
    i_block2 = mod(i_block1, numBlockBits) + 1;
    i_face = mod(i_check-1, numFaceBits) + 1;
    
    %fprintf('C%d = xor(xor(B%d,B%d), F%d)\n', i_check, i_block1, i_block2, i_face);
    
    binaryCode(checkBits(i_check)) = xor(binaryCode(faceBits(i_face)), ...
        xor(binaryCode(blockBits(i_block1)), binaryCode(blockBits(i_block2))) );
    
    used([blockBits(i_block1) blockBits(i_block2) faceBits(i_face)]) = true;
    
    i_block1 = mod(i_block1, numBlockBits) + 1;
end
assert(all(used([blockBits;faceBits])), 'Not all the face/block bits were involved in checksum.');

binaryCode = binaryCode(encodingBits);
value = bin2dec(strrep(num2str(binaryCode),' ', ''));

end


