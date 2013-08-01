classdef Quaternion
    
    properties
        q = zeros(4,1);
    end
    
    properties(Dependent = true)
        a,b,c,d;
        r, v;
    end
    
    methods(Access = public)
        function Q = Quaternion(q_in)
            
            if isa(q_in, 'Quaternion')
                q_in = q_in.q;
            end
            
            assert(isvector(q_in) && length(q_in)==4, ...
                    'Input should be a 4-element vector.');
                        
            Q.q = q_in(:);
            
        end % CONSTRUCTOR Quaternion()
        
        function Qconj = conj(Q)
            Qconj = Quaternion([Q.q(1); -Q.q(2:4)]);
        end
        
        function n = norm(Q)
            n = norm(Q.q);
        end
            
    end
    
    methods
        function a = get.a(this)
            a = this.q(1);
        end
        function b = get.b(this)
            b = this.q(2);
        end
        function c = get.c(this)
            c = this.q(3);
        end
        function d = get.d(this)
            d = this.q(4);
        end
        function r = get.r(this)
            r = this.q(1);
        end
        function v = get.v(this)
            v = this.q(2:4);
        end
    end
    
end % CLASSDEF Quaternion