// ============================================================
// 文件名: menu.c
// 功能说明: IPS114显示屏菜单与按键处理模块
// 实现ADC分压按键扫描、页面导航、参数调参及屏幕刷新。
// ============================================================
//
// ==================== 操作方法 ====================
//
// 【按键定义】（两路ADC分压键盘）
//  UP/DOWN    : 上下移动光标
//  OK         : 主页上进入子页面（ADC_ERR / SPD_DIS / GYRO）
//  BACK       : 从子页面返回主页
//  LEFT/RIGHT : 在调参页面增减参数（步进±0.001或±100）
//  RST        : 清屏刷新，保持当前页面和光标
//  ADJUST1    : 一键进入调参页1（慢速模式）
//  ADJUST2    : 一键进入调参页2（快速模式）
//
// 【页面结构】
//  主页(0) -> ADC_ERR(21)  显示四路电感ADC值
//  主页(0) -> SPD_DIS(22)  显示左右轮速度及行驶距离
//  主页(0) -> GYRO(23)     显示三轴陀螺仪角速度
//  ADJUST1(11) / ADJUST2(12) : 调参页，可调整KP_x/K2P_x/KD_x/base_speed/fan_duty
//
// 【调参页面操作】
//  UP/DOWN 切换光标项（共5项）
//  LEFT/RIGHT 微调当前项参数
//  切换页面时自动将参数写入EEPROM保存
// ============================================================

#include "config.h"


// ==================== 菜单状态变量 ====================
uint8 page = PAGE_HOME  ;            // 当前页面编号
uint8 arrow = 1;            // 当前光标位置（从1开始计数）
static uint8 mode = 1;  	// 菜单模式（1=浏览模式，2=调参模式）
uint16 key_adc1 = 0;		// 按键ADC通道1采样值
uint16 key_adc2 = 0;		// 按键ADC通道2采样值

// 显示状态（用于减少不必要的刷新与闪烁）
static uint16 last_displayed_page = 0xFFFF;
static uint8  last_displayed_arrow = 0xFF;

static uint16 last_key_adc1 = 0xFFFF;
static uint16 last_key_adc2 = 0xFFFF;

static LowPassFilter filt_key1;
static LowPassFilter filt_key2;


// ==================== 静态函数声明 ====================
static void   menu_draw_content(void);
static void   menu_draw_cursor(void);

// ==================== 按键扫描 ====================

// ADC按键扫描函数
// 通过两路ADC通道读取按键分压值，判断当前按下的按键
// 返回值: UP/DOWN/OK/LEFT/RIGHT/RST/BACK 或 0（无按键）
uint8 key_scan(void)
{
    uint8 key_status = 0;
    
    // 采集两路按键ADC，各进行3次均值滤波
    key_adc2 = adc_mean_filter_convert(KEY_CHANNEL1, 3);
    key_adc1 = adc_mean_filter_convert(KEY_CHANNEL2, 3);

    if(key_adc1 / 100>3&&key_adc1 / 100<9)
    {
		key_status = UP; // 约542
	}
	else if(key_adc1 / 100>9&&key_adc1 / 100<15)
	{
		key_status = DOWN; // 约1262  
	}
	else if(key_adc1 / 100>15&&key_adc1 / 100<20)
	{
		key_status = OK; // 约1840
	}
	else if(key_adc1 / 100>20&&key_adc1 / 100<25)
	{ 
		key_status = LEFT;  // 约2467
	}
	else if(key_adc1 / 100>25&&key_adc1 / 100<31)
	{ 
		key_status = RIGHT; // 约3082
	}


    if(key_adc2 / 100>3 && key_adc2 / 100 <9)
	{
      key_status = RST;  // 约542
	}	
	else if(key_adc2 / 100>9 && key_adc2 / 100<15)
	{
         key_status = ADJUST1;  // 约1262  一键加载平稳模式
	}
	else if(key_adc2 / 100>15 && key_adc2 / 100<20)
	{
        key_status = ADJUST2;  // 约1840  一键加载快速模式
	}
	else if(key_adc2 / 100>20 && key_adc2 / 100<25)
	{
        key_status = FOUR;   // 约2467  
	}
	else if(key_adc2 / 100>25 && key_adc2 / 100<31)
	{
        key_status = BACK;  // 约3082
	}

	 key_adc1 = (uint16)lowpass_update(&filt_key1, (float)key_adc1);
     key_adc2 = (uint16)lowpass_update(&filt_key2, (float)key_adc2);
    return key_status;
}

// 函数名: key_init
// 功能: 初始化按键ADC和启动按键GPIO
// 说明: 配置两路按键分压ADC通道和IO_P22启动按键输入。
void key_init(void)
{
	adc_init(KEY_CHANNEL1, ADC_12BIT);		// 初始化按键ADC通道1
	adc_init(KEY_CHANNEL2, ADC_12BIT);		// 初始化按键ADC通道2
	gpio_init(IO_P23, GPIO, 1, GPIO_NO_PULL);	// 初始化启动按键GPIO（P23），无上拉输入
	lowpass_init(&filt_key1, 0.7f);
    lowpass_init(&filt_key2, 0.7f);
}


