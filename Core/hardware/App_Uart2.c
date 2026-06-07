// UART2 初始化與 Echo 實現

#include "App_Uart2.h"

/* UART2 handle */
UART_HandleTypeDef huart2;

/* DMA handles for USART2 */
static DMA_HandleTypeDef hdma_usart2_tx;
static DMA_HandleTypeDef hdma_usart2_rx;

/* RX byte for interrupt-based async receive */
volatile uint8_t uart2_rx_byte;

/* ── USART2 GPIO 初始化 ── */
static void Uart2_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA2 — USART2_TX */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA3 — USART2_RX */
    GPIO_InitStruct.Pin       = GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* ── DMA 初始化（DMA1 Ch7 TX, Ch6 RX） ── */
static void Uart2_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA1 通道 7 — USART2_TX */
    hdma_usart2_tx.Instance                 = DMA1_Channel7;
    hdma_usart2_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_usart2_tx);
    __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);

    /* DMA1 通道 6 — USART2_RX */
    hdma_usart2_rx.Instance                 = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode                = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_usart2_rx);
    __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);
}

/* ── USART2 NVIC 初始化 ── */
static void Uart2_NVIC_Init(void)
{
    HAL_NVIC_SetPriority(USART2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/* ── UART2 初始化（公開接口） ── */
void Uart2_init(void)
{
    Uart2_GPIO_Init();
    Uart2_DMA_Init();

    /* USART2 外設配置：115200 8N1 全雙工 */
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);

    Uart2_NVIC_Init();

    /* 啟動中斷方式非同步接收（1 byte） */
    HAL_UART_Receive_IT(&huart2, (uint8_t*)&uart2_rx_byte, 1);
}

/******************************************************************************/
/*            USART2 Interrupt Handler (overrides weak default)               */
/******************************************************************************/
void USART2_IRQHandler(void)
{
    //(HAL 库判断中断类型)
    HAL_UART_IRQHandler(&huart2); 
}

/******************************************************************************/
/*            RX Complete Callback — Echo 回送                                */
/******************************************************************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        /* 將收到的位元組經 TX 發送回去（Echo） */
        HAL_UART_Transmit_IT(&huart2, (uint8_t*)&uart2_rx_byte, 1);
        /* 重新啟用 RX 中斷接收下一個位元組 */
        HAL_UART_Receive_IT(&huart2, (uint8_t*)&uart2_rx_byte, 1);
    }
}
