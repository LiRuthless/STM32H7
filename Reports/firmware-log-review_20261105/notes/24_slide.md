先确认不变量，再用板上证据关闭性能与联调问题。

本次仅制作讲解资料，没有改动固件或重新运行固件测试。历史Phase16软件验证、双LCD构建与Phase17软件收口不等于三端实机闭环。700/900us预算、SPI波形和持续吞吐必须实测；AC5与栈高水位保持待验。文档D08说明本端next_seq临近回绕会重建，是否还要求接收端检测对端临界序号尚未决定。32项标定仍为-1导致不能起跑，需真实标定后才具备实车验证条件。

源码与契约：SmartCar/Docs/known-issues.md；SmartCar/Docs/services/communication.md#f24；SmartCar/specs/roadmap.md。