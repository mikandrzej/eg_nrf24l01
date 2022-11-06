#ifndef _EG_NRF24L01_H_
#define _EG_NRF24L01_H_
#include "stdint.h"

#define EG_NRF24L01_ADDRESS_MAX_WIDTH 5u
#define EG_NRF24L01_ADDRESS_P2_P5_WIDTH 1u
#define EG_NRF24L01_MAX_ADDRESS_NO 6u

typedef void (*eg_nrf_rx_callback)(uint8_t *data, uint8_t data_size);
typedef void (*eg_nrf_set_pin_state_callback)(uint8_t state);

typedef enum
{
    NRF_OK = 0,
    NRF_INVALID_ADDRESS_WIDTH = -100,
    NRF_INVALID_ARGUMENT,
    NRF_INVALID_P2_P5_ADDRESS, /**< 4 MSB's of P2-P5 address must be the same as 4 MSB's of P1 address */
    NRF_INVALID_STATE
} eg_nrf_error_e;

typedef enum
{
    ADDRESS_WIDTH_3_BYTES = 1,
    ADDRESS_WIDTH_4_BYTES = 2,
    ADDRESS_WIDTH_5_BYTES = 3
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
    eg_nrf_set_pin_state_callback set_ce_callback;
    eg_nrf_set_pin_state_callback set_csn_callback;
} eg_nrf24l01_init_data_s;

typedef struct
{
    /** RX/TX Address field width
     * '00' - Illegal
     * '01' - 3 bytes
     * '10' - 4 bytes
     * '11' – 5 bytes
     * LSByte is used if address width is below 5 bytes */
    uint8_t aw : 2;
    uint8_t : 6;
} eg_nrf24l01_setup_aw_reg_s;

typedef union
{
    struct
    {
        uint8_t prim_rx : 1;
        uint8_t pwr_up : 1;
        uint8_t crco : 1;
        uint8_t en_crc : 1;
        uint8_t mask_max_rt : 1;
        uint8_t mask_max_ds : 1;
        uint8_t mask_rx_dr : 1;
        uint8_t res : 1;
    } __attribute__((packed));
    uint8_t val;
} eg_nrf24l01_config_reg_s;

typedef struct
{
    union
    {
        struct
        {
            uint8_t p0 : 1;
            uint8_t p1 : 1;
            uint8_t p2 : 1;
            uint8_t p3 : 1;
            uint8_t p4 : 1;
            uint8_t p5 : 1;
            uint8_t : 2;
        } __attribute__((packed));
        uint8_t val;
    };
} __attribute__((packed)) eg_nrf24l01_en_aa_reg_s;

typedef struct
{
    union
    {
        struct
        {
            uint8_t p0 : 1;
            uint8_t p1 : 1;
            uint8_t p2 : 1;
            uint8_t p3 : 1;
            uint8_t p4 : 1;
            uint8_t p5 : 1;
            uint8_t : 2;
        } __attribute__((packed));
        uint8_t val;
    };
} __attribute__((packed)) eg_nrf24l01_en_rxaddr_reg_s;

/**  */
typedef enum
{
    NRF_SM_POWER_OFF = 0,
    NRF_SM_MODULE_STARTUP,
    NRF_SM_CONFIGURE,
    NRF_SM_SLEEP,
    NRF_SM_IDLE,
    NRF_SM_EXT_INT,
    NRF_SM_STATUS_READING,
    NRF_SM_RECEIVE,
    NRF_SM_RECEIVING,
    NRF_SM_TRANSMIT,
    NRF_SM_TRANSMITTING,
    NRF_SM_POWERING_OFF,
    NRF_SM_MAX_STATE,
} eg_nrf24l01_sm_state_e;

/** NRF24L01 internal state structure */
typedef struct
{
    struct
    {
        eg_nrf24l01_setup_aw_reg_s setup_aw;
        uint8_t rx_addr_p0[EG_NRF24L01_ADDRESS_MAX_WIDTH];
        uint8_t rx_addr_p1[EG_NRF24L01_ADDRESS_MAX_WIDTH];
        uint8_t rx_addr_p2;
        uint8_t rx_addr_p3;
        uint8_t rx_addr_p4;
        uint8_t rx_addr_p5;
        eg_nrf24l01_en_rxaddr_reg_s en_rxaddr;
        eg_nrf24l01_en_rxaddr_reg_s en_aa;
        eg_nrf24l01_config_reg_s config;
    } config_registers;

    struct
    {
        eg_nrf_rx_callback rx_callback;
    } rx_pipe[EG_NRF24L01_ADDRESS_MAX_WIDTH];
    
    eg_nrf_set_pin_state_callback set_ce_callback;
    eg_nrf_set_pin_state_callback set_csn_callback;

    /* Flags*/
    volatile uint8_t power_on_request;
    volatile uint8_t spi_data_ready;

    /* SPI specific */
    uint8_t spi_tx_buf[50];
    uint8_t spi_rx_buf[50];
    volatile uint8_t spi_rx_data_len;

    /* SM data */
    uint64_t timestamp;
    eg_nrf24l01_sm_state_e sm_state;
} eg_nrf24l01_state_s;

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

extern eg_nrf_error_e eg_nrf24l01_power_on(eg_nrf24l01_state_s *state);

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
#endif /* _EG_NRF24L01_H_ */