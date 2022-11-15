#ifndef _EG_NRF24L01_INTERNAL_H_
#define _EG_NRF24L01_INTERNAL_H_

/**
 * @addtogroup NRF24L01_driver NRF24L01 communication module driver
 * @{
 * @addtogroup NRF24L01_driver_internal Internal header data
 * @{
 */
/** Maximum address length in bytes */
#define EG_NRF24L01_ADDRESS_MAX_WIDTH 5u

/** User RX callback prototype */
typedef void (*eg_nrf_rx_callback)(uint8_t *data, uint8_t data_size);
/** User set GPIO pin state prototype */
typedef void (*eg_nrf_set_pin_state_callback)(uint8_t state);

/** nRF24L01 SETUP_AW register struct */
typedef union
{
    struct
    {
        /** RX/TX Address field width
         * '00' - Illegal
         * '01' - 3 bytes
         * '10' - 4 bytes
         * '11' – 5 bytes
         * LSByte is used if address width is below 5 bytes */
        uint8_t aw : 2;
        uint8_t : 6;
    } __attribute__((packed));
    uint8_t val; /** RAW value */
} eg_nrf24l01_setup_aw_reg_s;

/** nRF24L01 CONFIG register struct */
typedef union
{
    struct
    {
        uint8_t prim_rx : 1;     /**< RX/TX control */
        uint8_t pwr_up : 1;      /**< Module power control */
        uint8_t crco : 1;        /**< CRC encoding scheme */
        uint8_t en_crc : 1;      /**< Enable CRC */
        uint8_t mask_max_rt : 1; /**< Mask interrupt caused by TX_DS */
        uint8_t mask_tx_ds : 1;  /**< Mask interrupt caused by RX_DS */
        uint8_t mask_rx_dr : 1;  /**< Mask interrupt caused by RX_DR */
        uint8_t : 1;
    } __attribute__((packed));
    uint8_t val; /** RAW value */
} eg_nrf24l01_config_reg_s;

/** nRF24L01 STATUS register struct */
typedef union
{
    struct
    {
        uint8_t tx_full : 1; /**< TX FIFO full flag */
        uint8_t rx_p_no : 3; /**< Data pipe number for the payload available */
        uint8_t max_rt : 1;  /**< Maximum number of TX retransmits interrupt */
        uint8_t tx_ds : 1;   /**< Data Sent TX FIFO interrupt */
        uint8_t rx_dr : 1;   /**< Data Ready RX FIFO interrupt */
        uint8_t : 1;
    } __attribute__((packed));
    uint8_t val; /** RAW value */
} eg_nrf24l01_status_reg_s;

/** nRF24L01 FIFO_STATUS register struct */
typedef union
{
    struct
    {
        uint8_t rx_empty : 1; /**< RX FIFO empty flag */
        uint8_t rx_full : 1;  /**< RX FIFO full flag */
        uint8_t : 2;
        uint8_t tx_empty : 1; /**< TX FIFO empty flag. */
        uint8_t tx_full : 1;  /**< TX FIFO full flag */
        uint8_t tx_reuse : 1; /**< Used for a PTX device */
        uint8_t : 1;
    } __attribute__((packed));
    uint8_t val; /** RAW value */
} eg_nrf24l01_fifo_status_reg_s;

/** nRF24L01 Enable Auto Acknowledgment register */
typedef struct
{
    union
    {
        struct
        {
            uint8_t p0 : 1; /**< PIPE 0 */
            uint8_t p1 : 1; /**< PIPE 1 */
            uint8_t p2 : 1; /**< PIPE 2 */
            uint8_t p3 : 1; /**< PIPE 3 */
            uint8_t p4 : 1; /**< PIPE 4 */
            uint8_t p5 : 1; /**< PIPE 5 */
            uint8_t : 2;
        } __attribute__((packed));
        uint8_t val; /** RAW value */
    };
} __attribute__((packed)) eg_nrf24l01_en_aa_reg_s;

/** nRF24L01 Enabled RX Addresses register */
typedef struct
{
    union
    {
        struct
        {
            uint8_t p0 : 1; /**< PIPE 0 */
            uint8_t p1 : 1; /**< PIPE 1 */
            uint8_t p2 : 1; /**< PIPE 2 */
            uint8_t p3 : 1; /**< PIPE 3 */
            uint8_t p4 : 1; /**< PIPE 4 */
            uint8_t p5 : 1; /**< PIPE 5 */
            uint8_t : 2;
        } __attribute__((packed));
        uint8_t val; /** RAW value */
    };
} __attribute__((packed)) eg_nrf24l01_en_rxaddr_reg_s;

/** nRF24L01 internal machine state states */
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
    /** nRF24L01 module registers cache*/
    struct
    {
        eg_nrf24l01_config_reg_s config;                   /**< CONFIG register value */
        eg_nrf24l01_en_rxaddr_reg_s en_aa;                 /**< EN_AA register value */
        eg_nrf24l01_en_rxaddr_reg_s en_rxaddr;             /**< EN_RXADDR register value */
        eg_nrf24l01_setup_aw_reg_s setup_aw;               /**< SETUP_AW register value */
        eg_nrf24l01_status_reg_s status;                   /**< STATUS register value */
        uint8_t rx_addr_p0[EG_NRF24L01_ADDRESS_MAX_WIDTH]; /**< RX address on PIPE 0 */
        uint8_t rx_addr_p1[EG_NRF24L01_ADDRESS_MAX_WIDTH]; /**< RX address on PIPE 1 */
        uint8_t rx_addr_p2;                                /**< RX address on PIPE 2 - four MSB ar the same as on P1 */
        uint8_t rx_addr_p3;                                /**< RX address on PIPE 3 - four MSB ar the same as on P1 */
        uint8_t rx_addr_p4;                                /**< RX address on PIPE 4 - four MSB ar the same as on P1 */
        uint8_t rx_addr_p5;                                /**< RX address on PIPE 5 - four MSB ar the same as on P1 */
        eg_nrf24l01_fifo_status_reg_s fifo_status;         /**< FIFO_STATUS register value */
    } config_registers;

    /** RX pipes user configuration */
    struct
    {
        eg_nrf_rx_callback rx_callback; /**< User data received callback */
    } rx_pipe[EG_NRF24L01_ADDRESS_MAX_WIDTH];

    /** Set Chip Enable GPIO user callback */
    eg_nrf_set_pin_state_callback set_ce_callback;
    /** Set non Chip Select GPIO user callback */
    eg_nrf_set_pin_state_callback set_csn_callback;

    /* SPI specific */
    /** Local SPI tx buffer */
    uint8_t spi_tx_buf[50];
    /** Local SPI rx buffer */
    uint8_t spi_rx_buf[50];
    /** Local SPI rx data length value */
    volatile uint8_t spi_rx_data_len;

    /* SM data */
    /** Machine state internal Power On Request flag */
    volatile uint8_t power_on_request;
    /** Machine state internal Wake Up Request flag */
    volatile uint8_t wake_up_request;
    /** Machine state internal Sleep Request flag */
    volatile uint8_t sleep_request;
    /** Machine state internal External Interrupt Request flag */
    volatile uint8_t ext_interrupt_request;
    /** Machine state SPI data ready flag */
    volatile uint8_t spi_data_ready;
    /** Saved timestamp value */
    uint64_t timestamp;
    /** State machine state */
    eg_nrf24l01_sm_state_e sm_state;
} eg_nrf24l01_state_s;

/**
 * @}
 * @}
 *
 */

#endif /* _EG_NRF24L01_INTERNAL_H_ */