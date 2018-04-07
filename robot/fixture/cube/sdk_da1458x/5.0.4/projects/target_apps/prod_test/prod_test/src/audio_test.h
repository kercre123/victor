 /**
 ****************************************************************************************
 *
 * @file audio_test.h
 *
 * @brief Audio 439 test.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
#ifndef AUDIO_TEST_H_
    #define AUDIO_TEST_H_
    
    #include <stdbool.h>
    #include <stdint.h>
    #include "app_audio439.h"
    #include "customer_prod.h"
    #include "ke_msg.h"
    
void audio_init(void);
void audio_send(int16_t *buffer);
size_t audio_start(struct msg_audio_test_cfm* msg, uint16_t packets);
size_t audio_stop(struct msg_audio_test_cfm* msg);

#endif // AUDIO_TEST_H_
