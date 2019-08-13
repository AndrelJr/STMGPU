#include <string.h>
#include <stdlib.h>

#ifdef STM32F40XX
#include <stm32f4xx.h>
#else
#include <stm32f10x.h>
#endif /* STM32F40XX */

#include <memHelper.h>
#include "uart.h"

// ------------------------------------------------------------ //

#ifdef USART1_USE_PROTOCOL_V1
#define MOVE_DMA_HEAD() (rx_buffer.head = SERIAL_BUFFER_SIZE - DMA2_Stream5->NDTR)
#define RX_AVALIABLE   (((uint16_t)(SERIAL_BUFFER_SIZE + rx_buffer.head - rx_buffer.tail)) % SERIAL_BUFFER_SIZE)

#define DMA2_HIFCR_MASK  (uint32_t)0x0F7D0F7D
#endif

#ifdef USART1_USE_PROTOCOL_V0
#define RX_AVALIABLE   (((uint16_t)(SERIAL_BUFFER_SIZE + rx_buffer.head - rx_buffer.tail)) % SERIAL_BUFFER_SIZE)
#endif

#define MIN_CUT_DATA_SIZE   3

// ------------------------------------------------------------ //

ring_buffer_t rx_buffer = { 0, 0 };
uint8_t rxBuffer[SERIAL_BUFFER_SIZE] = { 0 };

uint8_t *pDestBuf = 0;

// ------------------------------------------------------------ //

#ifdef USART1_USE_PROTOCOL_V1
uint8_t ucSearchSyncSeq(void)
{
  uint8_t ucRes = 0;

  do {
    for (uint16_t i =0; i < SERIAL_BUFFER_SIZE-1; i++) {
      if ( (rxBuffer[i] == 0x42) && (rxBuffer[i+1] == 0xDD) ) {
        ucRes = 1;
        break;
      }
    }
  }while (!ucRes);

  return ucRes;
}
#endif /* USART1_USE_PROTOCOL_V1 */

// ------------------------------------------------------------ //
// apply pointer to end buffer
void setBufferPointer_UART1(void *pBuf)
{
  pDestBuf = (uint8_t*) pBuf;
}

uint16_t dataAvailable_UART1(void)
{
#ifdef USART1_USE_PROTOCOL_V1
  MOVE_DMA_HEAD();
  return RX_AVALIABLE;
#endif

#ifdef USART1_USE_PROTOCOL_V0
  return RX_AVALIABLE;
#endif

}

uint8_t readData8_UART1(void)
{
  return rxBuffer[rx_buffer.tail++]; // no calculation need, just overflow
}

void cutData(uint8_t *pDest, uint16_t size)
{
   do {
    do {
#ifdef USART1_USE_PROTOCOL_V1
      MOVE_DMA_HEAD();
#endif
    } while (rx_buffer.tail == rx_buffer.head); // wait for something in buf
    *pDest++ = rxBuffer[rx_buffer.tail++];
  } while (--size);
}

uint8_t waitCutByte_UART1(void)
{
#ifdef USART1_USE_PROTOCOL_V1
  uint8_t byteData = 0;
  cutData((uint8_t*)&byteData, 1);
  return byteData;
#endif
#ifdef USART1_USE_PROTOCOL_V0
  while (!RX_AVALIABLE)
    ; // wait for something in buf
  return rxBuffer[rx_buffer.tail++];
#endif
}

uint16_t waitCutWord_UART1(void)
{
  uint16_t wordData = 0;
  cutData((uint8_t*) &wordData, 2);
  return wordData;
}

// this one use preinited pointer to end data buffer
void waitCutpBuf_UART1(uint16_t size)
{
#ifdef USART1_USE_PROTOCOL_V1
  MOVE_DMA_HEAD();
#endif

  if ((RX_AVALIABLE >= size) && (size >= MIN_CUT_DATA_SIZE)) {
    if (rx_buffer.tail < ((SERIAL_BUFFER_SIZE - 1) - size)) {
      memcpy32(pDestBuf, &rxBuffer[rx_buffer.tail], size);
      rx_buffer.tail += size;

      return;
    }
  }

  cutData(pDestBuf, size);
}

void waitCutBuf_UART1(void *dest, uint16_t size)
{
#ifdef USART1_USE_PROTOCOL_V1
  MOVE_DMA_HEAD();
#endif

  if ((RX_AVALIABLE >= size) && (size >= MIN_CUT_DATA_SIZE)) {
    if (rx_buffer.tail < ((SERIAL_BUFFER_SIZE - 1) - size)) {
      memcpy32(dest, &rxBuffer[rx_buffer.tail], size);
      rx_buffer.tail += size;

      return;
    }
  }

  cutData(dest, size);
}

#if 0 // not yet tested
// in theory returning pointer is faster than memcpy32
void *waitCutPtrBuf_UART1(uint16_t size)
{
  void *dest = &pDestBuf;
  if((RX_AVALIABLE >= size) && (size >= MIN_CUT_DATA_SIZE)) {
    if(rx_buffer.tail < ((SERIAL_BUFFER_SIZE-1) - size)) {
      dest = &rxBuffer[rx_buffer.tail];
      rx_buffer.tail += size;

      return dest;
    }
  }
  // not enough data or near to end of buffer
  cutData(dest, size);
  return dest;
}
#endif

