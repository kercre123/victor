function [this, binaryString] = orientAndThreshold(this, means)

orientBits = [this.UpBit this.DownBit this.LeftBit this.RightBit];
[~,whichDir] = max(means(orientBits));

if isempty(this.threshold)
    bright = means(orientBits(whichDir));
    dark   = mean(means(orientBits([1:(whichDir-1) (whichDir+1):end])));
    
    if bright < this.MinContrastRatio*dark
        % not enough contrast to work with
        this.isValid = false;
        return
    end
    
    this.threshold = (bright + dark)/2;
end

dirs = {'up', 'down', 'left', 'right'};
switch(dirs{whichDir})
    case 'up'
        reorder = 1:4;
    case 'down'
        means = rot90(rot90(means));
        reorder = [3 1 4 2];
    case 'left'
        means = rot90(rot90(rot90(means)));
        reorder = [2 4 1 3];
    case 'right'
        means = rot90(means);
        reorder = [4 3 2 1];
    otherwise
        error('Unrecognized topOrient "%s"', keyOrient);
end
            
this.corners = this.corners(reorder,:);

binaryString = strrep(num2str(row(means) < this.threshold), ' ', '');

end