// ==================== 按键动作处理（普通浏览模式） ====================

// 普通浏览模式按键处理
// 根据当前页面和按键执行页面跳转或状态切换
void key_action(uint8 key)
{
    switch(key)
    {
    case UP:
        if(page == PAGE_HOME)
            arrow = (arrow == 1) ? 4 : arrow - 1;
        else if(page == PAGE_ADC_ERR || page == PAGE_SPD_DIS|| page == PAGE_GYRO )
            arrow = (arrow == 1) ? 4 : arrow - 1;
        else if(page == PAGE_ADJUST1||page == PAGE_ADJUST2)
            arrow = (arrow == 1) ? 5 : arrow - 1;
        else
            arrow = (arrow == 1) ? 4 : arrow - 1;
        break;
    case DOWN:
        if(page == PAGE_HOME)
            arrow = (arrow == 4) ? 1 : arrow + 1;
        else if(page == PAGE_ADC_ERR || page == PAGE_SPD_DIS|| page == PAGE_GYRO)
            arrow = (arrow == 4) ? 1 : arrow + 1;
        else if(page == PAGE_ADJUST1||page == PAGE_ADJUST2)
            arrow = (arrow == 5) ? 1 : arrow + 1;
        else
            arrow = (arrow == 4) ? 1 : arrow + 1;
        break;

    case OK:
		if(page == PAGE_HOME)
			{
                page =  20+arrow;
				arrow = 1; 
				break;
			}
			
			

    case BACK:
         if(page == PAGE_ADC_ERR || page == PAGE_SPD_DIS|| page == PAGE_GYRO||page==PAGE_ADJUST1||page==PAGE_ADJUST2)
			{
                page = PAGE_HOME;
				arrow = 1; 
				break;
			}
			
			if( page==PAGE_ADJUST1)
			{
				write_speed_low();
				page = PAGE_HOME;
				arrow = 1; 
				mode=1;
				break;
			}
			if( page==PAGE_ADJUST2)
			{
				write_speed_high();
                page = PAGE_HOME;
				arrow = 1; 
				mode=1;
				break;
			}
			
	 case LEFT:
		 if((page==PAGE_ADJUST1||page==PAGE_ADJUST2)&&mode==2)
		 {
			 if(arrow==1)	KP_x-=0.001;
			 if(arrow==2)	K2P_x-=0.001;
			 if(arrow==3)	KD_x-=0.001;
			 if(arrow==4)	base_speed-=100;
			 if(arrow==5)	fan_duty-=100;
		 }
          break;

	 case RIGHT:
		 if((page==PAGE_ADJUST1||page==PAGE_ADJUST2)&&mode==2)
		 {
			 if(arrow==1)	KP_x+=0.001;
			 if(arrow==2)	K2P_x+=0.001;
			 if(arrow==3)	KD_x+=0.001;
			 if(arrow==4)	base_speed+=100;
			 if(arrow==5)	fan_duty+=100;
		 }
          
             break;
	case ADJUST1:
		{
			page =PAGE_ADJUST1;
			read_speed_low();
			mode=2;
			break;
		}

	case ADJUST2:
		{
			page =PAGE_ADJUST2;	
			read_speed_high();
			mode=2;
			break;
		}
	
}}

// ==================== 显示绘制函数 ====================

