/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_usbx_host.h
  * @author  MCD Application Team
  * @brief   USBX Host applicative header file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_USBX_HOST_H__
#define __APP_USBX_HOST_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ux_api.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_stm32.h"
#include "stm32h7xx_nucleo.h"
#include "ux_host_class_cdc_acm.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define USBH_UsrLog(...)   printf(__VA_ARGS__);\
                           printf("\n");

#define USBH_ErrLog(...)   printf("ERROR: ") ;\
                           printf(__VA_ARGS__);\
                           printf("\n");
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
UINT MX_USBX_Host_Init(VOID *memory_ptr);

/* USER CODE BEGIN EFP */
UINT  MX_USB_Host_Init(void);
void  USBH_DriverVBUS(uint8_t state);
void  usbx_app_thread_entry(ULONG arg);
VOID  ux_host_error_callback(UINT system_level, UINT system_context, UINT error_code);
UINT  ux_host_event_callback(ULONG event, UX_HOST_CLASS *p_host_class, VOID *p_instance);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_QUEUE_SIZE                               1
#define USBX_APP_STACK_SIZE                          1024
#define USBX_MEMORY_SIZE                             (64 * 1024)
#define NEW_RECEIVED_DATA                            0x01
#define NEW_DATA_TO_SEND                             0x02
#define BUTTON_KEY                                   BUTTON_USER
#define BUTTON_KEY_PIN                               BUTTON_USER_PIN
/* USER CODE END PD */

/* USER CODE BEGIN 1 */

typedef enum
{
  USB_VBUS_FALSE = 0,
  USB_VBUS_TRUE,
} USB_VBUS_State;

typedef enum
{
  CDC_ACM_Device = 1,
  Unsupported_Device,
  Unknown_Device,
} USB_Device_Type;

typedef enum
{
  Device_disconnected = 1,
  Device_connected,
  No_Device,
} Device_state;

typedef enum
{
  App_Ready = 1,
  App_Start,
  App_Idle,
} ux_app_stateTypeDef;

typedef struct
{
  USB_Device_Type Device_Type;
  Device_state    Dev_state;
} ux_app_devInfotypeDef;

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* __APP_USBX_HOST_H__ */
