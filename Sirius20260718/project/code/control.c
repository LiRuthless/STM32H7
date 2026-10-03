// ============================================================
// 文件名: control.c
// 功能说明: 控制策略与测试函数模块
// 包含速度环测试、循迹测试、ADC测试等控制策略实现。
// ============================================================

#include "config.h"

int16 track_out = 0;     // 方向控制输出（由循迹PID计算）
uint8 kernel_state = KERNEL_TRACKING;
uint8 cask_flag = 0;

void whole_test(void)
{   
	read_adc();     // 读取四路电感ADC值 

//	fan_duty = 6000;       // 初始化负压电机占空比为60%占空比
	
	if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 300 )
    {
        Run_flag = 1;      // 标记已启动
		
//		sprintf(uart,"%d,%d\n",kernel_state ,roundabout_state);
//		uart_write_buffer(UART_1,uart,strlen(uart));
		
		switch( kernel_state )
		{
			case KERNEL_TRACKING:
				
				weight_x   = 15;	//15
				weight_xx  = 20;	//20
				weight_y   = 22;	//28
				weight_abs = 10;	//5
				
				base_speed = 245; 	//500--2.9
				KP_x  = 2;   		//2
				K2P_x = 0.008; 		//0.008
				KD_x  = 15;	  		//15	
				K2D_x = 1;	  		//1
			
				track_error = get_track_error();
				track_out = PID_track(); 
				speed_control(track_out);
			
				crossroads_judge();
//				teeterboard_judge();
 				L_reroundabout_judge();
				//R_reroundabout_judge();
//				L_roundabout_judge();
//				R_roundabout_judge();
			//	cask_judge();

			
			break;
			
			case KERNEL_REISLAND:

//				KP_a = 2;
//				KD_a = 1.2;
//				KG_a = 0;
			
//				target_speed_L = base_speed;
//				target_speed_R = base_speed;
				
				weight_x   = 20;	//15
				weight_xx  = 10;	//20
				weight_y   = 1;	//28
				weight_abs = 10;	//5
				
				base_speed = 180; 	//500--2.9
				KP_x  = 2;   		//2
				K2P_x = 0.008;		//0.008
				KD_x  = 15;			//15	
				K2D_x = 1;	  		//1
			
				track_error = get_track_error();
				track_out = PID_track();
				
//				if( track_out >  10)	track_out =  10;
//				if( track_out < -10 )	track_out = -10;
			
				speed_control(track_out);
//				speed_control( 0 );
			
//				L_roundabout_judge();
				R_roundabout_judge();
//				entered_judge();
				reroundabout_out_judge();
				
			
			break;
				
			case KERNEL_CROSSROADS:
				
				weight_x   = 15;	//15
				weight_xx  = 20;	//20
				weight_y   = 2;		//28
				weight_abs = 10;	//5
				
				base_speed = 240; 	//500--2.9
				KP_x  = 2;   		//2
				K2P_x = 0.008;		//0.008
				KD_x  = 15;			//15	
				K2D_x = 1;	  		//1
			
				track_error = get_track_error();
				track_out = PID_track();
				
				if( track_out >  10)	track_out =  10;
				if( track_out < -10 )	track_out = -10;
			
				speed_control(track_out);
				
				crossroads_out_judge();
			
			break;
			
			case KERNEL_ISLAND_L:
				
				weight_x   = 15;	//15
				weight_xx  = 20;	//20
				weight_y   = 22;	//28
				weight_abs = 10;		//5
			
				KP_a = 2;
				KD_a = 1.2;
				KG_a = 0;
			
				sign_round = -1;
				roundabout();
			
			break;
			
			case KERNEL_ISLAND_R:
				
				weight_x   = 15;	//15
				weight_xx  = 20;	//20
				weight_y   = 22;	//28
				weight_abs = 10;	//5
			
				base_speed= 150;
			
				KP_a = 2;
				KD_a = 1.2;
				KG_a = 0;
				
				sign_round = 1;
				roundabout();
			
			break;
			
			case KERNEL_TEETERBOARD:
				
				base_speed = 100; 
				KP_x  = 4.5;
				K2P_x = 0.0;
				KD_x  = 6.2;
				K2D_x = 0.9;
			
				track_error = get_track_error();
				track_out = PID_track();
				speed_control(track_out);
			
				teeterboard_out_judge();
			
			break;
			
			case KERNEL_CASK:
				weight_x   = 15;	//15
				weight_xx  = 20;	//20
				weight_y   = 2;	//28
				weight_abs = 5;		//5
			
				base_speed = 210; 	//500--2.9
				KP_x  = 2;   		//2
				K2P_x = 0.008; 		//0.008
				KD_x  = 15;	  		//15	
				K2D_x = 1;	  		//1
			
				track_error = get_track_error();
				track_out = PID_track();
				speed_control(track_out);
				cask_out_judge();
			break;
				
				
			
		}
    }
    else // 电感值过低，认为出赛道或停止线
    {    
		key_flag = 0;
        Run_flag = 0;      // 清除启动标志
		Start_flag = 0;
    }
}


