function Update(this)

% Compute how many bytes we need to read to get the buffer up to the
% current desired size
numBytesToRead = this.desiredBufferSize - length(this.serialBuffer);

% If the number of bytes available is less than this amount, wait for a
% future call when we have enough
if this.serialDevice.BytesAvailable < numBytesToRead
    pause(.01)
    return;
end

newData = row(fread(this.serialDevice, [1 numBytesToRead], 'uint8'));

this.serialBuffer = [this.serialBuffer uint8(newData)];

this.StatusMessage(3, 'Received %d bytes. Buffer now %d bytes long.\n', ...
    length(newData), length(this.serialBuffer));

% Find the headers and footers
headerIndex = strfind(this.serialBuffer, this.HEADER);
footerIndex = strfind(this.serialBuffer, this.FOOTER);

% Process all the packets in the serial buffer
i_footer = 1;
i_header = 1;
while i_header <= length(headerIndex) && i_footer <= length(footerIndex) && ...
        length(this.serialBuffer) >= headerIndex(i_header) + length(this.HEADER)
    
    headerSuffix = this.serialBuffer(headerIndex(i_header) + length(this.HEADER));
    
    % Find the next footer that is after this header
    while i_footer <= length(footerIndex) && ...
            footerIndex(i_footer) < headerIndex(i_header)
        
        i_footer = i_footer + 1;
    end
    
    if i_footer <= length(footerIndex) && length(this.serialBuffer) >= footerIndex(i_footer)+length(this.FOOTER)
        
        footerSuffix = this.serialBuffer(footerIndex(i_footer) + length(this.FOOTER));
        if headerSuffix == footerSuffix
            % If header and footer must have same suffix, this must
            % be a valid packet.  Process it.
            packetStart = headerIndex(i_header) + length(this.HEADER) + 1;
            packetEnd   = footerIndex(i_footer) - 1;
            packet = this.serialBuffer(packetStart:packetEnd);
            
            this.ProcessPacket(headerSuffix, packet);
            
        else
            % If they don't match, we've got garbage between this
            % header/footer pair, so just move on to the next
            % header...
            
        end
    end
    
    i_header = i_header + 1;
end % FOR each header

% Kill what's in the buffer, whether we processed it above or
% ignored it as garbage.
if ~isempty(headerIndex) 
    
    if isempty(footerIndex) && length(headerIndex)==1 && headerIndex==1
        % We've got a full buffer, with a header right at the beginning.
        % We have to increase the desired buffer size or else we'll never
        % see the footer.
        
        % TODO: is this a reasonable thing to do?  What if we make the
        % desired size *too* large and then we are stuck waiting forever
        % for bytes that will never come from the robot?
        this.desiredBufferSize = this.desiredBufferSize * 2;

    else
        if headerIndex(end) > footerIndex(end)
            % If we've got a final header that had no footer after it,
            % we potentially have a partial message.  Keep that in the
            % buffer, to hopefully have its remainder appended on the
            % next Update.
            this.serialBuffer(1:(headerIndex(end)-1)) = [];
        else
            % Get rid of everythign up to the last footer, which we
            % presumably processed above
            this.serialBuffer(1:footerIndex(end)+length(this.FOOTER)) = [];
        end
    end
end

end % FUNCTION: Update()