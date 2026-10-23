/**
 * @file legacy_element.h
 * @brief App 旧元素判断保留模块；当前活动双模式调度不接入这些入口。
 * @details 声明赛道元素判定和辅助状态接口；完整条件与休眠边界位于 legacy_element.c。
 * @note 移植来源：Sirius20260718/project/code/element.h，STC AI8051U 省赛快照（2026-07-18）。
 * @note 源算法文件未单列作者及独立许可证；保留原出处，关联 Phase 13/15 规约。
 */
#ifndef SMARTCAR_APP_CONTROL_LEGACY_ELEMENT_H
#define SMARTCAR_APP_CONTROL_LEGACY_ELEMENT_H

#include <stdint.h>

/**
 * @brief 保留旧元素汇总调用壳及原调用边。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见 legacy_element.c::Element_Judge。
 */
void Element_Judge(void);

/**
 * @brief 保留源工程空直道判断入口。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见
 *     legacy_element.c::Element_JudgeStraight。
 */
void Element_JudgeStraight(void);

/**
 * @brief 按原电感和与对称度判断十字进入。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见
 *     legacy_element.c::Element_JudgeCrossroads。
 */
void Element_JudgeCrossroads(void);

/**
 * @brief 按原电感和判断十字退出。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见
 *     legacy_element.c::Element_JudgeCrossroadsExit。
 */
void Element_JudgeCrossroadsExit(void);

/**
 * @brief 保留跷跷板进入判断与 int16_t 计时赋值。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见
 *     legacy_element.c::Element_JudgeTeeterboard。
 */
void Element_JudgeTeeterboard(void);

/**
 * @brief 保留跷跷板信号和计时两个独立退出判断。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见
 *     legacy_element.c::Element_JudgeTeeterboardExit。
 */
void Element_JudgeTeeterboardExit(void);

/**
 * @brief 保留按 Y 角度进入路障的旧判断。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见 legacy_element.c::Element_JudgeCask。
 */
void Element_JudgeCask(void);

/**
 * @brief 保留按 Y 角度退出路障的旧判断。
 * @note 保留来源于 STC 省赛快照的旧入口，当前 App 活动双模式调度不接入；不启用或删除休眠代码。 完整契约见
 *     legacy_element.c::Element_JudgeCaskExit。
 */
void Element_JudgeCaskExit(void);

#endif
