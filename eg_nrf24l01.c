#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "eg_nrf24l01.h"

/**
 * @addtogroup NRF24L01_driver
 * @{
 *
 */

#define EG_NRF24L01_SETUP_AW_3BYTES 1u
#define EG_NRF24L01_SETUP_AW_4BYTES 2u
#define EG_NRF24L01_SETUP_AW_5BYTES 3u

#define POWER_ON_TIME_MS 100u /**< Time needed to power on then module */

#define NRF_CMD_READ_REG 0x00u
#define NRF_CMD_WRITE_REG 0x20u
#define NRF_CMD_NOP 0xFFu
#define NRF_CMD_W_TX_PAYLOAD 0xA0u
#define NRF_CMD_W_TX_PAYLOAD_NO_ACK 0xB0u
#define NRF_CMD_FLUSH_TX 0xE1
#define NRF_CMD_FLUSH_RX 0xE2

#define NRF_REG_CONFIG 0x00u
#define NRF_REG_EN_AA 0x01u
#define NRF_REG_EN_RXADDR 0x02u
#define NRF_REG_SETUP_AW 0x03u
#define NRF_REG_SETUP_RETR 0x04u
#define NRF_REG_RF_CH 0x05u
#define NRF_REG_RF_SETUP 0x06u
#define NRF_REG_STATUS 0x07u
#define NRF_REG_OBSERVE_TX 0x08u
#define NRF_REG_RPD 0x09u
#define NRF_REG_RX_ADDR_P0 0x0Au
#define NRF_REG_RX_ADDR_P1 0x0Bu
#define NRF_REG_RX_ADDR_P2 0x0Cu
#define NRF_REG_RX_ADDR_P3 0x0Du
#define NRF_REG_RX_ADDR_P4 0x0Eu
#define NRF_REG_RX_ADDR_P5 0x0Fu
#define NRF_REG_TX_ADDR 0x10u
#define NRF_REG_RX_PW_P0 0x11u
#define NRF_REG_RX_PW_P1 0x12u
#define NRF_REG_RX_PW_P2 0x13u
#define NRF_REG_RX_PW_P3 0x14u
#define NRF_REG_RX_PW_P4 0x15u
#define NRF_REG_RX_PW_P5 0x16u
#define NRF_REG_FIFO_STATUS 0x17u

static void sm_state_power_off_handler(eg_nrf24l01_state_s *state);
static void sm_state_module_startup_handler(eg_nrf24l01_state_s *state);
static void sm_state_configure_handler(eg_nrf24l01_state_s *state);
static void sm_state_sleep_handler(eg_nrf24l01_state_s *state);
static void sm_state_idle_handler(eg_nrf24l01_state_s *state);
static void sm_state_ext_int_handler(eg_nrf24l01_state_s *state);
static void sm_state_status_reading_handler(eg_nrf24l01_state_s *state);
static void sm_state_receive_handler(eg_nrf24l01_state_s *state);
static void sm_state_receiving_handler(eg_nrf24l01_state_s *state);
static void sm_state_transmit_handler(eg_nrf24l01_state_s *state);
static void sm_state_transmitting_handler(eg_nrf24l01_state_s *state);
static void sm_state_powering_off_handler(eg_nrf24l01_state_s *state);

static void spi_write_register(eg_nrf24l01_state_s *state,
                               uint8_t reg,
                               uint8_t *data,
                               uint8_t data_len);
static void spi_read_register(eg_nrf24l01_state_s *state,
                              uint8_t reg,
                              uint8_t read_len);

typedef void (*state_handler)(eg_nrf24l01_state_s *state);

const state_handler state_handlers_lut[NRF_SM_MAX_STATE] = {
    sm_state_power_off_handler,
    sm_state_module_startup_handler,
    sm_state_configure_handler,
    sm_state_sleep_handler,
    sm_state_idle_handler,
    sm_state_ext_int_handler,
    sm_state_status_reading_handler,
    sm_state_receive_handler,
    sm_state_receiving_handler,
    sm_state_transmit_handler,
    sm_state_transmitting_handler,
    sm_state_powering_off_handler};

