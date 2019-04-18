/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * $Id: bb_dba.h,v 1.7 2005/07/01 07:18:46 pagel Exp $
 *
 * File name:           $RCSfile: bb_dba.h,v $
 * File description :   DBASPI INTERFACE
 * Module :             COMMON\DBASPI\
 * File location :      .\common\dbaspi\api\
 * Purpose:             
 * Author:              NM
 * History : 
 *            28/08/2001    Created
 *
 *
 * Copyright (c) 1998-2001 BABEL TECHNOLOGIES
 * All rights reserved.
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN
 * AGREEMENT OR ROYALTY FEES.
 *
 */


#ifndef __DBASPI__BB_DBA_H__
#define __DBASPI__BB_DBA_H__

#include "defines.h"
#include "bb_def.h"
#include "bb_types.h"
#include "bb_mem.h"

/**********************************************************************/
/*   DBASPI CONSTANTS DESCRIPTION                                     */
/**********************************************************************/

#include "ic_dba.h"


/**********************************************************************/
/*   DBASPI OBJECTS DESCRIPTION                                       */
/**********************************************************************/
#include "io_dba.h"


/**********************************************************************/
/*   DBASPI FUNCTIONS PROTOTYPES                                      */
/**********************************************************************/
#include "if_dba.h"

#define BB_Db_init_file(id,name) { id.link= name; id.type= X_FILE; id.flags=0; }

#endif /* __DBASPI__BB_DBA_H__ */
