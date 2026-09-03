// ============================================================
// 文件名: control.c
// 功能说明: 控制策略模块
// whole_test() 主状态机：读四路电感→出赛道保护→按状态设权重/PID/
// 基础速度→循迹偏差→方向PID→差速→元素进出判断。
// ============================================================

#include "bsp.h"
#include "app_config.h"
#include "control.h"
#include "motor.h"
#include "track_sensor.h"
#include "pid.h"
#include "element.h"
#include "roundabout.h"

extern uint8_t key_flag;        // 按键标志（config模块）
extern uint8_t Start_flag;      // 启动标志位
extern uint8_t Run_flag;        // 运行标志位

int16_t track_out = 0;          // 方向控制输出（由循迹PID计算）
uint8_t kernel_state = KERNEL_TRACKING;
uint8_t cask_flag = 0;

void whole_test(void)
{
    read_adc();     // 读取四路电感ADC值

    if( adc_filted[0] + adc_filted[1] + adc_filted[2] + adc_filted[3] > 300 )
    {
        Run_flag = 1;   // 标记已启动

        switch( kernel_state )
        {
            case KERNEL_TRACKING:

                weight_x   = 15;    //15
                weight_xx  = 20;    //20
                weight_y   = 22;    //28
                weight_abs = 10;    //5

                base_speed = 245;   //500--2.9
                KP_x  = 2;          //2
                K2P_x = 0.008;      //0.008
                KD_x  = 15;         //15
                K2D_x = 1;          //1

                track_error = get_track_error();
                track_out = PID_track();
                speed_control(track_out);

                crossroads_judge();
                L_reroundabout_judge();

            break;

            case KERNEL_REISLAND:

                weight_x   = 20;    //15
                weight_xx  = 10;    //20
                weight_y   = 1;     //28
                weight_abs = 10;    //5

                base_speed = 180;   //500--2.9
                KP_x  = 2;          //2
                K2P_x = 0.008;      //0.008
                KD_x  = 15;         //15
                K2D_x = 1;          //1

                track_error = get_track_error();
                track_out = PID_track();

                speed_control(track_out);

                R_roundabout_judge();
                reroundabout_out_judge();

            break;

            case KERNEL_CROSSROADS:

                weight_x   = 15;    //15
                weight_xx  = 20;    //20
                weight_y   = 2;     //28
                weight_abs = 10;    //5

                base_speed = 240;   //500--2.9
                KP_x  = 2;          //2
                K2P_x = 0.008;      //0.008
                KD_x  = 15;         //15
                K2D_x = 1;          //1

                track_error = get_track_error();
                track_out = PID_track();

                if( track_out >  10 )   track_out =  10;
                if( track_out < -10 )   track_out = -10;

                speed_control(track_out);

                crossroads_out_judge();

            break;

            case KERNEL_ISLAND_L:

                weight_x   = 15;    //15
                weight_xx  = 20;    //20
                weight_y   = 22;    //28
                weight_abs = 10;    //5

                KP_a = 2;
                KD_a = 1.2;
                KG_a = 0;

                sign_round = -1;
                roundabout();

            break;

            case KERNEL_ISLAND_R:

                weight_x   = 15;    //15
                weight_xx  = 20;    //20
                weight_y   = 22;    //28
                weight_abs = 10;    //5

                base_speed = 150;

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

                weight_x   = 15;    //15
                weight_xx  = 20;    //20
                weight_y   = 2;     //28
                weight_abs = 5;     //5

                base_speed = 210;   //500--2.9
                KP_x  = 2;          //2
                K2P_x = 0.008;      //0.008
                KD_x  = 15;         //15
                K2D_x = 1;          //1

                track_error = get_track_error();
                track_out = PID_track();
                speed_control(track_out);
                cask_out_judge();

            break;
        }
    }
    else    // 电感值过低，认为出赛道或停止线
    {
        key_flag = 0;
        Run_flag = 0;   // 清除启动标志
        Start_flag = 0;
    }
}
