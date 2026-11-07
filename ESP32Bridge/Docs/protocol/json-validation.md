# JSON 全量校验与精确整数

核对基线441a778，2026-11-07。实现为 [json_guard.c](../../main/json_guard.c)、[声明](../../main/json_guard.h)；设计来源 [Phase02](../../specs/features/02-scb-protocol/group-2-design.md)。本端只验证表示，不判车辆参数。

<a id="f02-json"></a>
## F02 JSON机制

bridge_json_validate接受显式1～992字节，不要求NUL结尾；空指针/工作区、零长或超长返回ARGUMENT。根可为对象、数组或标量，业务对象要求由调用者追加。仅空格、Tab、CR、LF为空白。ROOT_VALUE消费根值后进入ROOT_END，只允许空白并恰好到payload_len；第二个值、尾随垃圾和截断返回SYNTAX。工作区成功失败均可能改写，不发布部分结果。

容器不递归：state[0]保存根状态；对象OBJECT_FIRST允许空对象，随后按键→OBJECT_COLON→OBJECT_VALUE→OBJECT_COMMA推进；逗号后OBJECT_KEY必须有键。数组ARRAY_FIRST允许空数组，值后ARRAY_COMMA，逗号后ARRAY_VALUE必须有值。父状态先置为值已消费，再压入子状态和object_start偏移，闭括号弹回父层。错括号、缺冒号、尾逗号均拒绝。

BRIDGE_JSON_MAX_BYTES/2+1为497项，含根，容器最多496层；入栈前depth==496拒绝。992B恰可容纳496层空数组，497项不能解释为497层容器。object_start为uint16_t[497]，state为uint8_t[497]，成员数据1491B，实际对象还受ABI尾部对齐影响。各SPI/NET调用任务独占自己的工作区，不共享可写解析状态；无堆、无递归，返回不发布workspace指针。

scalar/string_end拒绝未转义控制字符、坏转义、截断、UTF-8坏续字节/过长编码/代理区/超过U+10FFFF；接受简单转义及四位Unicode转义，高代理必须跟合法低代理，孤立低代理拒绝。缺闭引号也走UTF8错误。duplicate重扫当前对象此前直接键，跳过字符串和深层容器，equal_keys逐解码Unicode标量比较：`"a"`与`"\u0061"`重复；不同对象同名键允许。不做Unicode规范化。二次扫描换取无堆键表，不保证线性时间。

number验证符号、整数、小数和指数语法；前导零后的数字由外层拒绝。按有效十进制位/小数位/指数求数量级：零有限；数量级小于308接受，大于308返回NONFINITE；等于308逐位对比精确最近舍入溢出边界2^1024−2^970的309位十进制整数，相等也溢出。允许下溢到带符号零，不调用locale/浮点转换；NaN/Infinity不是JSON字面量。

原始字节入口object_get先清view并验证全文，要求根对象，按解码键匹配顶层ASCII名称，扫描完整嵌套值并去掉外部空白；找不到/非法返回false及空view。view借用原输入、无NUL，输入必须在读取完前有效且不变。string_equal对已验证字符串逐标量比较ASCII。

<a id="f02-u32"></a>
## 精确u32

bridge_json_u32进入先清零有效输出，拒绝空、超992B及任何负号（包括-0）。number重新验证且全量消费；扫描尾数的小数位和末尾零，将末尾零移入十进制power，逐位用(UINT32_MAX-d)/10检查累积。零直接成功；非零要求power为0～9，逐次乘10并检查UINT32_MAX/10，不做舍入或截断。1.0、1e2、10e-1可成功；1.1、1e-1、4294967296失败。成功范围0～4294967295，失败输出零。

## 参数与证据

| 来源/参数 | 值、类型、单位 | owner、生效与复位 |
|---|---|---|
| json_guard.h BRIDGE_JSON_MAX_BYTES | 992u，字节上限 | 编译固定，调用者传实际长度 |
| workspace.object_start/state | uint16_t[497]/uint8_t[497] | SPI/NET各任务私有；每调用重设根、覆盖所用栈项 |
| validate入栈条件 | 最大496层容器 | 单次局部depth，返回终止 |
| number overflow常量 | binary64舍入溢出阈值 | 编译固定，仅检查有限性 |
| u32 UINT32_MAX | 无符号整数上限4294967295，无单位 | 每次转换，失败输出零 |

codec将上述错误统一折为SCB_JSON；直接API仍区分ARGUMENT/SYNTAX/UTF8/DUPLICATE_KEY/NONFINITE。历史见 [P02 G1](../validation/phase-02-group-1.md)、[G3](../validation/phase-02-group-3.md)。本次逐分支静态核对，未重跑宿主或目标；解析成功不代表业务或三端验收。

## Phase02 G4 检查复用

2026-11-07已按[检查简化规约](../../specs/features/02-scb-protocol/check-simplification.md)同步实现。外部字节完整验证保留，内部只在来源已验证、输入仍有效且不变时复用视图。实际审查与构建结果见[本组记录](../validation/2026-11-07-check-simplification.md)。本组不改变协议接受范围、队列额度、超时及阶段验收状态。

bridge_json_object_get仍先验证全文，再委托bridge_json_object_get_validated定位字段。后者输入bridge_json_view_t，不持工作区，不复验全文；仅接受完整验证通过的根或由合法查询返回的子视图，要求借用期间输入不变。对象要求、找不到字段的空view、字符串语义比较和精确u32转换保持。查询得到的args可以直接查询其成员，不需要把args再完整解析一次。
