// ============================================================
// 文件名: main.c
// 功能说明: 主程序入口
// 智能车电磁循迹系统主循环，包含系统初始化、PID参数设定、
// 定时中断配置以及主循环空转。
// ============================================================

#include "config.h"

// 函数名: main
// 功能: 主函数入口
void main()
{
    clock_init(SYSTEM_CLOCK_40M);       // 设置系统时钟为40MHz

    All_init();                         // 调用各模块统一初始化

    tim3_irq_handler = pit_track;       // TIM3中断 -> 循迹控制处理

	KP_v  = 20.0;       				// 速度环比例系数			//15.0  0.85
    KI_v  = 0.75;      					// 速度环积分系数			//3.46  0.39!!!!!!!
	
	//gyro_calibrate();

    // 主循环
	while(1)
	{	
		key_start();    


		if (key_flag && !Start_flag)         
		{
			Start_flag = 1;                        
			
			pit_ms_init(PIT_TR, 2);    			// 初始化5ms定时中断（循迹控制）
			WDT_CONTR = 0x42;	//0x21;                   // 使能看门狗，预分频4（约140ms溢出）
		}
		else if (Start_flag)
		{
			
			
////			sprintf(uart,"%d,%d,%d,%d,",adc_filted[0],adc_filted[1] ,adc_filted[2],adc_filted[3]);
////			uart_write_buffer(UART_1,uart,strlen(uart));
////	
//			sprintf(uart,"%d,%d,",adc_filted[0]+adc_filted[3], adc_filted[0]+adc_filted[1] +adc_filted[2]+adc_filted[3]);
//			uart_write_buffer(UART_1,uart,strlen(uart));
//			
////			sprintf(uart,"%d,%d,",track_error,track_out);
////			uart_write_buffer(UART_1,uart,strlen(uart));
////			
//////			sprintf(uart,"%f,%f,",angle_err ,angle_out);
//////			uart_write_buffer(UART_1,uart,strlen(uart));
////				sprintf(uart,"%d,%d,%f,",target_speed_L,real_speed_L,PID_outL);
////				uart_write_buffer(UART_1,uart,strlen(uart));
////		
////				sprintf(uart,"%d,%d,%f,",target_speed_R,real_speed_R,PID_outR);
////				uart_write_buffer(UART_1,uart,strlen(uart));
//			
//			sprintf(uart,"%d,",Distance);
//			uart_write_buffer(UART_1,uart,strlen(uart));
////			
////	
//			sprintf(uart,"%f,%f,%f,%f,",gyro_x, gyro_y, angle_x ,angle_y);
//			uart_write_buffer(UART_1,uart,strlen(uart));
//			sprintf(uart,"%d,%d\n",kernel_state ,roundabout_state);
//			uart_write_buffer(UART_1,uart,strlen(uart));
////			
			
		}
		else
		{
			read_adc();
			track_error = get_track_error();
			track_out = PID_track();
			menu();		
//			adc_menu();
			
			time = 0;
//			gpio_set_level(MOTOR_PWM_M, 0);
			pwm_set_duty(MOTOR_PWM_M, 1100);
		}
	}
	
}


// 函数名: pit_track
// 功能: TIM3中断服务函数（5ms周期）
// 说明: 执行循迹控制，检测赛道并计算差速目标速度。
//       未启动或出赛道时强制停车。
void pit_track (void)
{
	
	CLR_WDT = 1;        // 在2ms中断中喂狗，不受主循环menu()耗时影响
	
	time++;
	
	read_gyro_angle();
	dl1b_get_distance();
	
//	sprintf(uart,"%d\n",dl1b_distance_mm);
//	uart_write_buffer(UART_1,uart,strlen(uart));
	
//	sprintf(uart,"%f,%f,%f,%f\n",gyro_x, gyro_y, angle_x ,angle_y);
//	uart_write_buffer(UART_1,uart,strlen(uart));
	
	if( Start_flag )
	{
//		gpio_set_level(MOTOR_PWM_M, 1);
		fan_duty = 1600;
		pwm_set_duty(MOTOR_PWM_M, fan_duty); // 设置负压电机占空比
		
		if( time > 1000 )
		{
//			KP_a = 0.5;
//			KD_a = 0;
//			KG_a = 0;
//			base_speed = 320;
//			
//			PID_angle( 0 );
			
//			weight_x   = 17;	//15
//			weight_xx  = 18;	//20
//			weight_y   = 38;	//28
//			weight_abs = 3;		//5
//			
//			base_speed = 380; 	//500--2.9				
//			KP_x  = 2.5;   		//2
//			K2P_x = 0.008; 		//0.008
//			KD_x  = 18;	  		//15	
//			K2D_x = 0.8;	  		//1
			
			whole_test();
//			roundabout_test();
//			track_test();
//			speed_test2();
//			gyro_test();
		}

// 		speed_test2();
//		adc_test();
		
	}

    if( !Run_flag || !Start_flag )
    {
		target_speed_L = 0;
		target_speed_R = 0;
		if( !key_flag )
		{
			gpio_set_level(MOTOR_PWM_M, 0);
			pwm_set_duty(MOTOR_PWM_M, 1100); // 负压电机回默认占空比
		}
    }
	
//	if( Distance > 3200 )
//	{
//		target_speed_L = 0;
//		target_speed_R = 0;
//		gpio_set_level(MOTOR_PWM_M, 0);
//		
//		Start_flag = 0;
//		key_flag = 0;
//	}
	
    read_encoder();
    motor_control(); 
	
	

//	
//	sprintf(uart,"%f,%f\n",angle_err ,angle_out);
//	uart_write_buffer(UART_1,uart,strlen(uart));
	
//	sprintf(uart,"%d,%d,%d,%d,",adc_filted[0] ,adc_filted[1] ,adc_filted[2],adc_filted[3]);
// 	uart_write_buffer(UART_1,uart,strlen(uart));
//	
//	sprintf(uart,"%f,%f,%f,%f\n",gyro_x, gyro_y, angle_x ,angle_y);
//	uart_write_buffer(UART_1,uart,strlen(uart));
//	
//	sprintf(uart,"%d,%d\n",track_error,track_out);
//	uart_write_buffer(UART_1,uart,strlen(uart));
 
	
//	sprintf(uart,"%d,%d,%f,",target_speed_L,real_speed_L,PID_outL);
//	uart_write_buffer(UART_1,uart,strlen(uart));
//		
//	sprintf(uart,"%d,%d,%f\n",target_speed_R,real_speed_R,PID_outR);
// 	uart_write_buffer(UART_1,uart,strlen(uart));
}                                                       
