/**
 ****************************************************************************************
 *
 * @file arch_console.c
 *
 * @brief Serial logging interface.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include <stdlib.h>
 
#include "arch_console.h"
#include "arch_api.h"

#include "ke_mem.h"

#include "uart.h"

#if defined (CFG_PRINTF)

printf_msg *printf_msg_list __attribute__((section("retention_mem_area0"), zero_init));
bool defer_sending __attribute__((section("retention_mem_area0"), zero_init));

extern uint8_t func_check_mem_flag;

#define PRINT_SZ 256


/*
 * List management functions
 */

// pull one from the beginning
static printf_msg *remove_from_list(printf_msg **list)
{
	printf_msg *node = NULL;
	
	if (*list)
    {	
        node = *list;
        *list = node->pNext;
        node->pNext = NULL;
    }
	
	return node;
}
	
// push one at the end
static int put_to_list(printf_msg **list, printf_msg *node)
{
	printf_msg *p;
	
	if (!node)
		return 0;
	
	if (!(*list))
		*list = node;
	else {
		for (p = *list; p->pNext != NULL; p = p->pNext);
		
		p->pNext = node;
		node->pNext = NULL;
	}
    
	return 1;
}


/*
 * Message functions
 */
static printf_msg* create_msg(uint32_t sz)
{
    printf_msg *node = (printf_msg *)ke_malloc(sizeof(printf_msg), KE_MEM_NON_RETENTION);
    
    if (!node)
        return NULL;
    
    node->pBuf = (char *)ke_malloc(sz, KE_MEM_NON_RETENTION);
    if (!node->pBuf) {
        ke_free(node);
        return NULL;
    }
    node->pNext = NULL;
    
    return node;
}

int delete_msg(printf_msg *node)
{
    if (!node)
        return 0;

    if (!node->pBuf)
        __asm("BKPT #0\n");
    
    ke_free(node->pBuf);
    ke_free(node);
    
    return 1;
}


/*
 * String manipulation functions
 */
static uint32_t arch_strlen(const char *s)
{
    unsigned int length = 0;
    
    while (s[length] != '\0') 
        length++;
    
    return length;
}


static uint32_t arch_itoa(int32_t value, uint32_t radix, uint32_t uppercase,
                          char *buf, int32_t pad)
{
    char *pbuf = buf;
    bool negative = false;
    uint32_t i, length;

    // Check for unusual radixes
    if (radix > 16)
        return 0;

    if (value < 0) {
        negative = true;
        value = -value;
    }

    // Build the string here starting from the end...
    do {
        int digit = value % radix;
        *(pbuf++) = (digit < 10 ? '0' + digit : (uppercase ? 'A' : 'a') + digit - 10);
        value /= radix;
    } while (value > 0);

    length = (pbuf - buf);
    for (i = length; i < pad; i++)
        *(pbuf++) = '0';

    // Add '-' sign if need be...
    if (negative)
        *(pbuf++) = '-';

    // Mark EOF
    *(pbuf) = '\0';

    // Reverse the string now...
    length = (pbuf - buf);
    for (i = 0; i < length / 2; i++) {
        char ch = buf[i];
        
        buf[i] = buf[length-i-1];
        buf[length-i-1] = ch;
    }

    return length;
}


static inline int _putc(char **wr_ptr, char *buf, uint32_t length, char ch)
{
    if ((uint32_t)((*wr_ptr - buf) + 1) >= length)
        return 0;
    
    **wr_ptr = ch;
    (*wr_ptr)++;
    **wr_ptr = '\0';
    
    return 1;
}
    

static inline int _puts(char **wr_ptr, char *buf, uint32_t length, char *s, uint32_t str_len)
{
    uint32_t i;

    if (length - (*wr_ptr - buf) - 1 < str_len)
        str_len = length - (*wr_ptr - buf) - 1;

    /* Copy to buffer */
    for (i = 0; i < str_len; i++) {
        **wr_ptr = s[i];
        (*wr_ptr)++;
    }
    **wr_ptr = '\0';

    return length;
}


