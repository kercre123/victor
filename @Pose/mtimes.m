function P3 = mtimes(P1, P2)

T1 = [P1.Rmat P1.T; zeros(1,3) 1];
T2 = [P2.Rmat P2.T; zeros(1,3) 1];

T3 = T1*T2;

P3 = Pose(T3(1:3,1:3), T3(1:3,4));

end