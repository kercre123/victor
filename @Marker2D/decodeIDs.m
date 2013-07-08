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

% TODO: This is for legacy support, i.e. for already-printed codes.  But I
% don't think I should need to reorder with EncodingBits in the future.
value = bin2dec(binaryCode(sort(this.EncodingBits))); % WHY DO I NEED THE SORT?!?
binaryCode = repmat('0', [1 numel(this.Layout)]);
binaryCode(this.EncodingBits) = dec2bin(value, length(this.EncodingBits));

for i = 1:this.numIDs
    this.ids(i) = bin2dec(binaryCode(this.IdBits(this.IdChars{i}))) + 1;
end

if any(this.ids == 0)
    this.isValid = false;
else
    binaryCode = binaryCode == '1'; % make logical
    
    % Compare check bits in the incoming value to what they should be according
    % to the decoded block and and face code
    checksum = binaryCode(this.CheckBits);
       
    checksumTruth = this.computeChecksum(binaryCode);
    
    this.isValid = all(checksum == checksumTruth);
end


end



