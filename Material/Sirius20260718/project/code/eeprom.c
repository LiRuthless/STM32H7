// ============================================================
// 文件名: eeprom.c
// 功能说明: EEPROM参数读写模块
// 将PID参数和速度参数存储到片内Flash（IAP）中，
// 实现断电保存与上电加载功能。
// ============================================================

#include "config.h"

int16 air = 0;      // 预留参数（用于扩展存储）


// 函数名: write_menu
// 功能: 将菜单状态写入EEPROM
// 说明: 保存当前页面和光标位置到Flash地址0x0110起。
void write_menu(void)
{  
    extern_iap_write_buff(0x0110, (uint8 *)&page, 2);
    extern_iap_write_buff(0x0112, (uint8 *)&arrow, 2);
    extern_iap_write_buff(0x0114, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0116, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0118, (uint8 *)&air, 2);
	
	extern_iap_write_buff(0x0120, (uint8 *)&air, 2);
    extern_iap_write_buff(0x0122, (uint8 *)&air, 2);
    extern_iap_write_buff(0x0124, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0126, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0128, (uint8 *)&air, 2);
}


// 函数名: write_pid_v
// 功能: 将当前速度环PID参数写入EEPROM
// 说明: 存储方向环和速度环的KP、KI、KD参数到Flash地址0x0200起
void write_pid_v(void)
{
    extern_iap_write_buff(0x0200, (uint8 *)&KP_v, 2);
    extern_iap_write_buff(0x0202, (uint8 *)&KI_v, 2);
}


// 函数名: write_speed_low
// 功能: 将慢速模式参数写入EEPROM
// 说明: 保存方向环PID及基础速度到Flash地址0x0210起。
void write_speed_low(void)
{  
    extern_iap_write_buff(0x0210, (uint8 *)&KP_x, 2);
    extern_iap_write_buff(0x0212, (uint8 *)&K2P_x, 2);
    extern_iap_write_buff(0x0214, (uint8 *)&KD_x, 2);
	extern_iap_write_buff(0x0216, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0218, (uint8 *)&base_speed, 2);
	
	extern_iap_write_buff(0x0220, (uint8 *)&fan_duty, 2);
    extern_iap_write_buff(0x0222, (uint8 *)&air, 2);
    extern_iap_write_buff(0x0224, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0226, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0228, (uint8 *)&air, 2);
}

// 函数名: write_speed_high
// 功能: 将当前快速模式参数写入EEPROM
// 说明: 存储基础速度和负压占空比到Flash地址0x0220起
void write_speed_high(void)
{
    extern_iap_write_buff(0x0230, (uint8 *)&KP_x, 2);
    extern_iap_write_buff(0x0232, (uint8 *)&K2P_x, 2);
    extern_iap_write_buff(0x0234, (uint8 *)&KD_x, 2);
	extern_iap_write_buff(0x0236, (uint8 *)&air, 2);
	extern_iap_write_buff(0x0238, (uint8 *)&base_speed, 2);
}

// 函数名: read_all
// 功能: 从EEPROM读取所有参数（预留统一接口）
void read_all(void)
{
	read_menu();
}




// 函数名: read_menu
// 功能: 从EEPROM读取菜单状态
// 说明: 从Flash地址0x0110起加载页面和光标位置。
void read_menu(void)
{  
    iap_read_buff(0x0110, (uint8 *)&page, 2);
    iap_read_buff(0x0112, (uint8 *)&arrow, 2);
    iap_read_buff(0x0114, (uint8 *)&air, 2);
	iap_read_buff(0x0116, (uint8 *)&air, 2);
	iap_read_buff(0x0118, (uint8 *)&air, 2);
	
	iap_read_buff(0x0120, (uint8 *)&air, 2);
    iap_read_buff(0x0122, (uint8 *)&air, 2);
    iap_read_buff(0x0124, (uint8 *)&air, 2);
	iap_read_buff(0x0126, (uint8 *)&air, 2);
	iap_read_buff(0x0128, (uint8 *)&air, 2);
}



// 函数名: read_pid_v
// 功能: 从EEPROM读取速度环PID参数
// 说明: 从Flash地址0x0200起加载方向环和速度环PID参数
void read_pid_v(void)
{
    iap_read_buff(0x0200, (uint8 *)&KP_v, 2);
    iap_read_buff(0x0202, (uint8 *)&KI_v, 2);
}

// 函数名: read_speed_low
// 功能: 从EEPROM读取慢速模式参数
// 说明: 从Flash地址0x0210起加载方向环PID及基础速度。
void read_speed_low(void)
{
    iap_read_buff(0x0210, (uint8 *)&KP_x, 2);
    iap_read_buff(0x0212, (uint8 *)&K2P_x, 2);
    iap_read_buff(0x0214, (uint8 *)&KD_x, 2);
	iap_read_buff(0x0216, (uint8 *)&air, 2);
	iap_read_buff(0x0218, (uint8 *)&base_speed, 2);
}

// 函数名: read_speed_high
// 功能: 从EEPROM读取快速模式参数
// 说明: 从Flash地址0x0230起加载方向环PID及基础速度。
void read_speed_high(void)
{
    iap_read_buff(0x0230, (uint8 *)&KP_x, 2);
    iap_read_buff(0x0232, (uint8 *)&K2P_x, 2);
    iap_read_buff(0x0234, (uint8 *)&KD_x, 2);
	iap_read_buff(0x0236, (uint8 *)&air, 2);
	iap_read_buff(0x0238, (uint8 *)&base_speed, 2);
}