eg_nrf_error_e eg_nrf24l01_init(eg_nrf24l01_state_s *state, eg_nrf24l01_init_data_s *init_data)
{
    if (NULL == state || NULL == init_data)
    {
        return NRF_INVALID_ARGUMENT;
    }

    memset(state, 0, sizeof(eg_nrf24l01_state_s));

    if (init_data->address_width < ADDRESS_WIDTH_3_BYTES || init_data->address_width > ADDRESS_WIDTH_5_BYTES)
    {
        return NRF_INVALID_ADDRESS_WIDTH;
    }
    state->config_registers.setup_aw.aw = (uint8_t)init_data->address_width;

    const struct
    {
        uint8_t *dest_address;
    } addr_lut[EG_NRF24L01_MAX_ADDRESS_NO] =
        {
            {.dest_address = state->config_registers.rx_addr_p0},
            {.dest_address = state->config_registers.rx_addr_p1},
            {.dest_address = &state->config_registers.rx_addr_p2},
            {.dest_address = &state->config_registers.rx_addr_p3},
            {.dest_address = &state->config_registers.rx_addr_p4},
            {.dest_address = &state->config_registers.rx_addr_p5},
        };

    for (uint8_t i = 0; i < EG_NRF24L01_MAX_ADDRESS_NO; i++)
    {
        if (1u == init_data->rx_pipe[i].enabled)
        {
            switch (i)
            {
            case 0u ... 1u:
                memcpy(addr_lut[i].dest_address, init_data->rx_pipe[i].address, EG_NRF24L01_ADDRESS_MAX_WIDTH);
                break;
            case 2u ... 5u:
                if (0 != memcmp(init_data->rx_pipe[1u].address, init_data->rx_pipe[i].address, EG_NRF24L01_ADDRESS_MAX_WIDTH - 1u))
                {
                    return NRF_INVALID_P2_P5_ADDRESS;
                }
                *addr_lut[i].dest_address = init_data->rx_pipe[i].address[EG_NRF24L01_ADDRESS_MAX_WIDTH - 1u];
                break;
            default:
                /* Unexpected */
                break;
            }

            state->config_registers.en_rxaddr.val |= 1u << i;
            if (1u == init_data->rx_pipe[i].auto_ack)
            {
                state->config_registers.en_aa.val |= 1u << i;
            }

            state->rx_pipe[i].rx_callback = init_data->rx_pipe[i].rx_callback;
        }
    }

    state->config_registers.config.en_crc = 1u;

    state->set_ce_callback = init_data->set_ce_callback;
    state->set_csn_callback = init_data->set_csn_callback;

    return NRF_OK;
}

void eg_nrf24l01_process(eg_nrf24l01_state_s *state)
{
    if (NULL == state)
    {
        return;
    }
    if (state->sm_state >= NRF_SM_MAX_STATE)
    {
        return;
    }
    /* Execute state handler */
    state_handlers_lut[state->sm_state](state);
}

eg_nrf_error_e eg_nrf24l01_power_on(eg_nrf24l01_state_s *state)
{
    if (NULL == state)
    {
        return NRF_INVALID_ARGUMENT;
    }

    state->power_on_request = 1u;

    return NRF_OK;
}

eg_nrf_error_e eg_nrf24l01_wake_up(eg_nrf24l01_state_s *state)
{
    if (NULL == state)
    {
        return NRF_INVALID_ARGUMENT;
    }

    state->wake_up_request = 1u;

    return NRF_OK;
}

eg_nrf_error_e eg_nrf24l01_sleep(eg_nrf24l01_state_s *state)
{
    if (NULL == state)
    {
        return NRF_INVALID_ARGUMENT;
    }

    state->sleep_request = 1u;

    return NRF_OK;
}

void eg_nrf24l01_spi_comm_complete(eg_nrf24l01_state_s *state,
                                   uint8_t rx_len)
{
    state->spi_rx_data_len = rx_len;
    state->spi_data_ready = 1u;

    if (state->set_csn_callback != NULL)
    {
        state->set_csn_callback(1u);
    }
}

void eg_nrf24l01_irq_handler(eg_nrf24l01_state_s *state)
{
    if (state != NULL)
    {
        state->ext_interrupt_request = 1u;
    }
}

