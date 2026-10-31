/**
 * @file legacy_roundabout.h
 * @brief App 环岛配置与旧入口；配置供电磁影子机使用，旧判断函数当前不接入。
 * @details 声明环岛方向、阶段状态和更新接口；完整转移条件与输出边界位于 legacy_roundabout.c。
 * @note 移植来源：Sirius20260718/project/code/roundabout.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_CONTROL_LEGACY_ROUNDABOUT_H
#define SMARTCAR_APP_CONTROL_LEGACY_ROUNDABOUT_H

#include <stdint.h>
#include "control/tracking.h"

#define ROUNDABOUT_STATE_NORMAL 0      /* 正常循迹状态 */
#define ROUNDABOUT_ISLAND_LPREENTER 1  /* 左预进岛 */
#define ROUNDABOUT_ISLAND_RPREENTER 2  /* 右预进岛 */
#define ROUNDABOUT_ISLAND_TURN_LEFT 3  /* 左打角进岛 */
#define ROUNDABOUT_ISLAND_TURN_RIGHT 4 /* 右打角进岛 */
#define ROUNDABOUT_ISLAND_IN 5         /* 岛中（环岛内部循迹） */
#define ROUNDABOUT_ISLAND_EXIT 6       /* 出岛 */
#define ROUNDABOUT_ISLAND_LENTER 9     /* 走距离准备左进岛（预留） */
#define ROUNDABOUT_ISLAND_RENTER 10    /* 走距离准备右进岛（预留） */
#define ROUNDABOUT_ISLAND_OUT 11       /* 走距离准备出岛 */
#define ROUNDABOUT_ISLAND_TURN_TURN_LEFT 12

/**
 * @brief 执行旧距离驱动环岛分支。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见 legacy_roundabout.c::Roundabout_Update。
 */
void Roundabout_Update(void);

/**
 * @brief 按原横和阈值及障碍距离判断左预入环。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgePreentryLeft。
 */
void Roundabout_JudgePreentryLeft(void);

/**
 * @brief 按原四路和/横和阈值判断右预入环。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgePreentryRight。
 */
void Roundabout_JudgePreentryRight(void);

/**
 * @brief 累计脉冲大于 5000 时清旧预入环状态。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgePreentryExit。
 */
void Roundabout_JudgePreentryExit(void);

/**
 * @brief 保留源左入环空壳。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgeEntryLeft。
 */
void Roundabout_JudgeEntryLeft(void);

/**
 * @brief 按原竖和、方向及路障条件判断右入环。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgeEntryRight。
 */
void Roundabout_JudgeEntryRight(void);

/**
 * @brief 累计脉冲达到原入环距离时转打角状态。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见 legacy_roundabout.c::Roundabout_JudgeAhead。
 */
void Roundabout_JudgeAhead(void);

/**
 * @brief 累计脉冲大于 13000 时转环内。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgeEntered。
 */
void Roundabout_JudgeEntered(void);

/**
 * @brief 角度误差绝对值小于 5 度时转环内。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgeEnteredAgain。
 */
void Roundabout_JudgeEnteredAgain(void);

/**
 * @brief 保留源预出环空壳。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见
 *     legacy_roundabout.c::Roundabout_JudgePreExit。
 */
void Roundabout_JudgePreExit(void);

/**
 * @brief 累计脉冲大于 32000 时转出环。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见 legacy_roundabout.c::Roundabout_JudgeExit。
 */
void Roundabout_JudgeExit(void);

/**
 * @brief 累计脉冲大于原出环距离时清环岛状态。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见 legacy_roundabout.c::Roundabout_JudgeExited。
 */
void Roundabout_JudgeExited(void);

/**
 * @brief 保留源赛道判断空壳。
 * @note 当前 App 运行链不接入；保留全部旧条件、状态更新和调用边，不以注释说明启用。 完整契约见 legacy_roundabout.c::Roundabout_Judge。
 */
void Roundabout_Judge(void);

/**
 * @brief 复位环岛状态和方向标志。
 * @note 起跑准备调用，不改距离与角度标定参数。 完整契约见 legacy_roundabout.c::Roundabout_ResetRunState。
 */
void Roundabout_ResetRunState(void);

#endif
