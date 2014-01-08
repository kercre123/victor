classdef MarkerCodeClassifier < handle
    
    methods(Access = 'public')
        
        function this = MarkerCodeClassifier(varargin)
            NumProbes = 8;
            NumBits   = 3;
            NumPerturbations = 100;
            StoreMean = true;
            
            parseVarargin(varargin{:});
            
            this.numProbes        = NumProbes;
            this.numBits          = NumBits;
            this.numPerturbations = NumPerturbations;
            this.storeMean        = StoreMean;
            
            %this.hashTable = containers.Map('KeyType', 'char', 'ValueType', 'char');
            
        end % CONSTRUCTOR MarkerCodeClassifier()
        
        function Train(this, trainingImages)
            numExamples = length(trainingImages);
            this.examples = cell(numExamples,1);
            this.labels   = cell(numExamples,1);
            
            for i_train = 1:numExamples
                [~,className,~] = fileparts(trainingImages{i_train});
                
                example = cell(this.numPerturbations,1);
                for i_perturb = 1:this.numPerturbations
                    code = createCodeFromImage(trainingImages{i_train}, [], this.numProbes, this.numBits);
                    %this.hashTable(char(code)) = className;
                    example{i_perturb} = row(single(code));
                end
                
                this.examples{i_train} = vertcat(example{:});
                
                if this.storeMean
                    this.examples{i_train} = mean(this.examples{i_train},1);
                    this.labels{i_train} = className;
                else
                    this.labels{i_train} = cell(1, this.numPerturbations);
                    [this.labels{i_train,:}] = deal(className);
                end
                
            end % FOR each trainingImage
            
            this.examples = vertcat(this.examples{:});
            this.labels   = this.labels(:);
                
        end % FUNCTION Train()
        
        function [matchedClass, matchDistance, d] = Classify(this, img, corners)
            code = createCodeFromImage(img, corners, this.numProbes, this.numBits);
            %matchedClass = this.hashTable(char(code));
            
            code = row(single(code));
            
            N = size(this.examples,1);
            d = sum(imabsdiff(code(ones(N,1),:), this.examples).^2,2);
                        
            %             d = zeros(1,N);
            %             for i = 1:N
            %                 d(i) = sum(column(imabsdiff(im2uint8(code), this.examples{i})).^2);
            %             end
            
            [matchDistance, matchIndex] = min(d);
            matchedClass = this.labels{matchIndex};
            matchDistance = sqrt(matchDistance);
            
        end % FUNCTION Classify()
        
    end % METHODS (Public)
    
    properties(SetAccess = 'protected')
        % Parameters
        numProbes;
        numBits;
        numPerturbations;
        storeMean;
        
        %hashTable;
        examples;
        labels;
        
    end % PROPERTIES (Protected)
    
end % CLASSDEF MarkerCodeClassifier