void fflush_UART1(void)
{
#ifdef USART1_USE_PROTOCOL_V1
  rx_buffer.head = rx_buffer.tail = SERIAL_BUFFER_SIZE - DMA2_Stream5->NDTR; // get current position from DMA
#endif
#ifdef USART1_USE_PROTOCOL_V0
  rx_buffer.head = rx_buffer.tail = 0;
#endif
}

void fflush_buffer_USART1(void)
{
  memset32(rxBuffer, 0x00, SERIAL_BUFFER_SIZE / 4);
}

inline void sendData8_UART1(uint8_t data)
{
  while (!(USART1->SR & USART_SR_TC))
    ;
  USART1->DR = data;
}

inline void sendArrData8_UART1(void *src, uint32_t size)
{
  for (uint32_t count = 0; count < size; count++) {
    sendData8_UART1(((uint8_t*) src)[count]);
  }
}

#ifdef USART1_USE_PROTOCOL_V1
void init_UART1_DMA_Rx(void)
{
  DMA_InitTypeDef DMA_UART_Rx_settings;

#if defined(STM32F10X_MD) || defined(STM32F10X_HD)

#else
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

  DMA_UART_Rx_settings.DMA_BufferSize = sizeof(rxBuffer);
  DMA_UART_Rx_settings.DMA_Channel = DMA_Channel_4;
  DMA_UART_Rx_settings.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_UART_Rx_settings.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_UART_Rx_settings.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_UART_Rx_settings.DMA_Memory0BaseAddr = (uint32_t)&rxBuffer[0];
  DMA_UART_Rx_settings.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_UART_Rx_settings.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_UART_Rx_settings.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_UART_Rx_settings.DMA_Mode = DMA_Mode_Circular;
  DMA_UART_Rx_settings.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
  DMA_UART_Rx_settings.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_UART_Rx_settings.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_UART_Rx_settings.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_UART_Rx_settings.DMA_Priority = DMA_Priority_Medium;

  DMA_ITConfig(DMA2_Stream5, DMA_IT_HT, ENABLE);
  DMA_ITConfig(DMA2_Stream5, DMA_IT_TC, ENABLE);
  DMA_Init(DMA2_Stream5, &DMA_UART_Rx_settings);

  // Enable DMA2 channel IRQ Channel
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_EnableIRQ(DMA2_Stream5_IRQn);

  USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
  DMA_Cmd(DMA2_Stream5, ENABLE);// shot DMA to transfer;
#endif
}
#endif

#ifdef USART1_USE_PROTOCOL_V0
void init_UART1_Rx(void)
{
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable IRQ

  //setup NVIC for USART IRQ Channel
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
#endif

void init_UART1(uint32_t baud)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

  GPIO_InitTypeDef GPIO_InitStruct;

#if defined(STM32F10X_MD) || defined(STM32F10X_HD)
  // set PA10 as input UART (RxD)
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIOA, &GPIO_InitStruct);

  // set PA9 as output UART (TxD)
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
  // set PA10 as input UART (RxD)
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStruct);

  // set PA9 as output UART (TxD)
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
#endif

  // setup UART1
  USART_InitTypeDef USART_InitStruct;
  USART_InitStruct.USART_BaudRate = baud;
  USART_InitStruct.USART_WordLength = USART_WordLength_8b;
  USART_InitStruct.USART_StopBits = USART_StopBits_1;
  USART_InitStruct.USART_Parity = USART_Parity_No;
  USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);
  USART_Init(USART1, &USART_InitStruct);        // apply settings

#ifdef USART1_USE_PROTOCOL_V1
  init_UART1_DMA_Rx();
#endif
#ifdef USART1_USE_PROTOCOL_V0
  init_UART1_Rx();
#endif

  USART_Cmd(USART1, ENABLE);                    // no comment
}

#ifdef USART1_USE_PROTOCOL_V0
void USART1_IRQHandler(void) // get data as fast as possible!
{
  //if(USART1->SR & USART_SR_RXNE ) {
  rxBuffer[rx_buffer.head++] = USART1->DR;
//  }
}
#endif
#ifdef USART1_USE_PROTOCOL_V1
void DMA2_Stream5_IRQHandler(void)
{
  if (DMA2->HISR & (DMA2_HIFCR_MASK & DMA_IT_HTIF5) ) { // DMA_GetITStatus(DMA2_Stream5, DMA_IT_HTIF5)
    DMA2->HIFCR = (uint32_t)(DMA2_HIFCR_MASK & DMA_IT_HTIF5);// DMA_ClearITPendingBit(DMA2_Stream5, DMA_IT_HTIF5);
    vCallbackFrom_USART1_IRQ();// set BSY flag
    return;
  }

  if (DMA2->HISR & (DMA2_HIFCR_MASK & DMA_IT_TCIF5) ) { // DMA_GetITStatus(DMA2_Stream5, DMA_IT_TCIF5)
    DMA2->HIFCR = (uint32_t)(DMA2_HIFCR_MASK & DMA_IT_TCIF5);// DMA_ClearITPendingBit(DMA2_Stream5, DMA_IT_TCIF5);
    vCallbackFrom_USART1_IRQ();// set BSY flag
    return;
  }
}
#endif
