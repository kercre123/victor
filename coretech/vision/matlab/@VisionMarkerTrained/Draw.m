function h = Draw(this, varargin)

Color = 'r';
TopColor = 'g';
Tag = class(this);
Parent = [];
TextColor = 'y';
FontSize = 20;
String = strrep(this.name, 'MARKER_', ''); % the code name without prefix 'MARKER_'

parseVarargin(varargin{:});

if isempty(Parent)
    Parent = gca;
end

if isempty(this.pose)
    h = plot(this.corners([1 2 4 3 1],1), this.corners([1 2 4 3 1],2), Color, ...
        'LineWidth', 3, 'Parent', Parent, 'Tag', Tag);
    
    if ~isempty(TopColor)
        h(end+1) = plot(this.corners([1 3],1), this.corners([1 3],2), TopColor, ...
            'LineWidth', 3, 'Parent', Parent, 'Tag', Tag);
    end
    
    h_text = [];
    if ~isempty(String)
        cen = mean(this.corners,1);
        %diagonal = sqrt(max(sum((corners(1,:)-corners(4,:)).^2), sum((corners(2,:)-corners(3,:)).^2)));
        
        h_shadow = text(cen(1), cen(2), String, 'Hor', 'c', 'Back', 'none', ...
            'Color', 'k', ...
            'Parent', Parent, 'FontSize', FontSize, 'FontWeight', 'b', ...
            'FontName', 'DriveSans', 'Interpreter', 'none', 'Tag', Tag);
        
        h_text = copyobj(h_shadow, Parent);
        
        set(h_text, 'Color', TextColor, 'Pos', [cen-1.5 0]);
    end
else
    P = this.GetPosition('World');
    X = P([1 3 4 2], 1);
    Y = P([1 3 4 2], 2);
    Z = P([1 3 4 2], 3);

    h = patch(X,Y,Z, 'w', 'EdgeColor', 'k', 'Parent', Parent);
    
    leaveHoldOff = false;
    if ~ishold(Parent)
        hold(Parent, 'on')
        leaveHoldOff = true;
    end
    
    h_text = text(mean(X), mean(Y), mean(Z), ...
        this.name, 'Horizontal', 'center', 'Parent', Parent, ...
        'FontSize', FontSize, 'FontWeight', 'b', ...
        'FontName', 'DriveSans');
    
    if leaveHoldOff
        hold(Parent, 'off')
    end
    
end

h = [h(:); h_text(:)];

end