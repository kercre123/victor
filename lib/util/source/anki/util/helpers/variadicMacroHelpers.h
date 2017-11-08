/**
 * File: variadicMacroHelpers.h
 *
 * Author:  Matt Michini
 * Created: 11/7/2017
 *
 * Description: Helpers for counting number of arguments in variadic macros and
 *              doing things individually to the arguments.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef UTIL_VARIADIC_MACRO_HELPERS_H_
#define UTIL_VARIADIC_MACRO_HELPERS_H_

// Many of these helpers are derived from https://stackoverflow.com/questions/6707148/foreach-macro-on-macros-arguments#6707531

// The PP_NARG macro evaluates to the number of arguments that have been
// passed to it (courtesy of Laurent Deniau)

#define PP_NARG(...)    PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...)   PP_ARG_N(__VA_ARGS__)

#define PP_ARG_N( \
        _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,  \
        _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
        _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
        _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
        _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
        _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
        _61,_62,_63,N,...) N

#define PP_RSEQ_N() \
        63,62,61,60,                   \
        59,58,57,56,55,54,53,52,51,50, \
        49,48,47,46,45,44,43,42,41,40, \
        39,38,37,36,35,34,33,32,31,30, \
        29,28,27,26,25,24,23,22,21,20, \
        19,18,17,16,15,14,13,12,11,10, \
        9,8,7,6,5,4,3,2,1,0

// Concatenation macros. PP_XPASTE uses an extra level to force another evaluation
#define PP_PASTE(A,B)    A ## B
#define PP_XPASTE(A,B)   PP_PASTE(A,B)

// Macros to return a comma-separated list of each argument as a string.
// e.g. PP_STRINGIZE_2("string1", "string2") expands to "string1", "string2".
// This list stops at 15 but could be extended to 64 (PPNARG's limit).
#define  PP_STRINGIZE_1(a) #a
#define  PP_STRINGIZE_2(a, b) PP_STRINGIZE_1(a),  PP_STRINGIZE_1(b)
#define  PP_STRINGIZE_3(a, b, c) PP_STRINGIZE_1(a),  PP_STRINGIZE_2(b, c)
#define  PP_STRINGIZE_4(a, b, c, d) PP_STRINGIZE_1(a),  PP_STRINGIZE_3(b, c, d)
#define  PP_STRINGIZE_5(a, b, c, d, e) PP_STRINGIZE_1(a),  PP_STRINGIZE_4(b, c, d, e)
#define  PP_STRINGIZE_6(a, b, c, d, e, f) PP_STRINGIZE_1(a),  PP_STRINGIZE_5(b, c, d, e, f)
#define  PP_STRINGIZE_7(a, b, c, d, e, f, g) PP_STRINGIZE_1(a),  PP_STRINGIZE_6(b, c, d, e, f, g)
#define  PP_STRINGIZE_8(a, b, c, d, e, f, g, h) PP_STRINGIZE_1(a),  PP_STRINGIZE_7(b, c, d, e, f, g, h)
#define  PP_STRINGIZE_9(a, b, c, d, e, f, g, h, i) PP_STRINGIZE_1(a),  PP_STRINGIZE_8(b, c, d, e, f, g, h, i)
#define  PP_STRINGIZE_10(a, b, c, d, e, f, g, h, i, j) PP_STRINGIZE_1(a),  PP_STRINGIZE_9(b, c, d, e, f, g, h, i, j)
#define  PP_STRINGIZE_11(a, b, c, d, e, f, g, h, i, j, k) PP_STRINGIZE_1(a), PP_STRINGIZE_10(b, c, d, e, f, g, h, i, j, k)
#define  PP_STRINGIZE_12(a, b, c, d, e, f, g, h, i, j, k, l) PP_STRINGIZE_1(a), PP_STRINGIZE_11(b, c, d, e, f, g, h, i, j, k, l)

#define PP_STRINGIZE_N(M, ...) M(__VA_ARGS__)
#define PP_STRINGIZE_X(...) PP_STRINGIZE_N(PP_XPASTE(PP_STRINGIZE_, PP_NARG(__VA_ARGS__)), __VA_ARGS__)


#endif // UTIL_VARIADIC_MACRO_HELPERS_H_
