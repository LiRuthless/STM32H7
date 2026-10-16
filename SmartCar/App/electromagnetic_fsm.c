/* 仅移入原 whole_test 的活动十字/预环岛/右环岛路径。
 * 本上下文独占所有判断历史；无 Motor/PID_Set/IMU_Zero 或硬件写入。 */
#include "electromagnetic_fsm.h"
#include "roundabout.h"
#include <stdlib.h>
#include <string.h>
static electromagnetic_state_t s_em;
static int32_t s_history;
const electromagnetic_state_t *ElectromagneticFSM_GetState(void) { return &s_em; }
void ElectromagneticFSM_Reset(void)
{
    memset(&s_em,0,sizeof(s_em)); s_history=0;
    s_em.configuration.weights=(track_weights_t){15,20,22,10};
}
void ElectromagneticFSM_Accumulate(int16_t left, int16_t right)
{ s_em.left_distance+=left; s_em.right_distance+=right; }
static int32_t distance(void) { return (s_em.left_distance+s_em.right_distance)/2; }
static void clear_distance(void)
{ s_em.left_distance=s_em.right_distance=0; s_em.reset_distance=1; }
static void clear_angle(void) { s_em.angle=0; s_em.zero_angle=1; }
static void direction(const electromagnetic_inputs_t *in)
{
    int32_t sx=in->adc[0]+in->adc[3], sy=in->adc[1]+in->adc[2];
    s_em.symmetry_x=sx ? abs(in->adc[0]-in->adc[3])*100/sx : 0;
    s_em.symmetry_y=sy ? abs(in->adc[1]-in->adc[2])*100/sy : 0;
    s_em.error=Track_CalculateError(in->adc,&s_em.configuration.weights);
    s_em.output=PID_DirectionStep(s_em.error,in->accel,&s_em.configuration.track,&s_history);
    s_em.steering=s_em.output;
}
void ElectromagneticFSM_Update(const electromagnetic_inputs_t *in)
{
    s_em.reset_distance=s_em.zero_angle=0;
    s_em.angle+=in->angle_delta;
    if(!in->enabled) return;
    Control_MergeProfile(&s_em.configuration,Control_GetKernelProfile(s_em.kernel_state));
    switch(s_em.kernel_state)
    {
    case KERNEL_TRACKING:
        direction(in);
        if(in->adc[1]+in->adc[2]>2800 && s_em.symmetry_y<25) s_em.kernel_state=KERNEL_CROSSROADS;
        /* 保留同一旧分支中预环岛覆盖十字的调用顺序。 */
        if(in->adc[0]+in->adc[3]>2800)
        {
            if(in->adc[0]>in->adc[3]) s_em.left_flag=1; else s_em.right_flag=1;
            s_em.cask_flag=0; clear_angle(); clear_distance(); s_em.kernel_state=KERNEL_REISLAND;
        }
        if(in->dl1b_mm<100) s_em.cask_flag=1;
        break;
    case KERNEL_CROSSROADS:
        direction(in);
        if(s_em.output>10) s_em.output=10;
        if(s_em.output< -10) s_em.output= -10;
        s_em.steering=s_em.output;
        if(in->adc[1]+in->adc[2]<2800) s_em.kernel_state=KERNEL_TRACKING;
        break;
    case KERNEL_REISLAND:
        direction(in);
        if(in->adc[1]+in->adc[2]<300 && s_em.right_flag && !s_em.cask_flag)
        {
            s_em.roundabout_state=ISLAND_LPREENTER; s_em.kernel_state=KERNEL_ISLAND_R;
            s_em.output=0; clear_angle(); clear_distance();
        }
        /* 入环清零后再判断超时，不能提前缓存 distance。 */
        if(distance()>5000)
        {
            s_em.kernel_state=KERNEL_TRACKING; s_em.left_flag=s_em.right_flag=0;
            s_em.output=0; clear_angle();
        }
        break;
    case KERNEL_ISLAND_R:
        Control_MergeProfile(&s_em.configuration,Roundabout_GetProfile(s_em.roundabout_state));
        switch(s_em.roundabout_state)
        {
        case ISLAND_LPREENTER:
            direction(in);
            if(distance()>=enter_distance1) { s_em.roundabout_state=ISLAND_TURN_LEFT; clear_distance(); }
            break;
        case ISLAND_TURN_LEFT:
            s_em.steering= -30;
            if(distance()>13000) { s_em.roundabout_state=ISLAND_IN; clear_distance(); }
            break;
        case ISLAND_IN:
            direction(in);
            if(distance()>32000) { s_em.roundabout_state=ISLAND_OUT; clear_distance(); }
            break;
        case ISLAND_OUT:
            direction(in);
            if(distance()>out_distance1)
            {
                s_em.left_flag=s_em.right_flag=0; s_em.roundabout_state=STATE_NORMAL;
                s_em.kernel_state=KERNEL_TRACKING;
            }
            break;
        default: break;
        }
        break;
    default: break; /* 无休眠元素入口 */
    }
}
