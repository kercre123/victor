function out = mpower(Q,y)

assert(isa(Q, 'Quaternion') && isscalar(y) && isa(y, 'double'), ...
    'Expecting a Quaternion raised to a scalar double power.');

out = exp(y*log(Q));

end