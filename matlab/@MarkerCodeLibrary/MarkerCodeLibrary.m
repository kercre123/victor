classdef MarkerCodeLibrary < handle
    
    properties
        codes;
    end
    
    methods
        function this = MarkerCodeLibrary()
            this.codes = {};
        end % CONSTRUCTOR MarkerCodeLibrary()
        
        function add(this, code, varargin)
            if ~iscell(code)
                code = {code};
            end
            
            for i_code = 1:length(code)
                
                if ~isa(code{i_code}, 'MarkerCode')
                    this.codes{end+1} = MarkerCode(code{i_code}, varargin{:});
                else
                    this.codes{end+1} = code{i_code};
                end
            end % FOR each code provided
            
            % Update the 
        end
        
        function [match, maxSimilarity, similarityDistribution] = findMatch(this, code, minSimilarity)
           maxSimilarity = minSimilarity;
           index   = 0;
           
           similarityDistribution = zeros(1, length(this.codes));
           for i = 1:length(this.codes)
               currentSim = this.codes{i}.compare(code);
               similarityDistribution(i) = currentSim;
               if currentSim > maxSimilarity
                   maxSimilarity = currentSim;
                   index = i;
               end
           end % FOR each code in the library
           
           if index == 0
               match = [];
           else
               match = this.codes{index};
           end
        end % FUNCTION findMatch()
        
    end % METHODS
    
end % CLASSDEF MarkerCodeLibrary