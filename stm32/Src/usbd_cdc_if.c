/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @author         : MCD Application Team
  * @version        : V1.1.0
  * @date           : 19-March-2012
  * @brief          :
  ******************************************************************************
  * COPYRIGHT(c) 2014 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"
#include <stdlib.h>
/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CDC 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */ 
  /* USER CODE BEGIN 0 */ 
USBD_CDC_LineCodingTypeDef LineCoding =
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  };
  /* USER CODE END 0 */ 
/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Defines
  * @{
  */ 
  /* USER CODE BEGIN 1 */
/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */
#define APP_RX_DATA_SIZE  64
#define APP_TX_DATA_SIZE  512
  /* USER CODE END 1 */  
/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Macros
  * @{
  */ 
  /* USER CODE BEGIN 2 */ 
  /* USER CODE END 2 */
/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_Private_Variables
  * @{
  */
  /* USER CODE BEGIN 3 */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/* Received Data over USB are stored in this buffer       */
uint8_t UserLowLevelRxBufferFS[APP_RX_DATA_SIZE];

#define APP_HIGHLEVEL_RX_BUFFER_SIZE 512

uint8_t AppHighLevelRxBuffer[APP_HIGHLEVEL_RX_BUFFER_SIZE];
volatile uint32_t AppRxBufferHeadIndex; /* writer:  app; reader:  ISR */
volatile uint32_t AppRxBufferTailIndex; /* writer:  ISR; reader:  app */

/* Send Data over USB CDC are stored in this buffer       */
uint8_t UserLowLevelTxBufferFS[APP_TX_DATA_SIZE];

void MX_USB_DEVICE_DeInit(void);
void MX_USB_DEVICE_Init(void);

  /* USER CODE END 3 */

/* USB handler declaration */
/* Handle for USB Full Speed IP */
USBD_HandleTypeDef  *hUsbDevice_0 = NULL;

extern USBD_HandleTypeDef hUsbDeviceFS;

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */
static int8_t CDC_Init_FS     (void);
static int8_t CDC_DeInit_FS   (void);
static int8_t CDC_Control_FS  (uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS  (uint8_t* pbuf, uint32_t *Len);

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = 
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,  
  CDC_Receive_FS
};

/* USB init/deinit/reinit debugging counters */

uint32_t usb_init_count = 0;
uint32_t usb_deinit_count = 0;
uint32_t usb_deinit_while_tx_count = 0;
uint32_t usb_set_line_coding_count = 0;
uint32_t usb_reset_count = 0;
uint32_t usb_last_tx_not_busy_timestamp = 0;

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  CDC_Init_FS
  *         Initializes the CDC media low layer over the FS USB IP
  * @param  None
  * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  hUsbDevice_0 = &hUsbDeviceFS;
  /* USER CODE BEGIN 4 */ 
  /* Set Application Buffers */
  USBD_CDC_SetTxBuffer(hUsbDevice_0, UserLowLevelTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(hUsbDevice_0, UserLowLevelRxBufferFS);
  AppRxBufferHeadIndex = 0;
  AppRxBufferTailIndex = 0;
  if ( hUsbDevice_0 == NULL ) return USBD_FAIL;
  usb_init_count++;
  return (USBD_OK);
  /* USER CODE END 4 */ 
}

/**
  * @brief  CDC_DeInit_FS
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
    if ( hUsbDevice_0 == NULL ) return USBD_FAIL;
    USBD_CDC_HandleTypeDef *pCDC =
            (USBD_CDC_HandleTypeDef *)hUsbDevice_0->pClassData;
    if ( pCDC->TxState != 0 ) {
        pCDC->TxState = 0;
        usb_deinit_while_tx_count++;
        MX_USB_DEVICE_DeInit();
        MX_USB_DEVICE_Init();
    }
    hUsbDevice_0 = NULL;
    usb_deinit_count++;
  /* USER CODE BEGIN 5 */ 
  return (USBD_OK);
  /* USER CODE END 5 */ 
}

/**
  * @brief  CDC_Control_FS
  *         Manage the CDC class requests
  * @param  Cmd: Command code            
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS  (uint8_t cmd, uint8_t* pbuf, uint16_t length)
{ 
  /* USER CODE BEGIN 6 */
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:
 
    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:
 
    break;

  case CDC_SET_COMM_FEATURE:
 
    break;

  case CDC_GET_COMM_FEATURE:

    break;

  case CDC_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */ 
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
  case CDC_SET_LINE_CODING:
    LineCoding.bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |\
                            (pbuf[2] << 16) | (pbuf[3] << 24));
    LineCoding.format     = pbuf[4];
    LineCoding.paritytype = pbuf[5];
    LineCoding.datatype   = pbuf[6];
    usb_set_line_coding_count++;
    break;

  case CDC_GET_LINE_CODING:
    pbuf[0] = (uint8_t)(LineCoding.bitrate);
    pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
    pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
    pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
    pbuf[4] = LineCoding.format;
    pbuf[5] = LineCoding.paritytype;
    pbuf[6] = LineCoding.datatype;
    break;

  case CDC_SET_CONTROL_LINE_STATE:

    break;

  case CDC_SEND_BREAK:
 
    break;    
    
  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 6 */
}

