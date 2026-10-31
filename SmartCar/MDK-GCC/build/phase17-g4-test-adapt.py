from pathlib import Path
import re
R=Path(__file__).resolve().parents[2];T=R/'tests';A=R/'App'
def wr(p,s):p.write_bytes((s.rstrip()+'\n').replace('\n','\r\n').encode('utf-8'))
p=A/'services/command_dispatch.c';s=p.read_text(encoding='utf-8').replace('status = s_tuning;','Tuning_GetStatus(&status);').replace('tune = s_tuning;','Tuning_GetStatus(&tune);');wr(p,s)
p=T/'control_equivalence.py';s=p.read_text(encoding='utf-8')
s=s.replace('    source = C_TOKENS.sub(replace, source)','    source = C_TOKENS.sub(replace, source)\n    source = source.replace(\'Tracking_TrySetMode(\', \'Lifecycle_TrySetMode(\')\n    source = source.replace(\'#include "param.h"\', \'#include "parameter_application.h"\\n#include "parameter_bank.h"\\n#include "parameter_store.h"\\n#include "lifecycle.h"\')\n    source = source.replace("g_menu_arrow=1;", "Menu_RestorePosition(Menu_GetPage(),1);")\n    source = source.replace("param_store_v1_t", "param_image_v1_t").replace("param_store_v2_t", "param_image_v2_t")')
s=s.replace('new_param = legacy_names((APP / "param.c").read_text(encoding="utf-8"))','new_param = legacy_names((APP / "parameter_image.h").read_text(encoding="utf-8").replace("param_image_v1_t", "param_store_v1_t").replace("param_image_v2_t", "param_store_v2_t").replace("param_image_v3_t", "param_store_t"))\n    new_store = (APP / "parameter_store.c").read_text(encoding="utf-8")')
s=s.replace("assert '0x1021u' in new_param and '0xFFFFu' in new_param","assert '0x1021u' in new_store and '0xFFFFu' in new_store")
wr(p,s)
p=T/'dual_mode_tracking.py';s=p.read_text(encoding='utf-8')
s=s.replace('#include "param.c"','#include "parameter_store.c"\n#include "parameter_application.h"\n#include "parameter_bank.h"\n#include "lifecycle.h"')
s=s.replace("shutil.copyfile(APP / 'param.c',work/'param.c')","shutil.copyfile(APP / 'parameter_store.c',work/'parameter_store.c')")
s=s.replace("files=['pid',", "files=['parameter_bank', 'parameter_application', 'lifecycle', 'menu_keys', 'menu_render', 'pid',")
s=s.replace("'optical_fsm', 'param', 'datalog'", "'optical_fsm', 'parameter_bank', 'parameter_store', 'parameter_application', 'menu_keys', 'menu_render', 'datalog'")
s=s.replace("if real_tuning: files += ('tuning',)","files += ('tuning', 'tuning_values', 'tuning_application', 'command_dispatch')")
s=s.replace("current_names(harness or APP_MAIN) + ('' if real_tuning else TUNING_DISABLED_STUB)","current_names(harness or APP_MAIN)")
# A pure parameter/FSM harness still uses disabled business methods, not the dispatch stub.
s=s.replace('const pid_track_gains_t*g,int16_t base)\n{(void)ordinary;(void)allowed;(void)g;(void)base;}','const pid_track_gains_t*g,int16_t base,const tuning_context_t*c)\n{(void)ordinary;(void)allowed;(void)g;(void)base;(void)c;}').replace('Tuning_GetControl(scb_values_t*v)','Tuning_GetControl(tuning_values_t*v)').replace('void Tuning_CompleteControl(void){}','void Tuning_CompleteControl(const tuning_context_t*c){(void)c;}')
# lifecycle run-state getters in parameter harness are now real; use its host-only seam.
for line in ['int32_t Lifecycle_GetControlTicks(void){return time;}','uint16_t Lifecycle_GetDistanceMm(void){return dl1b_distance_mm;}','uint8_t Lifecycle_IsStarted(void){return Start_flag;}','void Lifecycle_MarkRunning(void){Run_flag=1;}']:
    # only PARAM_MAIN definitions, do not affect other harness strings
    start=s.index('PARAM_MAIN =');end=s.index('def param_tests',start);sec=s[start:end].replace(line,'');s=s[:start]+sec+s[end:]