void roundabout_test(void)
{   
	read_adc();     // 读取四路电感ADC值 
	
	KP_a = 0.5;
	KD_a = 0;
	KG_a = 0;
	
	KP_x = 1.45;
	KD_x = 7.45; 
	
	base_speed = 180; 
	fan_duty = 6000;      
	
	if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 300 )
    {
        Run_flag = 1;      // 标记已启动
		pwm_set_duty(MOTOR_PWM_M, fan_duty); // 设置负压电机占空比
	
		if( kernel_state == KERNEL_TRACKING )	// 当前处于正常循迹状态 
		{				
			track_error = get_track_error();
			track_out = PID_track();
			speed_control(track_out);		
			
			L_roundabout_judge();
		} 
		else
		{
			sign_round = -1;
			roundabout();
		}
		
    }
    else // 电感值过低，认为出赛道或停止线
    {    
        Run_flag = 0;      // 清除启动标志
		Start_flag = 0;
    }
}




void track_test(void)
{
	read_adc();     // 读取四路电感ADC值 
	
//	fan_duty = 5500;       // 初始化负压电机占空比为60%占空比
	
//	base_speed = 600; 
//	KP_x = 0.04;
//	KD_x = 0.15;
	
//	base_speed = 350; 
//	KP_x = 1.0;
//	KD_x = 16.65;
	
//	base_speed = 300; 
//	KP_x = 0.7;
//	KD_x = 9.65;

	base_speed = 100; 
	KP_x = 1.45;
	KD_x = 7.45;

	
    // 检测电感值总和是否大于阈值（判断是否在赛道上）
    if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 300 )
    {
        // 在赛道上，计算循迹偏差并差速控制
		track_error = get_track_error();
        track_out = PID_track();
		
		speed_control(track_out);	// 根据偏差方向分配左右轮速度
		
		pwm_set_duty(MOTOR_PWM_M, fan_duty); // 设置负压电机占空比
		
        Run_flag = 1;      // 标记已启动
    }
    else // 电感值过低，认为出赛道或停止线
    {    
        Run_flag = 0;      // 清除启动标志
		Start_flag = 0;
    }
	
//	sprintf(uart, "%d,%d,%d,%d,", adc_filted[0], adc_filted[1], adc_filted[2], adc_filted[3]);
// 	uart_write_buffer(UART_1,uart,strlen(uart));
//	
//	sprintf(uart,"%d,%d\n",track_error,track_out);
// 	uart_write_buffer(UART_1,uart,strlen(uart));
    
    // 未启动状态下停止电机
//    if( Run == 0 )
//    {
//		target_speed_L = 0;
//		target_speed_R = 0;
//		pwm_set_duty(MOTOR_PWM_M, 1000); // 负压电机回默认占空比
//    }

}



// 函数名: speed_test2
// 功能: 速度环调参测试（固定周期切换速度）
// 说明: 每1000ms在600和300之间切换基础速度（在5ms中断中调用，cnt每200次切换一次），
//       用于测试速度环响应。
void speed_test2(void)
{
static uint16 cnt = 0; 

    cnt++;

	if(cnt >= 200)               // 200 * 2ms = 400ms
    {
        cnt = 0;
		if(base_speed != 500){
            base_speed = 500;
        }
        else{
            base_speed = 200;
        }
    }
	target_speed_L = base_speed;
	target_speed_R = base_speed;
	
	sprintf(uart,"%d,%d,%f,",target_speed_L,real_speed_L,PID_outL);
	uart_write_buffer(UART_1,uart,strlen(uart));
		
	sprintf(uart,"%d,%d,%f\n",target_speed_R,real_speed_R,PID_outR);
 	uart_write_buffer(UART_1,uart,strlen(uart));
}


