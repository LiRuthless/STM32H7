// ============================================================
// 文件名: eeprom.h
// 功能说明: EEPROM参数读写模块头文件
// 声明菜单状态、速度环PID、慢速/快速模式参数的Flash读写接口。
// ============================================================

#ifndef _EEPROM_H_
#define _EEPROM_H_

extern int16 air;      // 预留参数（用于扩展存储）

void write_menu(void);      // 写入菜单状态到EEPROM
void write_pid_v(void);     // 写入速度环PID参数到EEPROM
void write_speed_low(void); // 写入慢速模式参数到EEPROM
void write_speed_high(void);// 写入快速模式参数到EEPROM

void read_menu(void);       // 从EEPROM读取菜单状态
void read_pid_v(void);      // 从EEPROM读取速度环PID参数
void read_speed_low(void);  // 从EEPROM读取慢速模式参数
void read_speed_high(void); // 从EEPROM读取快速模式参数


#endif
