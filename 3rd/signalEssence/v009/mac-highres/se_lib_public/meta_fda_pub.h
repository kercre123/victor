#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   (C) Copyright 2018 SignalEssence; All Rights Reserved

    Module Name: meta_fda

    Author: Robert Yu

    Author: Robert Yu

    Description:
    Meta interface for freq domain analyzers

    Machine/Compiler:
    (ANSI C)
**************************************************************************/
#ifndef META_FDA_PUB_H_
#define META_FDA_PUB_H_

typedef enum
{
    META_FDA_FIRST_DONT_USE,
    META_FDA_IS127,
    META_FDA_WOLA,
    META_FDA_LAST
} MetaFdaImplementation_t;

#endif // META_FDA_PUB_H

#ifdef __cplusplus
}
#endif

