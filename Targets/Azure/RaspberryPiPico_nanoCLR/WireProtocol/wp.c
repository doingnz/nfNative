////
//// Copyright (c) .NET Foundation and Contributors
//// See LICENSE file in the project root for full license information.
////
///*
//Board: RaspberryPiPico
//
//   RaspberryPiPico-DK__    ____PC____
//  |   ________________|    |         |
//  |  |                |    | Virtual |
//  |  |    VCP UART    |<==>| Com     |
//  |  |                |    | Port    |
//  |  |________________|    |         |
//  |___________________|    |_________|
//
//*/
//
//#include "WireProtocol_HAL_Interface.h"
//#include "tusb.h"
//#include <ctype.h>
//#include <hardware/irq.h>
//#include <lib/tinyusb/src/device/usbd.h>
//
//#define PICO_ERROR_NO_DATA -3
//WP_Message inboundMessage;
//TX_EVENT_FLAGS_GROUP wpUartReceivedBytesEvent;
//
//void InitWireProtocolCommunications()
//{
//    // Initialize Tinyusb
//    tusb_init();
//}
//void WP_ReceiveBytes(uint8_t **ptr, uint32_t *size)
//{
//
//    if (*size != 0) // Required for the PING case where payload 0
//    {
//        while (!tud_cdc_n_connected(0))
//        {
//            tud_task();
//            tx_thread_relinquish();
//        }
//
//        while (!tud_cdc_n_available(0))
//        {
//            tud_task();
//            tx_thread_relinquish();
//        }
//        int requestedSize = *size;
//        size_t read = tud_cdc_n_read(0, (char *)*ptr, requestedSize);
//        tud_cdc_n_read_flush(0);
//        *ptr += read;
//        *size -= read;
//    }
//}
//uint8_t WP_TransmitMessage(WP_Message *message)
//{
//    bool operationResult = false;
//
//    while (!tud_cdc_write_available())
//    {
//        tud_task();
//        tx_thread_relinquish();
//    }
//    uint32_t writeResult = tud_cdc_n_write(0, (uint8_t *)&message->m_header, sizeof(message->m_header));
//
//    if (writeResult == sizeof(message->m_header))
//    {
//        operationResult = true;
//        // if there is anything on the payload send it to the output stream
//        if (message->m_header.m_size && message->m_payload)
//        {
//            operationResult = false;
//
//            while (!tud_cdc_write_available())
//            {
//                tud_task();
//                tx_thread_relinquish();
//            }
//
//            // Weird problem where if there are two 32 byte writes in a row, the PC doesn't see the bytes.
//            // Assume the issue is with TinyUSB (but need some PC USB port mon software to check theory)
//            // Add a sleep before sending the second 32 bytes
//            if (sizeof(message->m_header) == 32 && message->m_header.m_size == 32)
//            {
//                tx_thread_sleep(20);
//            }
//            writeResult = tud_cdc_n_write(0, (uint8_t *)message->m_payload, message->m_header.m_size);
//
//            if (writeResult == message->m_header.m_size)
//            {
//                operationResult = true;
//            }
//        }
//    }
//    tud_cdc_n_write_flush(0);
//
//    //  Wait until data sent
//    while (!tud_cdc_write_available())
//    {
//        tud_task();
//        tx_thread_relinquish();
//    }
//
//    return operationResult;
//}
