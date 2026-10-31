/**
 * @file menu_render.h
 * @brief 菜单绘制入口。
 * @details 绘制缓存位于menu_render.c，导航和编辑仍由menu.c持有。
 */
#ifndef SMARTCAR_APP_HMI_MENU_RENDER_H
#define SMARTCAR_APP_HMI_MENU_RENDER_H
#include <stdint.h>

/**
 * @brief 清屏并强制下轮重画
 * @note 完整契约见 menu_render.c::MenuRender_Reset。
 */
void MenuRender_Reset(void);
/**
 * @brief 按页面变化准备显示
 * @note 完整契约见 menu_render.c::MenuRender_PreparePage。
 */
void MenuRender_PreparePage(void);
/**
 * @brief 按内容和光标顺序绘制
 * @note 完整契约见 menu_render.c::MenuRender_Draw。
 */
void MenuRender_Draw(void);

#endif /* SMARTCAR_APP_HMI_MENU_RENDER_H */
