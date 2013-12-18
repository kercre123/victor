function Update(this)

% Read whatever is available in the serial port:
numBytes = this.serialDevice.BytesAvailable;
if numBytes <= 0
    return;
end

newData = row(fread(this.serialDevice, [1 numBytes], 'uint8'));

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
if isempty(headerIndex)
    this.serialBuffer = [];
    
else
    if isempty(footerIndex) || footerIndex(end) < headerIndex(end)
        this.serialBuffer(1:(headerIndex(end)-1)) = [];
        
    else
        this.serialBuffer = [];
    end
end
        
end % FUNCTION: Update()