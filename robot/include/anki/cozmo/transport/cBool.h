/**
 * @file cBool
 * @author Daniel Casner <daniel@anki.com>
 * 
 * C (not C++) definition of bool type with garunteed size.
 *
 **/
#ifndef __Embedded_cBool_H__
#define __Embedded_cBool_H__

typedef uint8_t bool;
static const bool false = 0;
static const bool true = 1;

#endif // __Embedded_cBool_H__
