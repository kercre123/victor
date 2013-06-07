function F3 = mtimes(F1, F2)

T1 = [F1.Rmat F1.T; zeros(1,3) 1];
T2 = [F2.Rmat F2.T; zeros(1,3) 1];

T3 = T1*T2;

F3 = Frame(T3(1:3,1:3), T3(1:3,4));

end