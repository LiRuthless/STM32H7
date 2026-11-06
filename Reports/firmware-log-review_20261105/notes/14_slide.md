通常每 10 ms 最多 10 条；边界到来可提前封包。

Telemetry_Service先固定head并DMB，只选择同一run、同一revision且index连续的1到10条。遇到缺口或身份变化时boundary置位，即使未到10ms也允许结束旧批。正常连续批未到10ms不消费。封包复制完成后才DMB并释放tail。revision在完整调参拍、暂停和恢复的既有时点发布，已入队槽不会被追溯改版本。

源码与契约：SmartCar/App/services/telemetry.c::Telemetry_Service；Telemetry_SetRevision。