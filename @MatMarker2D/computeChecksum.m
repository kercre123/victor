function checksum = computeChecksum(binX, binY)

bits = [binX(:); binY(:)];
assert(numel(bits) == 16, 'Expecting 16 total bits for X and Y encoding.');
lastBit = bits(end);
bits = reshape(bits(1:15), [3 5]);

checksum = xor(xor(bits(1,:), bits(2,:)), bits(3,:));
checksum(end) = xor(checksum(end), lastBit);

end