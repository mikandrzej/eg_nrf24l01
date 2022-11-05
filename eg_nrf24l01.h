#ifndef _EG_NRF24L01_H_
#define _EG_NRF24L01_H_
#include "stdint.h"

#define EG_NRF24L01_ADDRESS_MAX_WIDTH 5u
#define EG_NRF24L01_ADDRESS_P2_P5_WIDTH 1u
#define EG_NRF24L01_MAX_ADDRESS_NO 6u

typedef void (*eg_nrf_rx_callback)(uint8_t *data, uint8_t data_size);

typedef enum
{
    NRF_OK = 0,
    NRF_INVALID_ADDRESS_WIDTH = -100,
    NRF_INVALID_ARGUMENT,
    NRF_INVALID_P2_P5_ADDRESS, /**< 4 MSB's of P2-P5 address must be the same as 4 MSB's of P1 address */
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
    } config_registers;
    struct
    {
        eg_nrf_rx_callback rx_callback;
    } rx_pipe[EG_NRF24L01_ADDRESS_MAX_WIDTH];
} eg_nrf24l01_state_s;

/**
 * Function to configure NRF24L01 module driver instance
 *
 * @param state pointer to internal driver state object
 * @param init_data pointer to initialization data object
 * @return eg_nrf_error_e error code
 */
extern eg_nrf_error_e eg_nrf24l01_config(eg_nrf24l01_state_s *state, eg_nrf24l01_init_data_s *init_data);

#endif /* _EG_NRF24L01_H_ */