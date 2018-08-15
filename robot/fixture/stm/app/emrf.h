#ifndef EMRF_H
#define EMRF_H
//Description: definitions for 'fixture' region of the EMR (birth certificate)

#include <stdint.h>

#ifdef __cplusplus

#include "../../../include/anki/cozmo/shared/factory/emr.h" //common emr structure

//#define EMR_FIELD_OFS(fieldname)  offsetof(Anki::Vector::Factory::EMR::Fields, fieldname)/sizeof(uint32_t)
#define EMR_FIELD_OFS(fieldname) ( \
  ((uint32_t)&((Anki::Vector::Factory::EMR*)0)->fields.fieldname) / sizeof(uint32_t) \
  - ((uint32_t)&((Anki::Vector::Factory::EMR*)0)->data[0]) / sizeof(uint32_t)  \
)

#endif //__cplusplus
#endif //EMRF_H

