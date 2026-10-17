#ifndef _MENU_H_
#define _MENU_H_

// 按键值枚举
#define OK      1       // 确定键
#define UP      2       // 上键
#define DOWN    3       // 下键
#define BACK    4       // 返回键
#define LEFT    5       // 左键（参数减）
#define RIGHT   6       // 右键（参数加）
#define RST     7       // 复位键（清屏刷新，保持当前页面）
#define ADJUST1 8       // 调参1
#define ADJUST2 9       // 调参2
#define FOUR    10      // 预留

// 按键ADC通道定义（使用ADC分压键盘）
#define KEY_CHANNEL1    (ADC_CH4_P14)   // 第一路按键ADC（方向键+确定）
#define KEY_CHANNEL2    (ADC_CH3_P13)   // 第二路按键ADC（复位+返回）

// 页面ID定义（提高可读性，避免魔法数字）
#define PAGE_HOME           0   // 主页
#define PAGE_ADC_ERR        21  // ADC实时数据显示
#define PAGE_SPD_DIS        22  // 速度和距离
#define PAGE_GYRO           23  // 陀螺仪
#define PAGE_ADJUST1        11   // 调参页1，慢速
#define PAGE_ADJUST2        12   // 调参页2，快速
#define PAGE_EXTRA          1   //用于录视频

extern uint8 page;       // 当前页面编号
extern uint8 arrow;      // 当前光标位置（从1开始计数）
extern uint16 key_adc1;			// 按键ADC通道1采样值
extern uint16 key_adc2;				// 按键ADC通道2采样值 



// 函数声明
uint8 key_scan(void);                               // ADC按键扫描
void key_init(void);
void menu(void);                                    // 主菜单显示与处理

#endif