static void sm_state_power_off_handler(eg_nrf24l01_state_s *state)
{
    if (1u == state->power_on_request)
    {
        state->timestamp = eg_nrf24l01_user_timestamp_get() + POWER_ON_TIME_MS;

        /* Power UP the module */
        state->config_registers.config.pwr_up = 1u;
        /* Set module to RX mode */
        state->config_registers.config.prim_rx = 1u;
        /* Enable RX Data Ready interrupt */
        state->config_registers.config.mask_rx_dr = 1u;
        /* Enable TX Data Sent interrupt */
        state->config_registers.config.mask_tx_ds = 1u;

        spi_write_register(state,
                           NRF_REG_CONFIG,
                           &state->config_registers.config.val,
                           sizeof(state->config_registers.config));

        state->sm_state = NRF_SM_MODULE_STARTUP;
    }
}
static void sm_state_module_startup_handler(eg_nrf24l01_state_s *state)
{
    if (1u == state->spi_data_ready)
    {
        uint64_t timestamp = eg_nrf24l01_user_timestamp_get();
        if (timestamp >= state->timestamp)
        {
            /* Defined time elapsed */
            state->sm_state = NRF_SM_CONFIGURE;
        }
    }
}
static void sm_state_configure_handler(eg_nrf24l01_state_s *state)
{
    // /* Configure EN_AA register */
    // spi_write_register(state,
    //                    NRF_REG_EN_AA,
    //                    &state->config_registers.en_aa.val,
    //                    sizeof(state->config_registers.en_aa));
    // while (0u == state->spi_data_ready)
    //     ;

    // /* Configure EN_RXADDR register */
    // spi_write_register(state,
    //                    NRF_REG_EN_RXADDR,
    //                    &state->config_registers.en_rxaddr.val,
    //                    sizeof(state->config_registers.en_rxaddr));
    // while (0u == state->spi_data_ready)
    //     ;

    // /* Configure SETUP_AW register */
    // spi_write_register(state,
    //                    NRF_REG_SETUP_AW,
    //                    &state->config_registers.setup_aw.val,
    //                    sizeof(state->config_registers.setup_aw));
    // while (0u == state->spi_data_ready)
    //     ;

    // /* Configure RX_ADDR_P0 register */
    // spi_write_register(state,
    //                    NRF_REG_RX_ADDR_P0,
    //                    state->config_registers.rx_addr_p0,
    //                    sizeof(state->config_registers.rx_addr_p0));
    // while (0u == state->spi_data_ready)
    //     ;
    // /* Configure RX_ADDR_P1 register */
    // spi_write_register(state,
    //                    NRF_REG_RX_ADDR_P1,
    //                    state->config_registers.rx_addr_p1,
    //                    sizeof(state->config_registers.rx_addr_p1));
    // while (0u == state->spi_data_ready)
    //     ;
    // /* Configure RX_ADDR_P2 register */
    // spi_write_register(state,
    //                    NRF_REG_RX_ADDR_P2,
    //                    &state->config_registers.rx_addr_p2,
    //                    sizeof(state->config_registers.rx_addr_p2));
    // while (0u == state->spi_data_ready)
    //     ;
    // /* Configure RX_ADDR_P3 register */
    // spi_write_register(state,
    //                    NRF_REG_RX_ADDR_P3,
    //                    &state->config_registers.rx_addr_p3,
    //                    sizeof(state->config_registers.rx_addr_p3));
    // while (0u == state->spi_data_ready)
    //     ;
    // /* Configure RX_ADDR_P4 register */
    // spi_write_register(state,
    //                    NRF_REG_RX_ADDR_P4,
    //                    &state->config_registers.rx_addr_p4,
    //                    sizeof(state->config_registers.rx_addr_p4));
    // while (0u == state->spi_data_ready)
    //     ;
    // /* Configure RX_ADDR_P5 register */
    // spi_write_register(state,
    //                    NRF_REG_RX_ADDR_P5,
    //                    &state->config_registers.rx_addr_p5,
    //                    sizeof(state->config_registers.rx_addr_p5));
    // while (0u == state->spi_data_ready)
    //     ;
    // /* FLUSH_TX */
    // spi_write_register(state,
    //                    NRF_CMD_FLUSH_TX,
    //                    NULL,
    //                    0u);
    // while (0u == state->spi_data_ready)
    //     ;
    // /* FLUSH_RX */
    // spi_write_register(state,
    //                    NRF_CMD_FLUSH_RX,
    //                    NULL,
    //                    0u);
    // while (0u == state->spi_data_ready)
    //     ;

    state->sm_state = NRF_SM_SLEEP;
}
static void sm_state_sleep_handler(eg_nrf24l01_state_s *state)
{
    if (1u == state->wake_up_request)
    {
        state->wake_up_request = 0u;
        if (state->set_ce_callback != NULL)
        {
            state->set_ce_callback(1u);
        }
        state->sm_state = NRF_SM_IDLE;
    }
}
static void sm_state_idle_handler(eg_nrf24l01_state_s *state)
{
    if (1u == state->ext_interrupt_request)
    {
        state->sm_state = NRF_SM_EXT_INT;
    }
    else
    {
    }
}
static void sm_state_ext_int_handler(eg_nrf24l01_state_s *state)
{
    spi_read_register(state, NRF_REG_FIFO_STATUS, 1u);

    state->sm_state = NRF_SM_STATUS_READING;
}
static void sm_state_status_reading_handler(eg_nrf24l01_state_s *state)
{
    if (1u == state->spi_data_ready)
    {
        state->config_registers.status.val = state->spi_rx_buf[0u];
        state->config_registers.fifo_status.val = state->spi_rx_buf[1u];
    }
}
static void sm_state_receive_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_receiving_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_transmit_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_transmitting_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_powering_off_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}

static void spi_write_register(eg_nrf24l01_state_s *state,
                               uint8_t reg,
                               uint8_t *data,
                               uint8_t data_len)
{
    if (state->set_csn_callback != NULL)
    {
        state->set_csn_callback(0u);
    }

    state->spi_tx_buf[0u] = NRF_CMD_WRITE_REG | reg;
    memcpy(&state->spi_tx_buf[1u], data, data_len);

    state->spi_data_ready = 0u;
    eg_nrf24l01_user_spi_transmit_receive(state,
                                          state->spi_tx_buf,
                                          data_len + 1u,
                                          state->spi_rx_buf,
                                          data_len + 1u);
}

static void spi_read_register(eg_nrf24l01_state_s *state,
                              uint8_t reg,
                              uint8_t read_len)
{
    if (state->set_csn_callback != NULL)
    {
        state->set_csn_callback(0u);
    }

    state->spi_tx_buf[0u] = NRF_CMD_READ_REG | reg;

    state->spi_data_ready = 0u;
    eg_nrf24l01_user_spi_transmit_receive(state,
                                          state->spi_tx_buf,
                                          1u,
                                          state->spi_rx_buf,
                                          read_len + 1u);
}

/**
 * @}
 *
 */