// ============================================================
// 文件名: roundabout.c
// 功能说明: 环岛（roundabout）识别与控制模块
// 实现基于电感阈值和角度积分的环岛状态机，
// 包括预入环、打角入环、环内循迹、预出环、出环等阶段。
// ============================================================

#include "config.h"


uint8 roundabout_state = STATE_NORMAL;      // 当前赛道元素状态机状态
uint8 L_round_flag = 0;                    // 环岛进出岛判断标志（1表示正在处理环岛）
uint8 R_round_flag = 0;                    // 环岛进出岛判断标志（1表示正在处理环岛）

int8 sign_round = 1;

uint16 enter_distance1 = 8000;           // 环岛1入环前直行距离
uint16 out_distance1 = 10000;             // 环岛1出环后直行距离

int16 enter_angle1 = 30 ;                // 环岛1入环初始打角角度
int16 enter_angle2 = 60 ;
//int16 pre_out_angle1 = 280;              // 环岛1预出环判断角度阈值
int16 out_angle1 = 200;                  // 环岛1出环目标角度



void roundabout(void)
{
	switch( roundabout_state )
	{
		case ISLAND_LPREENTER:
			
//			PID_angle( 0 );	
		
			weight_x   = 20;	//15
			weight_xx  = 10;	//20
			weight_y   = 1;	//28
			weight_abs = 10;	//5
				
			base_speed = 150; 	//500--2.9
			KP_x  = 2;   		//2
			K2P_x = 0.008;		//0.008
			KD_x  = 15;			//15	
			K2D_x = 1;	  		//1
			
			track_error = get_track_error();
			track_out = PID_track();
			
			speed_control(track_out);
		
			ahead_judge();//是否满足距离	
		break;
		
		case ISLAND_TURN_LEFT:
			
//			PID_angle( sign_round * enter_angle1 );
//			target_speed_R = 0;
//			target_speed_L = 0;
		
			speed_control(-30);
		
			entered_judge();// 检测距离是否完成
		break;
		
//		case ISLAND_TURN_TURN_LEFT:
//			
////			PID_angle( sign_round * enter_angle2 );
//		
//			target_speed_R = 0;
//			target_speed_L = 0;
//			
//		
//			entered_entered_judge();// 检测打角是否完成
//		break;
		
		case ISLAND_IN:
			
			base_speed = 150; 	//500--2.9
			KP_x  = 2;   		//2
			K2P_x = 0.008;		//0.008
			KD_x  = 15;			//15	
			K2D_x = 1;	  		//1
		
			track_error = get_track_error();
			track_out = PID_track();
			speed_control( track_out );
		
//			target_speed_R = 0;			
//			target_speed_L = 0;
		
			//speed_control( 0 );
			
			//angle_err = out_angle1 - angle_x;
			
			exit_judge();
		break;
		
//		case ISLAND_EXIT:

//			PID_angle( sign_round * pre_out_angle1 );
//			
//			exit_judge();
//		break;
		
		case ISLAND_OUT:
			
//			PID_angle( sign_round * out_angle1 );
//			target_speed_R = 0;			
//			target_speed_L = 0;
		
			weight_x   = 20;	//15
			weight_xx  = 10;	//20
			weight_y   = 1;	//28
			weight_abs = 10;	//5
				
			base_speed = 150; 	//500--2.9
			KP_x  = 2;   		//2
			K2P_x = 0.008;		//0.008
			KD_x  = 15;			//15	
			K2D_x = 1;	  		//1
			
			track_error = get_track_error();
			track_out = PID_track();
			
			speed_control(track_out);
			
			outed_judge(); 
		break;
	}	
}


void L_reroundabout_judge(void)
{
//	if(adc_filted[0] > 1200 && adc_filted[3] > 1000 && adc_filted[1] > 300 && adc_filted[1] < 1000 &&adc_filted[2] < 200 )
//	if(adc_filted[0]  + adc_filted[3]> 2200 && track_error > 24  )
//	if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 5500 )
//	{
////		kernel_state = KERNEL_REISLAND;
//		L_round_flag = 1;
//		
//		track_out = 0;
//		angle_x = 0;
//		distance_L = 0;
//		distance_R = 0;
//		Distance = 0;
//		
//		in_time = time;
//	}
//	
//	if(adc_filted[1] > 250 && adc_filted[2] < 50 && adc_filted[0] + adc_filted[3] > 2000)
//	{
//		kernel_state = KERNEL_REISLAND;
//		L_round_flag = 1;
//		
//		track_out = 0;
		//angle_x = 0;
//	}
	if((adc_filted[0] + adc_filted[3] > 2800 ))
	{
		if( adc_filted[0] > adc_filted[3])
		{
			 L_round_flag = 1;
		}
		else 
		{
			 R_round_flag = 1;
		}
		
		cask_flag = 0;
		angle_x = 0;
		Distance = 0;
		distance_L = 0;
		distance_R = 0;
		kernel_state = KERNEL_REISLAND ;
	}
	
	if( dl1b_distance_mm < 100 )
	{
		cask_flag = 1;
	}
	
	
	
}