static int32_t arch_vsnprintf(char *buffer, uint32_t buffer_len, const char *fmt, va_list va)
{
    char *pbuffer = buffer;
    char bf[24];
    char ch;
    char qual = '\0';

    ch = *(fmt++);
    do {
        if ((unsigned int)((pbuffer - buffer) + 1) >= buffer_len)
            break;
        if (ch!='%')
            _putc(&pbuffer, buffer, buffer_len, ch);
        else {
            char zero_pad = 0;
            char *ptr;
            unsigned int len;

            ch=*(fmt++);

            /* Zero padding requested */
            if (ch=='0') {
                ch=*(fmt++);
                if (ch == '\0')
                    goto end;
                if (ch >= '0' && ch <= '9')
                    zero_pad = ch - '0';
                ch=*(fmt++);
            }

            if(ch == 'l' || ch == 'h') {
               qual = ch;
                ch=*(fmt++);
            }
            
            switch (ch) {
            case 0:
                goto end;

            case 'i':
            case 'd':
            {
                long val;

                if (qual == 'h') {
                    val = va_arg(va, int/*short*/);
                } else if (qual == 'l') {
                    val = va_arg(va, long);
                } else {
                    val = va_arg(va, int);
                }
                len = arch_itoa(val, 10, 0, bf, zero_pad);
                _puts(&pbuffer, buffer, buffer_len, bf, len);
                break;
            }
            case 'u':
            case 'x':
            case 'X':
            {
                unsigned long val;
                
                if(qual == 'h') {
                    val = va_arg(va, unsigned int/*unsigned short*/);
                } else if(qual == 'l') {
                    val = va_arg(va, unsigned long);
                } else {
                    val = va_arg(va, unsigned int);
                }
                if (ch == 'u')
                    len = arch_itoa(val, 10, 0, bf, zero_pad);
                else
                    len = arch_itoa(val, 16, (ch=='X'), bf, zero_pad);
                _puts(&pbuffer, buffer, buffer_len, bf, len);
                break;
            }

            case 'c' :
                ch = (va_arg(va, int32_t));
                _putc(&pbuffer, buffer, buffer_len, ch);
                break;

            case 's' :
                ptr = va_arg(va, char*);
                _puts(&pbuffer, buffer, buffer_len, ptr, arch_strlen(ptr));
                break;

            default:
                _puts(&pbuffer, buffer, buffer_len, "FATAL: unsupported printf character: ", 37);
                _putc(&pbuffer, buffer, buffer_len, ch);
                _puts(&pbuffer, buffer, buffer_len, ".\n", 2);
                break;
            }
        }
        ch = *(fmt++);
    } while(ch);
end:
    return pbuffer - buffer;
}


static int32_t arch_snprintf(char* buffer, uint32_t buffer_len, const char *fmt, va_list va)
{
    int ret;

    ret = arch_vsnprintf(buffer, buffer_len, fmt, va);

    return ret;
}

/* Note: App should not modify the sleep mode until all messages have been printed out */
static void uart_callback(uint8_t res)
{
    printf_msg* msg = remove_from_list(&printf_msg_list);
    delete_msg(msg);

#ifdef CFG_PRINTF_UART2    
    uart2_finish_transfers();
#else
    uart_finish_transfers();
#endif    
    
    if (printf_msg_list) {
        char *p = printf_msg_list->pBuf;
#ifdef CFG_PRINTF_UART2            
        uart2_write((uint8_t *)p, arch_strlen(p), uart_callback);
#else
        uart_write((uint8_t *)p, arch_strlen(p), uart_callback);        
#endif
        
    } else
        arch_restore_sleep_mode();
}

int arch_printf(const char *fmt, ...)
{
    va_list args;
    char my_buf[PRINT_SZ];
    printf_msg* msg;
    uint32_t len;
    
    // Check if heap is ready
    if (func_check_mem_flag) {
        // Heap is not ready => there's no memory to store the msg
        // The msg is discarded!
        return 0;
    }

    va_start(args, fmt);
    arch_snprintf(my_buf, sizeof(my_buf), fmt, args);
    va_end(args);

    len = arch_strlen(my_buf);
    msg = create_msg(len + 1);
    if (msg) {
        strcpy(msg->pBuf, my_buf);
        
        // Critical section
        GLOBAL_INT_DISABLE();
        
        // if printf_msg_list is NULL we can send the msg immediately (in the synchrouns part of the loop), else 
        // the callback function will handle it
        if (printf_msg_list == NULL)
            defer_sending = true;
        
        if (!put_to_list(&printf_msg_list, msg)) {
            delete_msg(msg);
            if (printf_msg_list == NULL)
                defer_sending = false;
        } else {
            //send to UART only if XTAL16 is used else it crashes: defer_sending will trigger the procedure!
        }
        // End of critical section
        GLOBAL_INT_RESTORE();
    }

    return 1;
}


void arch_puts(const char *s)
{
    arch_printf("%s", s);
}

void arch_printf_process(void)
{
    if (defer_sending) {
        arch_force_active_mode();
#ifdef CFG_PRINTF_UART2         
        uart2_write((uint8_t *)printf_msg_list->pBuf, arch_strlen(printf_msg_list->pBuf), uart_callback);
#else
        uart_write((uint8_t *)printf_msg_list->pBuf, arch_strlen(printf_msg_list->pBuf), uart_callback);        
#endif        
        defer_sending = false;
    }
}
#endif // CFG_PRINTF