uint32_t CDC_Receive_Available()
{
	return abs(AppRxBufferTailIndex - AppRxBufferHeadIndex);
}

/**
  * @brief  CDC_Receive_FS
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         untill exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result 
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS (uint8_t* Buf, uint32_t *Len)
{
    /* USER CODE BEGIN 7 */
    /* Transfer data into circular buffer */
    uint32_t new_tail_index = AppRxBufferTailIndex + *Len;
    uint32_t first_copy_len = *Len;
    uint32_t second_copy_len = 0;
    uint32_t buffer_bytes_available = APP_HIGHLEVEL_RX_BUFFER_SIZE - CDC_Receive_Available();
    /* If more bytes have arrived than are in the buffer, */
    /* discard extra bytes at the end of the receive buffer */
    if ( first_copy_len > buffer_bytes_available ) {
        first_copy_len = buffer_bytes_available;
    }
    /* Handle wrap */
    if ( new_tail_index >= APP_HIGHLEVEL_RX_BUFFER_SIZE ) {
        first_copy_len = APP_HIGHLEVEL_RX_BUFFER_SIZE - AppRxBufferTailIndex;
        second_copy_len = *Len - first_copy_len;
        new_tail_index = second_copy_len;
    }
    /* Copy data */
    memcpy(&AppHighLevelRxBuffer[AppRxBufferTailIndex],Buf,first_copy_len);
    AppRxBufferTailIndex += first_copy_len;
    if ( second_copy_len > 0 ) {
        memcpy(&AppHighLevelRxBuffer[0],Buf + first_copy_len, second_copy_len);
        AppRxBufferTailIndex = second_copy_len;
    }
    /* Enable reception of next packet */
    USBD_CDC_ReceivePacket(hUsbDevice_0);
    return (USBD_OK);
  /* USER CODE END 7 */ 
}

uint8_t CDC_Receive_Peek() {
	uint8_t character;
	if ( CDC_Receive_Available() == 0) {
		return -1;
	}
	character = AppHighLevelRxBuffer[AppRxBufferHeadIndex];
	return character;
}

uint8_t CDC_Receive_Read() {
	uint8_t character;
	if ( CDC_Receive_Available() == 0) {
		return -1;
	}
	character = AppHighLevelRxBuffer[AppRxBufferHeadIndex];
	//HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
	if ( AppRxBufferHeadIndex >= APP_HIGHLEVEL_RX_BUFFER_SIZE - 1) {
		AppRxBufferHeadIndex = 0;
	} else {
		AppRxBufferHeadIndex++;
	}
	//HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
	return character;
}

/**
  * @brief  CDC_Transmit_FS
  *         Data send over USB IN endpoint are sent over CDC interface 
  *         through this function.           
  *         @note
  *         
  *                 
  * @param  Buf: Buffer of data to be send
  * @param  Len: Number of data to be send (in bytes)
  * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;

  if ( hUsbDevice_0 == NULL ) return USBD_FAIL;
  if ( hUsbDevice_0->dev_state != USBD_STATE_CONFIGURED ) return USBD_FAIL;
  USBD_CDC_HandleTypeDef *pCDC =
          (USBD_CDC_HandleTypeDef *)hUsbDevice_0->pClassData;

  /* Determine if transfer is already in progress.   */
  /* If transfer in progress is stuck in that state, */
  /* then reset the interface and try again.         */

  if ( pCDC->TxState != 0 ) {
      return USBD_BUSY;
  }

  usb_last_tx_not_busy_timestamp = HAL_GetTick();

   /* USER CODE BEGIN 8 */
	if (Len > APP_TX_DATA_SIZE)
	{
		int offset;
		for (offset = 0; offset < Len; offset++)
		{
			int todo = MIN(APP_TX_DATA_SIZE,
						   Len - offset);
			result = CDC_Transmit_FS(Buf + offset, todo);
			if ( ( result != USBD_OK ) && ( result != USBD_BUSY ) ) {
				/* Error:  Break out now */
				return result;
			}
		}

		return USBD_OK;
	}

	pCDC = (USBD_CDC_HandleTypeDef *)hUsbDevice_0->pClassData;

	uint32_t tx_completion_wait_timestamp = HAL_GetTick();
	while(pCDC->TxState)
	{
	    if ( HAL_GetTick() - tx_completion_wait_timestamp > (uint32_t)100 ) {
	        /* Timeout */
	        return USBD_BUSY;
	    }
	} //Wait for previous transfer to complete

	int i;
	for ( i = 0; i < Len; i++ ) {
		UserLowLevelTxBufferFS[i] =  Buf[i];
	}
	USBD_CDC_SetTxBuffer(hUsbDevice_0, &UserLowLevelTxBufferFS[0], Len);
	result = USBD_CDC_TransmitPacket(hUsbDevice_0);

	/* USER CODE END 8 */
	return result;
}

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

