// ============================================================
// 文件名: element.c
// 功能说明: 赛道元素处理模块（预留扩展）
// 本文件为预留模块，可用于实现十字交叉、三岔路、坡道等
// 特殊赛道元素的识别与处理策略。
// ============================================================

#include "config.h"

int16 in_time = 0;

void element_judge(void)
{
	L_roundabout_judge();
	R_roundabout_judge();
	crossroads_judge();
	teeterboard_judge();
}

void straight_judge(void)
{
	
	
	
}

void crossroads_judge(void)
{
	if( adc_filted[1] + adc_filted[2] > 2800 && symmetry_y < 25)
	{
		
		kernel_state = KERNEL_CROSSROADS;

	}
}

void crossroads_out_judge(void)
{
	if( adc_filted[1] + adc_filted[2] < 2800 )
	{
		kernel_state = KERNEL_TRACKING;

	}
}

void teeterboard_judge(void)
{
	if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] < 800 )
	{
		kernel_state = KERNEL_TEETERBOARD;
		in_time = time;
	}
}

void teeterboard_out_judge(void)
{
	if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 1000 )
	{
		kernel_state = KERNEL_TRACKING;
	}
	
	if( (time - in_time) > 300 ) 
	{
		kernel_state = KERNEL_TRACKING;
		
	}
}
void cask_judge(void)
{
	if(angle_y > 60)
	{
		kernel_state = KERNEL_CASK;	
	}
}
void cask_out_judge(void)
{
	if(angle_y < 20)
	{
		kernel_state = KERNEL_TRACKING;	
	}
}