void R_reroundabout_judge(void)
{
//	if(adc_filted[3] > 1200 && adc_filted[0] > 1000 && adc_filted[2] > 300 && adc_filted[2] < 1000 &&adc_filted[1] < 200 )
//	if(adc_filted[0]  + adc_filted[3]> 2200 && track_error < -24  )
//	if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 5500 )
//	{
//		kernel_state = KERNEL_REISLAND;
//		R_round_flag = 1;
//		
//		track_out = 0;
//		angle_x = 0;
//		distance_L = 0;
//		distance_R = 0;
//		Distance = 0;
//		
//		in_time = time;
//	}
//	
//	if(adc_filted[2] > 250 && adc_filted[1] < 50 && adc_filted[0] + adc_filted[3] > 2000)
//	{
//		kernel_state = KERNEL_REISLAND;
//		L_round_flag = 1;
//		
//		track_out = 0;
//		angle_x = 0;
//	}
	if(( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 4500 )||(adc_filted[0] + adc_filted[3] >3000 ))
	{
		angle_x = 0;
		kernel_state = KERNEL_REISLAND ;
		
	}
	
}

void reroundabout_out_judge(void)
{
//	if(adc_filted[0] > 1200 && adc_filted[3] > 1000 && adc_filted[1] > 300 && adc_filted[1] < 1000 &&adc_filted[2] < 200 )
//	if(adc_filted[0]  + adc_filted[3]> 2200 && track_error > 24  )
	if( Distance > 5000 )
	{
		kernel_state = KERNEL_TRACKING;
		L_round_flag = 0;
		R_round_flag = 0;
		
		track_out = 0;
		angle_x = 0;
	}
}




// 函数名: L_enter_judge
// 功能: 左环岛入环条件判断
// 说明: 当电感值满足左环岛特征（左外侧、左内侧、右内侧、右外侧均较强）且未进入环岛时，
//       将状态切换为左预入环状态。
void L_roundabout_judge(void)
{
//	if((adc_filted[0] > 1500 && adc_filted[3] > 2500) && angle_y < 5 && round_flag == 0)
//	if((adc_filted[1] + adc_filted[2] < 250) && L_round_flag == 1 && !cask_flag)
//	{
//		roundabout_state = ISLAND_LPREENTER;
//		kernel_state = KERNEL_ISLAND_L;
//		
//		track_out = 0;
//		angle_x = 0;
//		distance_L = distance_R = Distance = 0;
//	}
}		


void R_roundabout_judge(void)
{
//	if((adc_filted[3] > 1500 && adc_filted[0] > 2500) && angle_y < 5 && round_flag == 0)
	if((adc_filted[1] + adc_filted[2] < 300) && R_round_flag == 1 && !cask_flag)
	{
		roundabout_state = ISLAND_LPREENTER;
		kernel_state = KERNEL_ISLAND_R;
		
		track_out = 0;
		angle_x = 0;
		distance_L = distance_R = Distance = 0;		
	}
}	




// 函数名: ahead_judge
// 功能: 左环岛预入环阶段距离判断
// 说明: 在预入环状态下直线行驶，当累计距离达到设定入环距离后，
//       切换为左转入环执行状态。
void ahead_judge(void)
{
	if( Distance >= enter_distance1 ) 
	{
		roundabout_state = ISLAND_TURN_LEFT;
		distance_L = distance_R = Distance = 0;	
	}
}

// 函数名: entered_judge
// 功能: 入环打角完成判断
// 说明: 当角度误差绝对值小于2度时，认为入环姿态已调整好，
//       切换为环岛内部循迹状态。
void entered_judge(void)
{
	
//	angle_err = enter_angle1 - angle_x;
	
//	if ( float_abs(angle_err) < 5.0 )  // 角度误差小于2度，认为入环姿态已调整好 
//	{
//		roundabout_state = ISLAND_IN;
//		
//		angle_x = 0;
//	}	
	
	if ( Distance > 13000 ) 
	{
		roundabout_state = ISLAND_IN;
		distance_L = distance_R = Distance = 0;	
	}	
	
//	if ( (time - in_time) > 1) 
//	{
//		roundabout_state = ISLAND_IN;
//	}
	
}

