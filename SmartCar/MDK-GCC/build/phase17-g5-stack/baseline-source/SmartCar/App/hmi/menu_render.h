/**
 * @file menu_render.h
 * @brief 菜单绘制入口。
 * @details 绘制缓存位于menu_render.c，导航和编辑仍由menu.c持有。
 */
#ifndef SMARTCAR_APP_HMI_MENU_RENDER_H
#define SMARTCAR_APP_HMI_MENU_RENDER_H
#include <stdint.h>

/**
 * @brief 清屏并使下次页面绘制失效。
 * @note 完整契约见 menu_render.c::MenuRender_Reset。
 */
void MenuRender_Reset(void);
/**
 * @brief 页面改变时清屏并复位光标缓存。
 * @note 完整契约见 menu_render.c::MenuRender_PreparePage。
 */
void MenuRender_PreparePage(void);
/**
 * @brief 依次绘制页面内容与光标。
 * @note 完整契约见 menu_render.c::MenuRender_Draw。
 */
void MenuRender_Draw(void);

#endif /* SMARTCAR_APP_HMI_MENU_RENDER_H */
