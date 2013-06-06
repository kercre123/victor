function rotateAndTranslate(this, position, orientation)

R = orientation;
if isvector(R)
    R = rodrigues(R);
end

this.X = R(1,1)*this.modelX + R(1,2)*this.modelY + R(1,3)*this.modelZ + position(1);
this.Y = R(2,1)*this.modelX + R(2,2)*this.modelY + R(2,3)*this.modelZ + position(2);
this.Z = R(3,1)*this.modelX + R(3,2)*this.modelY + R(3,3)*this.modelZ + position(3);

end