void entered_entered_judge(void)
{
	
//	angle_err = enter_angle1 - angle_x;
	
	if ( float_abs(angle_err) < 5.0 )  // 角度误差小于2度，认为入环姿态已调整好 
	{
		roundabout_state = ISLAND_IN;
	}	
	
//	if ( Distance > 50 ) 
//	{
//		roundabout_state = ISLAND_IN;
//	}	
//	
//	if ( (time - in_time) > 1) 
//	{
//		roundabout_state = ISLAND_IN;
//	}
	
}

// 函数名: pre_out_judge
// 功能: 环岛预出环判断
// 说明: 在环内循迹时，当车身角度超过预出环角度阈值（正向或负向），
//       认为已对准出口，切换为出环执行状态。
//void pre_out_judge(void)
//{
//	if ( float_abs(angle_x) > pre_out_angle1 ) 
//	{
//		roundabout_state = ISLAND_EXIT;
//		
//		track_out = 0; 
//	}
//}

// 函数名: exit_judge
// 功能: 出环打角完成判断
// 说明: 当角度误差绝对值小于2度时，认为出环姿态已调整好，
//       切换为出环后直行状态。
void exit_judge(void)
{
//	angle_err = out_angle1 - angle_x;
//	
//	if ( float_abs(angle_err) < 7.0 )  // 角度误差小于2度，认为出环姿态已调整好 
//	{
//		roundabout_state = ISLAND_OUT;
//		
//		distance_L = distance_R = Distance = 0;
//	}		
	
	if(Distance > 32000)
	{
		roundabout_state = ISLAND_OUT;
		distance_L = distance_R = Distance = 0;
	}
}

// 函数名: outed_judge
// 功能: 出环完成判断
// 说明: 出环后直线行驶足够距离，确认已完全离开环岛，
//       清零环岛标志并返回正常循迹状态。
void outed_judge(void)
{
	if( Distance > out_distance1 )  // 行驶足够距离，确认出环完成 
	{
		L_round_flag = 0;		
		R_round_flag = 0;	
		
		roundabout_state = STATE_NORMAL;
		kernel_state = KERNEL_TRACKING;
	}
}

//// 函数名: judge
//// 功能: 赛道元素状态机主分支（预留扩展框架）
//// 说明: 根据 current_state 分发到不同的环岛处理阶段。
////       当前主要实现左环岛流程，右环岛相关分支保留供后续扩展。
//void judge(void)
//{
//	switch (current_state)  // 赛道元素状态机主分支
//	{
//        case STATE_NORMAL:  // 正常循迹状态 0
//							
//			// 若choose_flag=1（菜单中启用环岛功能），则根据当前环岛编号调用对应的入环判断函数
//	//			if(choose_flag == 1)                      
//	//			{
//					// 环岛1：同时判断左入环和右入环条件（根据round1_flag决定哪个方向生效）
//					L_ahead_judge();		
//	//			}
//	//			if(cross_flag == 1)		crossroad_judge();
//			break;
//				
//		case ISLAND_LPREENTER://1  // 左预入环状态
//				
//			//识别到即将入岛
//			//当距离达到设定值 进行入岛
//			L_enter_judge();
//			break;
//							
//		case ISLAND_RPREENTER://2  // 右预入环状态

//			//识别到即将入岛
//			//当距离达到设定值 进行入岛
////			renter_judge();								
//			break;
//					
//		case ISLAND_LENTER://1  // 入环判断状态

//			//识别到即将入岛
//			//当距离达到设定值 进行入岛
//			entered_judge();	
//			break;
//							
//		case ISLAND_RENTER://2  // 入环判断状态

//			//识别到即将入岛
//			//当距离达到设定值 进行入岛
//			entered_judge();										
//			break;
//							
//		case ISLAND_TURN_LEFT://3  // 左转入环执行状态

//			entered_judge();
//			break;

//		case ISLAND_TURN_RIGHT://4  // 右转入环执行状态

////			entered_judge();
//			break;	
//					
//        case ISLAND_IN:                   //     5  // 环岛内部循迹状态

////			send_end((float)STATE_IN_ISLAND);
//            // 检测环岛出口
//            pre_out_judge();
//        break;
//		
//        case ISLAND_EXIT:                 //     4  // 出环执行状态
//			//电感值再次回归正常
//            exit_judge();
//        break;
//					
////		case CROSSROAD:  // 十字交叉状态

////			crossroad_out();
////		break;
//					
//	}
//}