// 绘制当前页面的静态/动态内容
static void menu_draw_content(void)
{
    switch(page)
    {
//	case PAGE_EXTRA:
//		ips114_show_string(2, 0,  "adc1 ");    ips114_show_int16(100, 0,  adc_filted[0]);
//        ips114_show_string(2, 16, "adc1 ");    ips114_show_int16(100, 16, adc_filted[1]);
//	    ips114_show_string(2, 32, "adc3 ");    ips114_show_int16(100, 32, adc_filted[2]);
//        ips114_show_string(2, 48, "adc4 ");    ips114_show_int16(100, 48, adc_filted[3]);
//	    ips114_show_string(2, 64, "diff ");    ips114_show_int16(100, 64, diff_g);
//	    ips114_show_string(2, 80, "W_X ");    ips114_show_int16(100, 80, weight_x);
//		ips114_show_string(2, 96, "W_y ");    ips114_show_int16(100, 96, weight_y);	
//	    ips114_show_string(2, 112, "gyro_x");   ips114_show_float(100, 112, gyro_x,2,2);
//       // ips114_show_string(2, 80, "gyro_y");   ips114_show_float(100, 128, gyro_y,2,2);
//        ips114_show_string(2, 128, "gyro_z");   ips114_show_float(100, 128, gyro_z,2,2);
//		
		
    case PAGE_HOME:
        ips114_show_string(2, 16 * 0, "ADC_ERR");
        ips114_show_string(2, 16 * 1, "SPD_DIS");
        ips114_show_string(2, 16 * 2, "GYRO");
        break;

    case PAGE_ADC_ERR :
		ips114_show_string(2, 0,  "adc1 ");    ips114_show_int16(100, 0,  adc_filted[0]);
        ips114_show_string(2, 16, "adc1 ");    ips114_show_int16(100, 16, adc_filted[1]);
	    ips114_show_string(2, 32, "adc3 ");    ips114_show_int16(100, 32, adc_filted[2]);
        ips114_show_string(2, 48, "adc4 ");    ips114_show_int16(100, 48, adc_filted[3]);
		ips114_show_string(2, 64, "error ");   ips114_show_int16(100, 64, track_error);
		ips114_show_string(2, 80, "S_X ");    ips114_show_int16(100, 80, symmetry_x);
		ips114_show_string(2, 96, "S_Y ");   ips114_show_int16(100, 96, symmetry_y);
// 	    ips114_show_string(2, 32, "adc3 ");    ips114_show_int16(100, 32, key_adc1);
//        ips114_show_string(2, 48, "adc4 ");    ips114_show_int16(100, 48, key_adc2);
         break;

    case PAGE_SPD_DIS:
        ips114_show_string(2, 0,  "speed_L");  ips114_show_int16(100, 0,  real_speed_L);
        ips114_show_string(2, 16, "speed_R");  ips114_show_int16(100, 16, real_speed_R);
        ips114_show_string(2, 32, "distance"); ips114_show_int32(100, 32, Distance, 8);
        break;

    case PAGE_GYRO:
        ips114_show_string(2, 0,  "gyro_x");   ips114_show_float(100, 0,  gyro_x,2,2);
        ips114_show_string(2, 16, "gyro_y");   ips114_show_float(100, 16, gyro_y,2,2);
        ips114_show_string(2, 32, "gyro_z");   ips114_show_float(100, 32, gyro_z,2,2);
	    break;
	case PAGE_ADJUST1:
        ips114_show_string(2, 0,  "KP_x");         ips114_show_float(100, 0,  KP_x,2,3);
        ips114_show_string(2, 16, "K2P_x");        ips114_show_float(100, 16, K2P_x,2,3);
	    ips114_show_string(2, 32, "KD_x");         ips114_show_float(100, 32, KD_x,2,3);
	    ips114_show_string(2, 48, "base_speed");   ips114_show_int16(100, 48, base_speed);
	    ips114_show_string(2, 64, "fan_duty");     ips114_show_int16(100, 64, fan_duty);
		write_speed_low();
	    break;
	
	 case PAGE_ADJUST2:
        ips114_show_string(2, 0,  "KP_x");         ips114_show_float(100, 0,  KP_x,2,3);
        ips114_show_string(2, 16, "K2P_x");        ips114_show_float(100, 16, K2P_x,2,3);
	    ips114_show_string(2, 32, "KD_x");         ips114_show_float(100, 32, KD_x,2,3);
	    ips114_show_string(2, 48, "base_speed");   ips114_show_int16(100, 48, base_speed);
	    ips114_show_string(2, 64, "fan_duty");     ips114_show_int16(100, 64, fan_duty);
	
	    break; 
    }
}

// 绘制光标指示器
static void menu_draw_cursor(void)
{
    // 浏览模式：仅在光标位置变化时更新，减少闪烁
    if(arrow != last_displayed_arrow || page != last_displayed_page)
    {
        if(last_displayed_arrow > 0 && last_displayed_arrow <= 6)
        {
            ips114_show_string(220, 16 * (last_displayed_arrow - 1), "  ");
        }
        ips114_show_string(220, 16 * (arrow - 1), "<<");
        last_displayed_arrow = arrow;
    }
}

// ==================== 主菜单函数 ====================

// 主菜单显示与处理函数
// 应在主循环中周期性调用，完成按键响应和屏幕刷新
void menu(void)
{
	static uint8 last_key = 0;
	
    static uint8 last_written_page = 0xFF;   // 新增
    static uint8 last_written_arrow = 0xFF;  // 新增
   
    uint8 key = key_scan();//读取

    // 无键清零
    if(key == 0)
    {
        last_key = 0;
    }

    // RST键处理：清屏重绘
    if(key == RST && key != last_key)
    {
        ips114_clear(RGB565_WHITE);
        last_displayed_page = 0xFFFF;   // 强制下次重绘
        last_displayed_arrow = 0xFF;
        last_key = key;
        return;
    }

    // 按键按下沿检测：有按键且与上次不同
    if(key != last_key && key != 0)
    {
		key_action(key);//读取
        last_key = key;
    }

    // 页面切换检测：仅在切换时执行清屏，避免每帧闪烁
    if(page != last_displayed_page)
    {
        ips114_clear(RGB565_WHITE);
        last_displayed_page = page;
        last_displayed_arrow = 0xFF;    // 强制重绘光标
    }

	if(key_adc1 != last_key_adc1 || key_adc2 != last_key_adc2)
    {
     
    ips114_show_int16(180, 96,  key_adc1);
    ips114_show_int16(180, 112, key_adc2);
     

    last_key_adc1 = key_adc1;
    last_key_adc2 = key_adc2;
   }
	
	
    // 绘制页面内容
    menu_draw_content();

    // 绘制光标指示器
    menu_draw_cursor();
}
