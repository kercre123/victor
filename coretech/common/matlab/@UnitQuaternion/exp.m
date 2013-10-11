function expQ = exp(Q)

assert(Q.q(1) == 0, ...
    'Can only exponentiate a UnitQuaternion whose scalar part is 0.');

theta = norm(Q.q(2:4));
v     = Q.q(2:4)/theta;

expQ = UnitQuaternion([cos(theta); sin(theta)*v]);

end