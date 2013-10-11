function this = decodeIDs(this, binaryCode)
% Decode 16-bit code into faceType and blockType, plus validity flag.
%
% [blockType, faceType, isValid] = decodeIDs(code)
%
%
% See also: encodeIDs
% ------------
% Andrew Stein
%

binaryIDs = cell(1, this.numIDs);
for i = 1:this.numIDs
    binaryIDs{i} = binaryCode(this.IdBits(this.IdChars{i}));
    this.ids(i) = bin2dec(binaryIDs{i}) + 1;
    binaryIDs{i} = binaryIDs{i} == '1'; % make logical
end

if any(this.ids == 0)
    this.isValid = false;
else
    binaryCode = binaryCode == '1'; % make logical
    
    % Compare check bits in the incoming value to what they should be according
    % to the decoded block and and face code
    checksum = binaryCode(this.CheckBits);
    
    checksumTruth = this.computeChecksum(binaryIDs{:});
    
    this.isValid = all(checksum == checksumTruth);
end


end