s=s.replace('Start_flag=1;assert(Tracking_TrySetMode(1)==TRACKING_BUSY);Start_flag=0;', 'Lifecycle_TestSetRunState(0,1,1,1);assert(Tracking_TrySetMode(1)==TRACKING_BUSY);Lifecycle_TestSetRunState(0,0,0,0);')
s=s.replace('uint8_t App_IsStopped(void){return !Start_flag;}','uint8_t App_IsStopped(void){return Lifecycle_IsStopped();}\nuint8_t Safety_IsStopPending(void){return 0;}')
s=s.replace("'-DREFACTORED','-ffunction-sections'","'-DREFACTORED','-DSMARTCAR_HOST_TEST','-ffunction-sections'")
# Link and request boundary substitutes retain all existing event assertions.
s=s.replace('void SCB_Service(void){++link_services;}', '''void SCB_Service(void){++link_services;}
static scb_command_handler_t registered_command;
static scb_session_handler_t registered_reset;
void SCB_SetHandlers(scb_command_handler_t c,scb_session_handler_t r){registered_command=c;registered_reset=r;assert(c&&r);}
uint8_t SCB_QueueResult(uint32_t i,uint8_t t,const uint8_t*p,uint16_t n,uint8_t f){(void)i;(void)t;(void)p;(void)n;(void)f;return 1;}
uint8_t SCB_GetRequestResult(uint32_t i,uint8_t*t,uint8_t*p,uint16_t*n){(void)i;(void)t;(void)p;(void)n;return 0;}
void Telemetry_SetRevision(uint32_t r){upload_status.revision=r;}''')
s=s.replace('#include "tuning.h"\n#include "optical_fsm.h"','#include "tuning.h"\n#include "command_dispatch.h"\n#include "optical_fsm.h"')
wr(p,s)
p=T/'runtime_tuning.py';s=p.read_text(encoding='utf-8')
s=s.replace('void SCB_SetHandlers(scb_command_handler_t c,scb_session_handler_t s){assert(c&&s);}', 'static scb_command_handler_t registered_command;\nstatic scb_session_handler_t registered_reset;\nvoid SCB_SetHandlers(scb_command_handler_t c,scb_session_handler_t s){assert(c&&s);registered_command=c;registered_reset=s;}')
s=s.replace('Tuning_HandleCommand(id,&command);','registered_command(id,&command);').replace('Tuning_Service();','CommandDispatch_Service();').replace('Tuning_ResetSession();','registered_reset();')
wr(p,s)
p=T/'spi_log_export.py';s=p.read_text(encoding='utf-8').replace('Tuning_ResetSession();','CommandDispatch_ResetSession();');wr(p,s)
p=T/'g2_system.py';s=p.read_text(encoding='utf-8').replace('uint8_t SCB_QueueEvent(const scb_event_t*e){(void)e;assert(0);return 0;}','void CommandDispatch_PublishRunStopped(uint8_t r){(void)r;assert(0);}') .replace('void Tuning_FinalizeStop(void){assert(0);}','void TuningApplication_FinalizeStop(void){assert(0);}')
wr(p,s)
for n in ['scb_protocol.py','scb_link.py','datalog_v2.py']:
    p=T/n;s=p.read_text(encoding='utf-8')
    s=s.replace('APP / "scb_protocol.c"','APP / "scb_protocol.c", APP / "tuning_values.c"').replace("str(APP / 'scb_protocol.c')","str(APP / 'scb_protocol.c'),str(APP / 'tuning_values.c')")
    wr(p,s)
