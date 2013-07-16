function Qi = LinearBlend(P, Q, t)
% DLB, Dual Linear Blending.  Generalization of Quaternion Linear Blending.

% Reference:
%  "Dual Quaternions for Rigid Transformation Blending," Kavan, 2006.
%  https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&ved=0CCwQFjAA&url=https%3A%2F%2Fbitbucket.org%2Fdbacchet%2Fdbsdev%2Fsrc%2Fe69aa94ff4d9740649162b426539c1fcd61ca8ee%2Fdocs%2Fpapers%2FKavan%2520-%25202006%2520-%2520Dual%2520Quaternions%2520for%2520Rigid%2520Transformation%2520Blending.pdf&ei=U2XlUd3aIay6yAHo54GYDg&usg=AFQjCNFLimH50cPMueOKc4BqTpVb5JWQTA&bvm=bv.48705608,d.aWc

assert(isa(P, 'DualQuaternion') && isa(Q, 'DualQuaternion') && ...
    isa(t, 'double') && isscalar(t) && t>=0 && t<=1, ...
    'Expecting two DualQuaternions and a scalar double, [0,1].');

Qi = (1-t)*P + t*Q;

end
