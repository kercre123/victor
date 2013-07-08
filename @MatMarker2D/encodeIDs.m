function binaryCode = encodeIDs(Xpos, Ypos)

binaryCode = false(size(MatMarker2D.Layout));
binaryCode(MatMarker2D.UpBit) = false;
binaryCode([MatMarker2D.LeftBit MatMarker2D.RightBit ...
    MatMarker2D.DownBit]) = true;

Xbits = MatMarker2D.IdBits('X');
Ybits = MatMarker2D.IdBits('Y');
binaryCode(Xbits) = dec2bin(Xpos-1, length(Xbits)) == '1';
binaryCode(Ybits) = dec2bin(Ypos-1, length(Ybits)) == '1';
binaryCode(MatMarker2D.CheckBits) = MatMarker2D.computeChecksum(binaryCode);

% This keeps things compatible with old way of producing (Block) codes (which have
% already been printed.  Not sure re-ordering by EncodingBits is strictly
% necessary though.
% TODO: remove this.
binaryCode(sort(MatMarker2D.EncodingBits)) = binaryCode(MatMarker2D.EncodingBits);

end