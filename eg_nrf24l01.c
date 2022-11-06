#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "eg_nrf24l01.h"

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

#define NRF_REG_TX_ADDR 0x10u
#define NRF_REG_RX_ADDR_P0 0x0Au
#define NRF_REG_CONFIG 0x00u

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

static void spi_write_config_register(eg_nrf24l01_state_s *state);

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

static void sm_state_power_off_handler(eg_nrf24l01_state_s *state)
{
    if (1u == state->power_on_request)
    {
        state->timestamp = eg_nrf24l01_user_timestamp_get() + POWER_ON_TIME_MS;

        state->config_registers.config.pwr_up = 1u;
        spi_write_config_register(state);

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
            state->sm_state = NRF_SM_CONFIGURE;
        }
    }
}
static void sm_state_configure_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_sleep_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_idle_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_ext_int_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
}
static void sm_state_status_reading_handler(eg_nrf24l01_state_s *state)
{
    asm("nop");
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

static void spi_write_config_register(eg_nrf24l01_state_s *state)
{
    if (state->set_csn_callback != NULL)
    {
        state->set_csn_callback(0u);
    }

    state->spi_tx_buf[0u] = NRF_CMD_WRITE_REG | NRF_REG_CONFIG;
    state->spi_tx_buf[1u] = state->config_registers.config.val;

    state->spi_data_ready = 0u;
    eg_nrf24l01_user_spi_transmit_receive(state, state->spi_tx_buf, 2u, state->spi_rx_buf, 2u);
}