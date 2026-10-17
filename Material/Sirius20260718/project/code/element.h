// ============================================================
// 文件名: element.h
// 功能说明: 赛道元素处理模块头文件（预留扩展）
// 本文件为预留接口，可用于声明十字交叉、三岔路、坡道等
// 特殊赛道元素的识别与处理函数。
// ============================================================

#ifndef _ELEMENT_H_
#define _ELEMENT_H_

extern int16 in_time;


void element_judge(void);
void crossroads_judge(void);
void crossroads_out_judge(void);
void teeterboard_judge(void);
void teeterboard_out_judge(void);
void cask_judge(void);
void cask_out_judge(void);

#endif
