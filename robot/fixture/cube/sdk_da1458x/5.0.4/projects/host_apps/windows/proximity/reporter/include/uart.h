/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief Definitions for uart interface.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _UART_H_
#define _UART_H_

#define MAX_PACKET_LENGTH 350
#define MIN_PACKET_LENGTH 9
 /*
 ****************************************************************************************
 * @brief Write message to UART.
 *
 *  @param[in] size  Message's size.
 *  @param[in] data  Pointer to message's data.
 *
 * @return void.
 ****************************************************************************************
*/
void UARTSend(unsigned short size, unsigned char *data);
/*
 ****************************************************************************************
 * @brief Send message received from UART to application's main thread.
 *
 *  @param[in] length           Message's size.
 *  @param[in] bInputDataPtr    Pointer to message's data.
 *
 * @return void.
 ****************************************************************************************
*/
void SendToMain(unsigned short length, uint8_t *bInputDataPtr);
/*
 ****************************************************************************************
 * @brief UART Reception thread loop.
 *
 * @return void.
 ****************************************************************************************
*/
void UARTProc(PVOID unused);

/*
 ****************************************************************************************
 * @brief Init UART iface.
 *
 *  @param[in] Port         COM prot number.
 *  @param[in] BaudRate     Baud rate.
 *
 * @return -1 on failure / 0 on success.
 ****************************************************************************************
*/
uint8_t InitUART(int Port, int BaudRate);


#endif /* _UART_H_ */
