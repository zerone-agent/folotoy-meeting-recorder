# FoloToy AI Passport 专项调研

整理 FoloToy AI Passport 的产品体验、技术方案、相关资料与验证记录。

## 调研资料

- [产品与技术初步调研](research/2026-10-29-ai-passport-overview.md)：产品功能、硬件与固件方案、安装方式、官方资料索引及待确认事项。
- [AI 玩法与应用架构](research/2026-10-29-ai-app-architectures.md)：具体玩法对比、设备与手机或电脑的分工、云端和模型服务链路，以及凭据与验证重点。

- [会议录音卡可行方案](research/2026-10-29-meeting-recorder-feasibility.md)：社区现成方案、官方社区入口及离线录音架构比较。

- [会议录音卡参考项目方案](research/2026-10-29-meeting-recorder-reference-plan.md)：官方基线、容量估算和离线录音验证计划。

- [录音转录与内容分析](research/recording-test-2026-10-29/analysis.md)：存满录音的恢复、自动转录结果与识别限制。

- [阶跃星辰 ASR 接入调研](research/2026-10-30-stepfun-asr.md)：SSE 与 WebSocket 的流式边界、模型与待验证事项。

## 会议录音卡原型

- [固件设计](docs/superpowers/specs/2026-11-02-scr1-auto-upload-design.md)和[实施计划](docs/superpowers/plans/2026-11-02-scr1-auto-upload.md)：离线保存、只读恢复、配网和自动上传的要求与未完成项。
- [实机测试记录](research/2026-11-03-agent-commissioning-upload-test.md)：构建版本、设备行为、故障及结果的证据范围。
- [上传内存配置候选](firmware/sdkconfig.meeting-upload.defaults)：针对 ESP32-C3 的可选构建参数；该候选仍需完成设备验收。
- [会议服务](server/README.md)：接收录音、转录和纪要服务。

已在受控实验固件上完成 30 分钟录音的上传、服务端摘要核对及设备清理核查，也在可复现候选源码的短录音测试中获得服务端确认。正式的自动认证、证书更新，以及最终配置的断电恢复和端到端验收尚未完成；测试范围和条件以实机记录为准。
