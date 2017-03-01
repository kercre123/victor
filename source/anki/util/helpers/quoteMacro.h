/**
 * File: quoteMacro.h
 *
 * Author:  Kevin Yoon
 * Created: 12/13/2016
 *
 * Description: Macro for generating quoted form of whatever is passed in
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef UTIL_QUOTE_MACRO_H_
#define UTIL_QUOTE_MACRO_H_

#define QUOTE_HELPER(__ARG__) #__ARG__
#define QUOTE(__ARG__) QUOTE_HELPER(__ARG__)

#endif // UTIL_QUOTE_MACRO_H_
