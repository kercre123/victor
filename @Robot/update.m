function update(this, img)

if nargin < 2 || isempty(img)
    this.camera.grabFrame();
else
    this.camera.image = img;
end

this.observationWindow(2:end) = this.observationWindow(1:end-1);
this.observationWindow{1} = Observation(this.camera.image, this);
% this.frame = this.observationWindow{1}.frame;

end % FUNCTION Robot/update()