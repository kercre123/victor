#ifndef EMRF_H
#define EMRF_H
//Description: definitions for 'fixture' region of the EMR (birth certificate)

#include <stdint.h>

#ifdef __cplusplus

#include "../../../include/anki/cozmo/shared/factory/emr.h" //common emr structure

#define EMR_FIELD_OFS(fieldname)  \
  (uint32_t)&((Anki::Cozmo::Factory::EMR*)0)->field.fieldname - \
  (uint32_t)&((Anki::Cozmo::Factory::EMR*)0)->data[0]

#endif //__cplusplus
#endif //EMRF_H

