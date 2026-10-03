// ============================================================
// 文件名: roundabout.h
// 功能说明: 环岛（roundabout）状态宏与变量声明头文件
// 定义环岛状态机各阶段常量，以及环岛控制相关的外部变量和函数声明。
// ============================================================

#ifndef _ROUNFABOUT_H_
#define _ROUNDABOUT_H_


#define STATE_NORMAL 			0					// 正常循迹状态
#define ISLAND_LPREENTER 		1    				// 左预进岛
#define ISLAND_RPREENTER 		2    				// 右预进岛
#define ISLAND_TURN_LEFT 		3   				// 左打角进岛
#define ISLAND_TURN_RIGHT 		4       			// 右打角进岛
#define ISLAND_IN          		5					// 岛中（环岛内部循迹）
#define ISLAND_EXIT  			6					// 出岛
#define ISLAND_LENTER 			9   				// 走距离准备左进岛（预留）
#define ISLAND_RENTER 			10   				// 走距离准备右进岛（预留）
#define ISLAND_OUT          	11      			// 走距离准备出岛
#define ISLAND_TURN_TURN_LEFT	12

extern uint8 roundabout_state;						// 当前赛道元素状态机状态
//extern uint8 round_flag;						// 环岛进出岛判断标志（1表示正在处理环岛）

extern int8 sign_round;

extern int16 enter_angle1;						// 环岛1入环初始打角角度
extern int16 enter_angle2;
//extern int16 pre_out_angle1; 					// 环岛1预出环判断角度阈值
extern int16 out_angle1;	 					// 环岛1出环目标角度


void roundabout(void);

void L_reroundabout_judge(void);
void R_reroundabout_judge(void);// 赛道元素状态机主分支
void reroundabout_out_judge(void);

void L_roundabout_judge(void);					// 左环岛入环条件判断
void R_roundabout_judge(void);

void ahead_judge(void);						// 左环岛预入环阶段距离判断
void entered_judge(void);						// 入环打角完成判断
void entered_entered_judge(void);

void pre_out_judge(void);						// 环岛预出环判断
void exit_judge(void);							// 出环打角完成判断
void outed_judge(void);							// 出环完成判断
void judge(void);	





#endif