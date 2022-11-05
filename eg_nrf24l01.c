#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "eg_nrf24l01.h"

#define EG_NRF24L01_SETUP_AW_3BYTES 1u
#define EG_NRF24L01_SETUP_AW_4BYTES 2u
#define EG_NRF24L01_SETUP_AW_5BYTES 3u

eg_nrf_error_e eg_nrf24l01_config(eg_nrf24l01_state_s *state, eg_nrf24l01_init_data_s *init_data)
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

    return NRF_OK;
}