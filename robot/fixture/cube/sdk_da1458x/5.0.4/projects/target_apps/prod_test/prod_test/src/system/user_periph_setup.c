/**
 ****************************************************************************************
 *
 * @file periph_setup.c
 *
 * @brief Peripherals setup and initialization. 
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
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"       // peripheral configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization
#if HAS_AUDIO
    #include "app_audio439.h"
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

/****************************************************************************************/
/* UART pin configuration                                                               */
/****************************************************************************************/
uint8_t dummy1  	__attribute__((section("non_init"), zero_init));
uint8_t dummy2  	__attribute__((section("non_init"), zero_init));
uint8_t port_sel    __attribute__((section("non_init"), zero_init));

_uart_sel_pins uart_sel_pins;
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void init_TXEN_RXEN_irqs(void);
void update_uart_pads(GPIO_PORT port, GPIO_PIN tx_pin, GPIO_PIN rx_pin);

#if DEVELOPMENT_DEBUG

/**
 ****************************************************************************************
 * @brief GPIO_reservations. Globally reserved GPIOs
 *
 * @return void 
 ****************************************************************************************
*/
void GPIO_reservations(void)
{
		dummy1 = dummy2 = 0; // Give values so not to be stripped by -O3 optimization.
#ifdef CONFIG_UART_GPIOS
		
		switch (port_sel)
    {
				case 0:
						RESERVE_GPIO(UART1_TX, GPIO_PORT_0, GPIO_PIN_0, PID_UART1_TX);
						RESERVE_GPIO(UART1_RX, GPIO_PORT_0, GPIO_PIN_1, PID_UART1_RX);
						break;
        case 2:
						RESERVE_GPIO(UART1_TX, GPIO_PORT_0, GPIO_PIN_2, PID_UART1_TX);
						RESERVE_GPIO(UART1_RX, GPIO_PORT_0, GPIO_PIN_3, PID_UART1_RX);
						break;
				case 4:
						RESERVE_GPIO(UART1_TX, GPIO_PORT_0, GPIO_PIN_4, PID_UART1_TX);
						RESERVE_GPIO(UART1_RX, GPIO_PORT_0, GPIO_PIN_5, PID_UART1_RX);
						break;
        case 6:
						RESERVE_GPIO(UART1_TX, GPIO_PORT_0, GPIO_PIN_6, PID_UART1_TX);
						RESERVE_GPIO(UART1_RX, GPIO_PORT_0, GPIO_PIN_7, PID_UART1_RX);
            break;
        default:
						RESERVE_GPIO(UART1_TX, GPIO_PORT_0, GPIO_PIN_4, PID_UART1_TX);
						RESERVE_GPIO(UART1_RX, GPIO_PORT_0, GPIO_PIN_5, PID_UART1_RX);
            break;
    }

#else	
		port_sel = 4; // TX-GPIO_PIN_4 and RX-GPIO_PIN_5.
		
#if defined(UART_P04_P05)
    RESERVE_GPIO( UART1_TX, GPIO_PORT_0,  GPIO_PIN_4, PID_UART1_TX);
    RESERVE_GPIO( UART1_RX, GPIO_PORT_0,  GPIO_PIN_5, PID_UART1_RX);
#elif defined(UART_P00_P01)
    RESERVE_GPIO( UART1_TX, GPIO_PORT_0,  GPIO_PIN_0, PID_UART1_TX);
    RESERVE_GPIO( UART1_RX, GPIO_PORT_0,  GPIO_PIN_1, PID_UART1_RX);
#elif defined(UART_P14_P15)
    RESERVE_GPIO( UART1_TX, GPIO_PORT_1,  GPIO_PIN_4, PID_UART1_TX);
    RESERVE_GPIO( UART1_RX, GPIO_PORT_1,  GPIO_PIN_5, PID_UART1_RX);
#elif defined(UART_P04_P13)
    RESERVE_GPIO( UART1_TX, GPIO_PORT_0,  GPIO_PIN_4, PID_UART1_TX);
    RESERVE_GPIO( UART1_RX, GPIO_PORT_1,  GPIO_PIN_3, PID_UART1_RX);
#else
    #error "=== No UART pin configuration selected in periph_setup.h ==="
#endif // defined(UART_P04_P05)
  
#endif //CONFIG_UART_GPIOS
	
#ifdef SEND_RX_DATA_BACK_BY_SPI
    RESERVE_GPIO( SPI_CLK, GPIO_PORT_0, GPIO_PIN_6, PID_SPI_CLK);
    RESERVE_GPIO( SPI_DO, GPIO_PORT_0, GPIO_PIN_4, PID_SPI_DO);	
#endif

#if HAS_AUDIO
    declare_audio439_gpios();
#endif
}

#endif //DEVELOPMENT_DEBUG

/**
 ****************************************************************************************
 * @brief Map port pins
 *
 * The Uart and SPI port pins and GPIO ports(for debugging) are mapped
 ****************************************************************************************
 */
