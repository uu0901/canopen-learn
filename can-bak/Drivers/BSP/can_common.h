#ifndef CAN_COMMON_H
#define CAN_COMMON_H

#include "fdcan.h"

#define CANFD_RX_STD_ID_COUNT 2
#define CANFD_RX_EXT_ID_COUNT 1
typedef struct
{
    uint32_t id;       // 接收到的 CAN ID
    uint8_t data[64];  // 接收数据 (CAN FD 最大 64 字节)
    uint8_t len;       // 数据长度
    uint8_t isNew;     // 新消息标志：1 = 有新消息待处理
} CANFD_RxMsg_t;
typedef struct
{
    CANFD_RxMsg_t fifo0;
    CANFD_RxMsg_t fifo1;
} DawnCanRxTypeDef;

typedef struct
{
    FDCAN_TxHeaderTypeDef header;
    uint8_t data[64];
} DawnCanTxTypeDef;
typedef struct
{
    DawnCanRxTypeDef rx;
    DawnCanTxTypeDef tx;

} DawnCanTypeDef;

void Dawn_CANFD_Init(void);
HAL_StatusTypeDef Dawn_CANFD_SendMessage(uint32_t id, uint32_t idType, uint8_t *data, uint8_t len, uint8_t isFD);
#endif /* DAWN_CAN_COMMON_H */