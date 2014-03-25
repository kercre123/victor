
#ifndef BLOCK_DEFINITION_MODE

#define BLOCK_ENUM_MODE 0
#define BLOCK_LUT_MODE  1

#define START_BLOCK_DEFINITION(__NAME__, __SIZE__, __COLOR__)
//#define SET_COLOR(_R_, _G_, _B_)
//#define SET_SIZE(_X_, _Y_, _Z_)
#define ADD_FACE_CODE(__WHICHFACE__, __SIZE__, __CODE__)
#define END_BLOCK_DEFINITION

#else

#undef START_BLOCK_DEFINITION
//#undef SET_COLOR
//#undef SET_SIZE
#undef ADD_FACE_CODE
//#undef SET_FACE_SIZE
#undef END_BLOCK_DEFINITION

#if BLOCK_DEFINITION_MODE == BLOCK_ENUM_MODE
#define START_BLOCK_DEFINITION(__NAME__, __SIZE__, __COLOR__) __NAME__##_BLOCK_TYPE,
//#define SET_COLOR(_R_, _G_, _B_)
//#define SET_SIZE(_X_, _Y_, _Z_)
#define ADD_FACE_CODE(__WHICHFACE__, __SIZE__, __CODE__)
//#define SET_FACE_SIZE(__FACE_NUM__, __SIZE__)
#define END_BLOCK_DEFINITION

#elif BLOCK_DEFINITION_MODE == BLOCK_LUT_MODE
#define UNWRAP(...) __VA_ARGS__
//#define QUOTE_HELPER(__ARG__) #__ARG__
//#define QUOTE(__ARG__) QUOTE_HELPER(__ARG__)
#define START_BLOCK_DEFINITION(__NAME__, __SIZE__, __COLOR__) \
,{.name = QUOTE(__NAME__), .color = {UNWRAP __COLOR__}, .size = {UNWRAP __SIZE__}, .faces = {

#define ADD_FACE_CODE(__WHICHFACE__, __SIZE__, __CODE__) \
{.whichFace = __WHICHFACE__, .size = __SIZE__, .code = __CODE__},

//#define SET_COLOR(_R_, _G_, _B_)                       ,.color = {_R_,_G_,_B_}
//#define SET_SIZE(_X_, _Y_, _Z_)                        ,.size = {_X_,_Y_,_Z_}
//#define ADD_FACE_CODE(__WHICHFACE__, __SIZE__, ...)    ,.faces[__WHICHFACE__] = {.size = __SIZE__, .code = {__VA_ARGS__}}

#define END_BLOCK_DEFINITION  } }

#else
#error Unknown BLOCK_DEFINITION_MODE!

#endif // #if/elif BLOCK_DEFINITION_MODE == <which_mode>

#undef BLOCK_DEFINITION_MODE

#endif // #ifndef BLOCK_DEFINTION_MODE
