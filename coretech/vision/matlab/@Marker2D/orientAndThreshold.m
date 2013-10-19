function [this, binaryString] = orientAndThreshold(this, means)

orientBits = [this.UpBit this.DownBit this.LeftBit this.RightBit];
[~,whichDir] = max(means(orientBits));

if isempty(this.threshold)
    bright = means(orientBits(whichDir));
    dark   = mean(means(orientBits([1:(whichDir-1) (whichDir+1):end])));
    
    if bright < this.MinContrastRatio*dark
        % not enough contrast to work with
        this.isValid = false;
        binaryString = '';
        return
    end
    
    this.threshold = (bright + dark)/2;
end

switch(whichDir)
    case 1 % 'up'
        this.reorderCorners = 1:4;
        this.upAngle =-pi/2;
    case 2 % 'down'
        means = rot90(rot90(means));
        this.reorderCorners = [4 3 2 1];
        this.upAngle = pi/2;
    case 3 % 'left'
        means = rot90(rot90(rot90(means)));
        this.reorderCorners = [2 4 1 3];
        this.upAngle = pi;
    case 4 % 'right'
        means = rot90(means);
        this.reorderCorners = [3 1 4 2];
        this.upAngle = 0;
    otherwise
        error('Unrecognized whichDir "%d"', whichDir);
end
            
this.corners = this.corners(this.reorderCorners,:);
this.upDirection = whichDir;

binaryString = strrep(num2str(row(means) < this.threshold), ' ', '');

end