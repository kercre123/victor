function logQ = log(Q)

logQ = UnitQuaternion([0; Q.q(1)*Q.q(2:4)]);

end