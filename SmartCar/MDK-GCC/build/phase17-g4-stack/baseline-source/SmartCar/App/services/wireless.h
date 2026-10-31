/**
 * @file wireless.h
 * @brief App 原串口调参和日志命令；仅停车前台解析，协议逐字保持。
 * @details 声明无线调参初始化与轮询接口；完整命令、副作用和持久化约束位于 wireless.c。
 * @note 移植来源：Sirius20260718/project/code/config.c。
 * @note STC AI8051U 省赛快照（2026-07-18）；原算法未单列作者或独立许可证。
 * @note 保留源工程出处，关联 Phase 13/14/15 规约。
 */
#ifndef SMARTCAR_APP_WIRELESS_H
#define SMARTCAR_APP_WIRELESS_H

/**
 * @brief 处理原串口调参和日志命令。
 * @note 仅停车前台调用；导出/擦除可能阻塞。保持 d/c、vp/vi/xp/xd、数字顺序、帧长及逐字回显。 完整契约见 wireless.c::Wireless_Adjust。
 */
void Wireless_Adjust(void);

#endif /* SMARTCAR_APP_WIRELESS_H */
