// ============================================================
// 文件名: element.h
// 功能说明: 赛道元素处理模块头文件
// 声明十字交叉、跷跷板、路障等特殊赛道元素的识别与处理函数。
// ============================================================

#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <stdint.h>

extern int16_t in_time;

void element_judge(void);
void straight_judge(void);
void crossroads_judge(void);
void crossroads_out_judge(void);
void teeterboard_judge(void);
void teeterboard_out_judge(void);
void cask_judge(void);
void cask_out_judge(void);

#endif