// 函数名: speed_test
// 功能: 速度环与循迹综合控制
// 说明: 每400ms切换基础速度（在5ms中断中调用，cnt每80次切换一次），
//       同时进行循迹检测与差速控制。
//       当电感值总和超过阈值时，认为在赛道上，启动循迹PID并差速；
//       否则停止电机。
void speed_test(void)
{
static uint16 cnt = 0; 
	
	read_adc();     // 读取四路电感ADC值 
	
    cnt++;
    if(cnt >= 100)               // 80 * 2ms = 400ms
    {
        cnt = 0;
        if(base_speed != 300)
        {
            base_speed =300;
//			KP_x = 1.6f;
//			KD_x = 0.6f;
        }
        else
        {
            base_speed = 180;
//			KP_x = 0.5f;
//			KD_x = 0.6f;
        }
    }
	
    // 检测电感值总和是否大于阈值（判断是否在赛道上）
    if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 300 )
    {
        // 在赛道上，计算循迹偏差并差速控制
		track_error = get_track_error();
        track_out = PID_track();
        
        // 根据偏差方向分配左右轮速度
        if( track_out >= 0 ){
            target_speed_L = base_speed - 3 * track_out;    // 左轮减速
            target_speed_R = base_speed + 1 * track_out;    // 右轮加速
        }
        else{
            target_speed_L = base_speed - 1 * track_out;    // 左轮加速
            target_speed_R = base_speed + 3 * track_out;    // 右轮减速
        }
		
		pwm_set_duty(MOTOR_PWM_M, fan_duty); // 设置负压电机占空比
		
        Run_flag = 1;      // 标记已启动
    }
    else // 电感值过低，认为出赛道或停止线
    {    
        Run_flag = 0;      // 清除启动标志
		Start_flag = 0;
    }
    
    // 未启动状态下停止电机
//    if( Run == 0 )
//    {
//		target_speed_L = 0;
//		target_speed_R = 0;
//		pwm_set_duty(MOTOR_PWM_M, 1000); // 负压电机回默认占空比
//    }

}

// 函数名: gyro_test
// 功能: 陀螺仪测试（预留）
// 说明: 读取IMU陀螺仪数据并通过串口输出，用于传感器调试。
void gyro_test(void)
{
//	imu660rb_get_gyro();
//	
//	gyro_x = imu660rb_gyro_transition(imu660rb_gyro_x);
//	gyro_y = imu660rb_gyro_transition(imu660rb_gyro_y);
//	gyro_z = imu660rb_gyro_transition(imu660rb_gyro_z);
	
	static int16 angle = 0;
	KP_a = 2;
	KD_a = 1.2;
	KG_a = 0;
	base_speed=0;
//	
//	if( angle_x <= 0)
//	{
		angle = 90;
//	}
//	if( angle_x >= 360)
//	{
//		angle = -20;	
//	}
//	 
	PID_angle( angle );
	
	sprintf(uart,"%f,%d,%f\n",angle_x,angle,angle_err);
	uart_write_buffer(UART_1,uart,strlen(uart));
	
	
//	sprintf(uart,"%f,%f,%f,",gyro_x,gyro_y,gyro_z);
//	uart_write_buffer(UART_1,uart,strlen(uart));
//		
//	sprintf(uart,"%d,%d,%d\n",imu660rb_gyro_x,imu660rb_gyro_y,imu660rb_gyro_z);
// 	uart_write_buffer(UART_1,uart,strlen(uart));
	
}

// 函数名: adc_test
// 功能: ADC采集与串口输出测试
// 说明: 读取四路ADC并计算循迹偏差，通过串口输出电感值和偏差数据，用于调参调试
void adc_test(void)
{
	read_adc();     // 读取四路电感ADC值 
	
	KP_x = 0.02;
	KD_x = 0.36;
	
	track_error = get_track_error();
    track_out = PID_track();
	
	sprintf(uart, "%d,%d,%d,%d,", adc_filted[0], adc_filted[1], adc_filted[2], adc_filted[3]);
 	uart_write_buffer(UART_1,uart,strlen(uart));
	
	sprintf(uart,"%d,%d\n",track_error,track_out);
 	uart_write_buffer(UART_1,uart,strlen(uart));
	
}

