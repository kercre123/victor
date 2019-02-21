# Poor man's "help" to show all the arguments in the faceEval code
 grep GetArg\(\"-- faceEval* | perl -nle 'print /(\w*\.(?:h|cpp)|\s|--\w*)/g'