void set_pad_functions(void)
{
#if HAS_AUDIO
    init_audio439_gpios(app_audio439_timer_started);
#endif

#ifdef CONFIG_UART_GPIOS
    switch (port_sel)
    {
        case 0:
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_0, OUTPUT, PID_UART1_TX, false);
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_1, INPUT, PID_UART1_RX, false);
            update_uart_pads(GPIO_PORT_0, GPIO_PIN_0, GPIO_PIN_1);
            break;
        case 2:
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_2, OUTPUT, PID_UART1_TX, false);
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_3, INPUT, PID_UART1_RX, false);
            update_uart_pads(GPIO_PORT_0, GPIO_PIN_2, GPIO_PIN_3);
            break;
        case 4:
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_4, OUTPUT, PID_UART1_TX, false);
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_5, INPUT, PID_UART1_RX, false);
            update_uart_pads(GPIO_PORT_0, GPIO_PIN_4, GPIO_PIN_5);
            break;
        case 6:
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_6, OUTPUT, PID_UART1_TX, false);
            GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_7, INPUT, PID_UART1_RX, false);
            update_uart_pads(GPIO_PORT_0, GPIO_PIN_6, GPIO_PIN_7);
            break;
        default:
            set_pad_uart();
            break;
    }
#else
#if defined(UART_P04_P05)
    GPIO_ConfigurePin( GPIO_PORT_0, GPIO_PIN_4, OUTPUT, PID_UART1_TX, false );
    GPIO_ConfigurePin( GPIO_PORT_0, GPIO_PIN_5, INPUT, PID_UART1_RX, false );
#elif defined(UART_P00_P01)
    GPIO_ConfigurePin( GPIO_PORT_0, GPIO_PIN_0, OUTPUT, PID_UART1_TX, false );
    GPIO_ConfigurePin( GPIO_PORT_0, GPIO_PIN_1, INPUT, PID_UART1_RX, false );
#elif defined(UART_P14_P15)
    // wait until debugger is disconnected and then disable debuging
    while ((GetWord16(SYS_STAT_REG) & DBG_IS_UP) == DBG_IS_UP) {};
    SetBits16(SYS_CTRL_REG, DEBUGGER_ENABLE, 0);    // close debugger
    GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_4, OUTPUT, PID_UART1_TX, false );
    GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_5, INPUT, PID_UART1_RX, false );
#elif defined(UART_P04_P13)
    GPIO_ConfigurePin( GPIO_PORT_0, GPIO_PIN_4, OUTPUT, PID_UART1_TX, false);
    GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_3, INPUT, PID_UART1_RX, false);
#else
    #error "=== No UART pin configuration selected in periph_setup.h ==="
#endif // defined(UART_P04_P05)

#ifdef SEND_RX_DATA_BACK_BY_SPI
//ALLOCATE SPI SIGNALS
#if 0 //for testing with PXI system
    RESERVE_GPIO( SPI_CLK, GPIO_PORT_0, GPIO_PIN_6, PID_SPI_CLK);
    RESERVE_GPIO( SPI_DO, GPIO_PORT_0, GPIO_PIN_4, PID_SPI_DO);	
#endif
#endif //SEND_RX_DATA_BACK_BY_SPI
 
#endif //CONFIG_UART_GPIOS
}

void set_pad_uart(void)
{
    GPIO_ConfigurePin((GPIO_PORT) uart_sel_pins.uart_port_tx, (GPIO_PIN) uart_sel_pins.uart_tx_pin, OUTPUT, PID_UART1_TX, false);
    GPIO_ConfigurePin((GPIO_PORT) uart_sel_pins.uart_port_rx, (GPIO_PIN) uart_sel_pins.uart_rx_pin, INPUT, PID_UART1_RX, false);
}

void update_uart_pads(GPIO_PORT port, GPIO_PIN tx_pin, GPIO_PIN rx_pin)
{
    uart_sel_pins.uart_port_tx = port;
    uart_sel_pins.uart_tx_pin = tx_pin;
    uart_sel_pins.uart_port_rx = port;
    uart_sel_pins.uart_rx_pin = rx_pin;
}

/**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down.
 *
 * The Uart and SPi clocks are set. 
 ****************************************************************************************
 */
void periph_init(void)  // set i2c, spi, uart, uart2 serial clks
{
	// Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP)) ; 
    
    SetBits16(CLK_16M_REG,XTAL16_BIAS_SH_ENABLE, 1);
	
	// TODO: Application specific - Modify accordingly!
	// Example: Activate UART and SPI.
	
    // Initialize UART component
    SetBits16(CLK_PER_REG, UART1_ENABLE, 1);    // enable clock - always @16MHz
	
    // NOTE:
    // If a lower than 4800bps baudrate is required then uart_init() must be replaced
    // by a call to arch_uart_init_slow() (defined in arch_system.h).
    // mode=3-> no parity, 1 stop bit 8 data length
    uart_init(UART_BAUDRATE_115K2, 3);
	
	//rom patch
	patch_func();
	
	//Init pads
	set_pad_functions();

    // Enable the pads
	SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}

void init_TXEN_RXEN_irqs(void)
{
//init for TXEN	
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_WSEL_0, 2); //SO SELECT RADIO_DIAG0
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_MASK_0, 1); //ENABLE IRQ
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_BSEL_0, 7); //BIT7 OF DIAG0 BUS, SO TXEN
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_EDGE_0, 0); //SELECT POS EDGE

//init for RXEN	
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_WSEL_1, 3); //SO SELECT RADIO_DIAG1
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_MASK_1, 1); //ENABLE IRQ
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_BSEL_1, 7); //BIT7 OF DIAG1 BUS, SO RXEN
	SetBits32(BLE_RF_DIAGIRQ_REG, DIAGIRQ_EDGE_1, 0); //SELECT POS EDGE
	
	NVIC_EnableIRQ(BLE_RF_DIAG_IRQn); 
	NVIC_SetPriority(BLE_RF_DIAG_IRQn,4);     
    NVIC_ClearPendingIRQ(BLE_RF_DIAG_IRQn); //clear eventual pending bit, but not necessary becasuse this is already cleared automatically in HW

}		
