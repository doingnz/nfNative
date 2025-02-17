//
// Copyright (c) .NET Foundation and Contributors
// Portions Copyright (c) Microsoft Corporation.  All rights reserved.
// See LICENSE file in the project root for full license information.
//
/*
Board: RaspberryPiPico

   _ RaspberryPiPico-DK ___      ____PC___
  |   ________________     |    |         |
  |  |                |    |    | Virtual |
  |  |    VCP UART    |    |<==>| Com     |
  |  |                |    |    | Port    |
  |  |________________|    |    |         |
  |________________________|    |_________|
*/

#include "wp_Communications.h"
#include "tusb.h"
#include "wp_CircularBuffer.h"
#include <lib/tinyusb/src/device/usbd.h>
#include <tx_api.h>

static bool UsbConnected = false;

TX_EVENT_FLAGS_GROUP wpReceivedEvent;

void InitWireProtocolCommunications()
{
    // Initialize Tinyusb
    tusb_init();

    // Uncomment out when DtrEnabled on VS-Extension
    // Tinyusb connected depends on the host setting the Dtr bit
    // while (!tud_cdc_n_connected(0))
    // {
    // tud_task();
    // }

    // Create event based data synchronization
    tx_event_flags_create(&wpReceivedEvent, "wpReceiveDataEvent");
}
int wp_ReadBytes(uint8_t **ptr, uint32_t *size, uint32_t wait_time)
{
    ULONG actual_flags;
    uint32_t requestedSize = *size;
    tx_event_flags_get(&wpReceivedEvent, 0x1, TX_OR_CLEAR, &actual_flags, wait_time);

    if (!UsbConnected)
    {
        while (!tud_cdc_n_available(0))
        {
            tud_task();
        }
        UsbConnected = true;
    }
    // Bytes have arrived try to read what was requested
    while (!tud_cdc_n_available(0))
    {
        tud_task();
    }
    ULONG read = tud_cdc_n_read(0, *ptr, requestedSize);
    return read;
}
bool wp_WriteBytes(uint8_t *ptr, uint16_t size)
{
    bool operationResult = false;

    while (!tud_cdc_write_available())
    {
        tud_task();
        //        tx_thread_sleep(1);
    }

    uint32_t writeResult = tud_cdc_n_write(0, ptr, size);
    tud_cdc_n_write_flush(0);

    return true;
}

void WireProtocolUsb_dcd_event_rp2040(int eventType)
{
    switch (eventType)
    {
    case 1:
        tx_event_flags_set(&wpReceivedEvent, 0x1, TX_OR);
        break;
    case 2:
        tx_event_flags_set(&wpReceivedEvent, 0x1, TX_OR);
        break;
    case 3:
        tx_event_flags_set(&wpReceivedEvent, 0x1, TX_OR);
        break;
    }
}
