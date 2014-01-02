#ifndef	_CTYPE_H
#define	_CTYPE_H

#define isdigit(x) ((x) >= '0' && (x) <= '9')
#define isspace(x) ((x)==' ' || (x)=='\t' || (x)=='\v' || (x)=='\f' || (x)=='\r' || (x)=='\n')
#define isupper(x) ((x) >='A' && (x) <='Z')
#define islower(x) ((x) >='a' && (x) <='z')
#define isalpha(x) (isupper(x) || islower(x))
#define isascii(x) ( (x) > 0 && (x) < 127 )

#endif //_CTYPE_H
