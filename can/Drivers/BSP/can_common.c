#include "can_common.h"

DawnCanTypeDef DawnCan = {0};

// 标准帧 (11-bit) 接收列表
static const uint32_t CANFD_RxStdIdList[CANFD_RX_STD_ID_COUNT] = {
    0x7FF,  // 广播指令
    0x080,
};
// 扩展帧 (29-bit) 接收列表
static const uint32_t CANFD_RxExtIdList[1] = {
    0x00000001,  // 扩展帧测试 ID
};

/**
  * @brief  配置 FDCAN 接收滤波器
  * @note   配置标准帧 FIFO0、扩展帧 FIFO1、CANopen SDO 滤波器、
  *         电机专属 ID / MIT / PID 控制帧滤波器，未匹配帧全部拒绝
  */
static void Dawn_CANFD_ConfigFilter(void)
{
    FDCAN_FilterTypeDef sFilterConfig;
    uint8_t i;

    // ---- 标准帧 (11-bit ID) 滤波器 → FIFO 0  sailisi协议----
    for (i = 0; i < CANFD_RX_STD_ID_COUNT && i < hfdcan2.Init.StdFiltersNbr; i++)
    {
        sFilterConfig.IdType = FDCAN_STANDARD_ID;
        sFilterConfig.FilterIndex = i;
        sFilterConfig.FilterType = FDCAN_FILTER_MASK;
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        sFilterConfig.FilterID1 = CANFD_RxStdIdList[i];  //过滤表配置

        sFilterConfig.FilterID2 = 0x7FF;  // 全部 11 位精确匹配

        if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig) != HAL_OK)
            Error_Handler();
    }
    // ---- 扩展帧 (29-bit ID) 滤波器 → FIFO 1  测试用----
    for (i = 0; i < CANFD_RX_EXT_ID_COUNT && i < hfdcan2.Init.ExtFiltersNbr; i++)
    {
        sFilterConfig.IdType = FDCAN_EXTENDED_ID;
        sFilterConfig.FilterIndex = i;
        sFilterConfig.FilterType = FDCAN_FILTER_MASK;
        sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
        sFilterConfig.FilterID1 = CANFD_RxExtIdList[i];  //过滤表配置

        sFilterConfig.FilterID2 = 0x1FFFFFFF;  // 全部 29 位精确匹配

        if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig) != HAL_OK)
            Error_Handler();
    }
    // ---- 全局滤波器: 未匹配帧全部拒绝 ----
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan2,

        FDCAN_REJECT,  // 未匹配标准帧 → 拒绝

        FDCAN_REJECT,  // 未匹配扩展帧 → 拒绝

        FDCAN_FILTER_REMOTE,  // 拒绝标准远程帧

        FDCAN_FILTER_REMOTE);  // 拒绝扩展远程帧
}
/**
  * @brief  启动 FDCAN 通信
  * @note   配置 TDC 发送延迟补偿、RX FIFO 覆盖模式、中断线路由，最后启动并激活中断
  */
static void Dawn_CANFD_Start(void)
{
    // ---- 发送延迟补偿 (TDC) — CAN FD BRS 5Mbps 必需 ----

    HAL_FDCAN_ConfigTxDelayCompensation(&hfdcan2, 8, 2);

    HAL_FDCAN_EnableTxDelayCompensation(&hfdcan2);

    // ---- RX FIFO 阻塞模式 ----
    // STM32G4 的每个硬件 RX FIFO 固定只有 3 帧。总线过载时宁可丢新帧，
    // 也不使用 HAL 覆盖路径，避免 FIFO 满时计算出非法的 RAM 索引。
    HAL_FDCAN_ConfigRxFifoOverwrite(&hfdcan2, FDCAN_RX_FIFO0, FDCAN_RX_FIFO_BLOCKING);
    HAL_FDCAN_ConfigRxFifoOverwrite(&hfdcan2, FDCAN_RX_FIFO1, FDCAN_RX_FIFO_BLOCKING);

    // ---- 中断线路由 ----
    HAL_FDCAN_ConfigInterruptLines(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, FDCAN_INTERRUPT_LINE0);
    HAL_FDCAN_ConfigInterruptLines(&hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, FDCAN_INTERRUPT_LINE1);

    //    HAL_FDCAN_Stop(&hfdcan2);
    //    HAL_Delay(1000);
    // ---- 启动 FDCAN ----
    if (HAL_FDCAN_Start(&hfdcan2) != HAL_OK)
        Error_Handler();

    // ---- 激活 RX 中断通知 ----
    HAL_FDCAN_ActivateNotification(&hfdcan2,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
}
/**
  * @brief  字节数 → FDCAN DLC 编码
  * @param  bytes : 实际数据字节数 (1~64)
  * @retval FDCAN DLC 枚举值 (≤8 直接返回，>8 映射到 12/16/20/24/32/48/64)
  */
uint32_t Dawn_CANFD_BytesToDLC(uint8_t bytes)
{
    if (bytes <= 8)
        return (uint32_t)bytes;
    if (bytes <= 12)
        return FDCAN_DLC_BYTES_12;
    if (bytes <= 16)
        return FDCAN_DLC_BYTES_16;
    if (bytes <= 20)
        return FDCAN_DLC_BYTES_20;
    if (bytes <= 24)
        return FDCAN_DLC_BYTES_24;
    if (bytes <= 32)
        return FDCAN_DLC_BYTES_32;
    if (bytes <= 48)
        return FDCAN_DLC_BYTES_48;
    return FDCAN_DLC_BYTES_64;
}

/**
  * @brief  通过 FDCAN 发送一帧 (使用全局 Tx FIFO 缓冲区)
  * @param  id     : CAN 标识符 (11-bit 或 29-bit)
  * @param  idType : FDCAN_STANDARD_ID 或 FDCAN_EXTENDED_ID
  * @param  data   : 发送数据指针 (最大 64 字节)
  * @param  len    : 实际数据字节数
  * @param  isFD   : 1 = CAN FD 帧 (BRS 使能)，0 = 传统 CAN 帧
  * @retval HAL 状态 (HAL_OK / HAL_ERROR / HAL_BUSY / HAL_TIMEOUT)
  */
HAL_StatusTypeDef Dawn_CANFD_SendMessage(uint32_t id, uint32_t idType, uint8_t *data, uint8_t len, uint8_t isFD)
{
    uint8_t actualLen = (len > 64) ? 64 : len;

    DawnCan.tx.header.DataLength = Dawn_CANFD_BytesToDLC(actualLen);
    DawnCan.tx.header.Identifier = id;
    DawnCan.tx.header.IdType = idType;
    DawnCan.tx.header.TxFrameType = FDCAN_DATA_FRAME;
    DawnCan.tx.header.FDFormat = isFD ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;

    DawnCan.tx.header.BitRateSwitch = isFD ? FDCAN_BRS_ON : FDCAN_BRS_OFF;  //FDCAN_BRS_ON

    DawnCan.tx.header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;  //FDCAN_ESI_PASSIVE
    DawnCan.tx.header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    DawnCan.tx.header.MessageMarker = 0;

    for (uint8_t i = 0; i < actualLen; i++)
    {
        DawnCan.tx.data[i] = data[i];
    }

    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &DawnCan.tx.header, DawnCan.tx.data);
}

/* ==================== 公开函数 ==================== */

/**
  * @brief  CAN 统一初始化入口
  * @note   依次调用滤波器配置和 FDCAN 启动
  */
void Dawn_CANFD_Init(void)
{
    Dawn_CANFD_ConfigFilter();
    Dawn_CANFD_Start();
}