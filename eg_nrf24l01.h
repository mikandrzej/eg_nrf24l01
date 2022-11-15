#ifndef _EG_NRF24L01_H_
#define _EG_NRF24L01_H_
#include "stdint.h"
#include "eg_nrf24l01_internal.h"

/**
 * @addtogroup NRF24L01_driver NRF24L01 communication module driver
 * @{
 * 
 */

#define EG_NRF24L01_MAX_ADDRESS_NO 6u /**< Maximum address index */

/** NRF24L01 error codes */
typedef enum
{
    NRF_OK = 0,                       /**< NRF No error*/
    NRF_INVALID_ADDRESS_WIDTH = -100, /**< Invalid given address width */
    NRF_INVALID_ARGUMENT,             /**< Invalid function argument */
    NRF_INVALID_P2_P5_ADDRESS,        /**< 4 MSB's of P2-P5 address must be the same as 4 MSB's of P1 address */
} eg_nrf_error_e;

/** NRF24L01 initialisation address width field value */
typedef enum
{
    ADDRESS_WIDTH_3_BYTES = 1, /**< Address width : 3 bytes */
    ADDRESS_WIDTH_4_BYTES = 2, /**< Address width : 4 bytes */
    ADDRESS_WIDTH_5_BYTES = 3  /**< Address width : 5 bytes */
} eg_nrf_address_width_e;

/** NRF24L01 initialization structure */
typedef struct
{
    eg_nrf_address_width_e address_width; /**< RX/TX Address field width */
    struct
    {
        uint8_t address[EG_NRF24L01_ADDRESS_MAX_WIDTH]; /**< RX address bytes */
        uint8_t enabled;                                /**< Enable RX address */
        uint8_t auto_ack;                               /**< Enable auto acknowledge */
        eg_nrf_rx_callback rx_callback;                 /**< User function callback to handle incomming data */
    } rx_pipe[EG_NRF24L01_MAX_ADDRESS_NO];              /**< RX addresses */
    eg_nrf_set_pin_state_callback set_ce_callback;      /**< User callback for setting CE pin state */
    eg_nrf_set_pin_state_callback set_csn_callback;     /**< User callback for setting CSn pin state */
} eg_nrf24l01_init_data_s;

/**
 * Function to configure NRF24L01 module driver instance
 *
 * @param state pointer to internal driver state object
 * @param init_data pointer to initialization data object
 * @return eg_nrf_error_e error code
 */
extern eg_nrf_error_e eg_nrf24l01_init(eg_nrf24l01_state_s *state, eg_nrf24l01_init_data_s *init_data);

/**
 * Function to process state machine of given driver instance
 *
 * @param state pointer to internal driver state object
 */
extern void eg_nrf24l01_process(eg_nrf24l01_state_s *state);

/**
 * Function to Power On the module
 * 
 * @param state pointer to internal driver state object
 * @return eg_nrf_error_e error code
 */
extern eg_nrf_error_e eg_nrf24l01_power_on(eg_nrf24l01_state_s *state);

/**
 * Function to Wake up the module
 * 
 * @param state pointer to internal driver state object
 * @return eg_nrf_error_e error code
 */
extern eg_nrf_error_e eg_nrf24l01_wake_up(eg_nrf24l01_state_s *state);

/**
 * Function to Sleep the module
 * 
 * @param state pointer to internal driver state object
 * @return eg_nrf_error_e error code
 */
extern eg_nrf_error_e eg_nrf24l01_sleep(eg_nrf24l01_state_s *state);

/**
 * User function to get timestamp in ms.
 * @brief User must define it somwhere in own code.
 *
 * @return uint64_t current time in milliseconds.
 */
extern uint64_t eg_nrf24l01_user_timestamp_get(void);

/**
 * User function to handle SPI data transmit / receive.
 * @brief User must define it somwhere in own code.
 * Function should not exceed rx buf length.
 *
 * @param state pointer to internal driver state object
 * @param tx_buf pointer to tx data buffer
 * @param tx_len length of data to transmit
 * @param rx_buf pointer to rx data buffer
 * @param rx_len expected length of data
 */

extern void eg_nrf24l01_user_spi_transmit_receive(eg_nrf24l01_state_s *state,
                                                  uint8_t *tx_buf,
                                                  uint8_t tx_len,
                                                  uint8_t *rx_buf,
                                                  uint8_t rx_len);

/**
 * Function to handle SPI transmit / receive data complete event
 *
 * @param state pointer to internal driver state object given by eg_nrf24l01_user_spi_transmit_receive function
 * @param rx_len length of data read from SPI
 */
extern void eg_nrf24l01_spi_comm_complete(eg_nrf24l01_state_s *state,
                                          uint8_t rx_len);

/**
 * Function to handle External Interrupt on FALLING edge of IRQ module pin
 * 
 * @param state pointer to internal driver state object given by eg_nrf24l01_user_spi_transmit_receive function
 */
extern void eg_nrf24l01_irq_handler(eg_nrf24l01_state_s *state);

/**
 * @}
 * 
 */

#endif /* _EG_NRF24L01_H_ */