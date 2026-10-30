# FoloToy AI Passport：产品与技术初步调研

调研日期：2026-10-29（中国时间）。本记录基于可公开访问的产品官网、官方教程、FoloToy GitHub 仓库与社区页面；是资料研究，**尚未取得设备、未实机体验、未编译或刷写固件**。网站内容、社区数据与仓库主分支会变化。

## 一、产品定位与使用方式

AI Passport 是透明外壳、可佩戴的开放式设备，官方强调“Wear / Play / Create”：出厂身份卡、安装社区玩法、自己开发玩法。官网标称尺寸 60 × 95 × 8.5 mm、重量 50 g、电池 500 mAh；透明外壳展示屏幕、电路板、NFC、麦克风、扬声器和电池。[官网](https://ai-passport.folotoy.cn/)

| 使用方式 | 官方描述及边界 |
| --- | --- |
| 开箱身份卡 | 首次开机无需安装其他内容；微信小程序通过本地蓝牙同步名称、头像、简介、全屏图片。默认玩法内置小游戏。[官网](https://ai-passport.folotoy.cn/) |
| 社区玩法 | 浏览社交、游戏、效率等玩法并安装；官网提示安装社区玩法会覆盖设备当前全部内容。每个玩法的功能、联网与兼容性应单独核实。[官网](https://ai-passport.folotoy.cn/) · [玩法社区](https://ai-passport.folotoy.cn/plays/) |
| 自行创作 | 可从官方参考项目出发，用编程工具或 AI 编程 Agent 设计、构建并安装应用。官方要求测试设备验证，并记录恢复方法。[创作教程](https://ai-passport.folotoy.cn/guides/create-your-own-play/) |
| 定制 | 官网列出前面板 LOGO、开机界面、专属玩法，轻定制 100 件起订；价格与交付细节需另行确认。[官网](https://ai-passport.folotoy.cn/) |

社区页面本次访问显示“1000+ 款玩法”，并列出游戏、效率、学习、社交等分类；这是页面展示的动态数量，不等于经过独立核验的可用或活跃应用数量。[玩法社区](https://ai-passport.folotoy.cn/plays/)

**AI 能力边界：** 官网将其定位为开放式可穿戴 AI 平台，但出厂体验主要是身份卡和小游戏；具体 AI 对话、语音、联网及其服务供应商取决于所安装玩法。不能由硬件定位推断默认固件已有某个大模型、云端服务或本地推理能力。社区例子 Muse Passport 声称需手机配合并提供语音对话，属于社区玩法的作者描述，并非对整个平台的验证。[官网](https://ai-passport.folotoy.cn/) · [玩法社区](https://ai-passport.folotoy.cn/plays/)

## 二、硬件和软件技术方案

以下为官方参考仓库 `main` 所描述的板级能力及其实现边界；并不代表每个出厂固件或社区玩法都实现了这些功能。[主仓库硬件能力表](https://github.com/FoloToy/ai-passport#hardware-capability-contract)

| 部件 | 已公开资料 | 工程边界 |
| --- | --- | --- |
| 主控和存储 | ESP32-C3、8 MB Flash、无 PSRAM。 | 参考分区为 NVS、PHY 和一个 factory 应用，其他应用可以使用不同有效布局。 |
| 屏幕 | ST7789P3，240 × 320，RGB565，SPI2 40 MHz；LEDC 调背光。 | 小型单 DMA 缓冲；参考 BSP 未暴露 LCD MISO、触摸、TE 接口。 |
| 输入 | UP、DOWN、OK 通过 GPIO0 ADC 电阻梯连接。 | 回调不能阻塞；不要重复创建 ADC1。 |
| 音频 | ES8311、I2S0 全双工 PCM，支持播放、麦克风采集和软件休眠。 | PCM 读写阻塞，应放入工作任务；休眠前停止音频 I/O。 |
| 电池 | CW2017 读取电量及电压；标称电池 500 mAh。 | 电量监测在运行时可选，结果受电芯和 profile 影响，不等同校准值。 |
| 无线 | ESP32-C3 支持 2.4 GHz Wi-Fi 与 BLE。 | 主仓库示例仅扫描 Wi-Fi、发送不可连接 BLE 广播；不等同已经实现联网配网或完整 BLE 应用。射频、共存和功耗需实测；不支持经典蓝牙。 |
| NFC | 官网展示 NFC；Arduino 库说明为未连接 MCU 的无源 NFC 标签，该库无软件接口。 | 不应假定应用固件能读写该 NFC。[Arduino 库](https://github.com/FoloToy/ai-passport-arduino) |
| 调试和刷写 | ESP32-C3 原生 USB Serial/JTAG；GPIO18/19 保留给 USB。 | 主仓库指出 GPIO21 与背光冲突；刷机可能影响数据和分区。[主仓库](https://github.com/FoloToy/ai-passport) |

### 固件架构与开发

- [主仓库](https://github.com/FoloToy/ai-passport)的 `main` 是可运行的硬件测试基线，不是完整成品应用；`components/bsp` 提供板级接口，`main` 包含应用页面、状态和任务，`tests` 提供无需硬件的轻量逻辑测试。官方要求新应用重新设计界面，而非照搬测试菜单。
- 仓库使用 ESP-IDF、FreeRTOS 和 LVGL；Wi-Fi/BLE 由应用层使用 ESP-IDF 服务，而非主仓库 BSP 自带完整网络应用。演示分支展示游戏、持久化、音频和 BLE 等具体方案，不应当作 `main` 默认功能。[主仓库](https://github.com/FoloToy/ai-passport)
- [Arduino 驱动库](https://github.com/FoloToy/ai-passport-arduino)提供屏幕、按键、音频和电量接口及例子，标注为早期 0.1.0，完整实板验收仍待完成；库本身不需要云账号、API Key 或网络。其许可和第三方依赖须分别查看仓库声明。
- 官方创作教程要求提供可从 `0x0` 刷写的合并固件、说明测试及未验证项；本地浏览器安装工具读取本地文件，教程称不上传服务器。设备清除数据会删除当前配置及用户数据。[创作教程](https://ai-passport.folotoy.cn/guides/create-your-own-play/)

### 安装与恢复：区分不同实现

官网描述电脑 Chrome 或 Edge + Type-C 恢复默认固件，以及社区玩法安装覆盖当前内容。[官网](https://ai-passport.folotoy.cn/)

FoloToy GitHub 下的 [小程序玩法安装器](https://github.com/FoloToy/ai-passport-miniapp-installer)自称是由 SHLcy 维护的社区项目 592，处于测试中，不能将其永久 Recovery 机制直接等同于所有出厂设备或所有玩法：首次需要 USB 安装完整镜像，会清除旧玩法和已保存 Wi-Fi；之后小程序通过 BLE 配网、设备通过 Wi-Fi 下载、校验和安装；按 Power + Up 可进入安装模式。配套小程序是独立项目，未包含在该仓库中，文件大小本身不能保证兼容。

## 三、资料索引

| 资料 | 用途 |
| --- | --- |
| [产品官网](https://ai-passport.folotoy.cn/) | 定位、规格、开箱、定制及教程入口。 |
| [玩法社区](https://ai-passport.folotoy.cn/plays/) | 发现应用、阅读作者说明和安装入口；内容与数量动态变化。 |
| [创造自己的玩法](https://ai-passport.folotoy.cn/guides/create-your-own-play/) | 参考项目、AI 编程、构建、实机验证与本地安装流程。 |
| [开发主仓库](https://github.com/FoloToy/ai-passport) | 硬件能力表、BSP、示例、开发文档、构建与刷机规则。 |
| [Arduino 驱动库](https://github.com/FoloToy/ai-passport-arduino) | Arduino API、示例、依赖与验证状态。 |
| [小程序玩法安装器](https://github.com/FoloToy/ai-passport-miniapp-installer) | 社区维护的 BLE 配网、HTTPS 下载和 Recovery 实现。 |
| [飞书固件仓库](https://github.com/FoloToy/ai-passport-feishu) | 飞书消息应用的独立源码，可作为后续应用方案研究对象。 |
| [FoloToy GitHub 组织](https://github.com/FoloToy) | 查找相关仓库与后续更新。 |

## 四、待确认与下一步

1. **真实开箱体验：** 获取设备并记录硬件版本、固件版本、购入渠道及开箱流程，实测身份卡同步、内置游戏、充电和恢复。
2. **安装安全及兼容：** 明确出厂分区、身份数据和恢复机制；在测试设备上验证安装覆盖范围、断电恢复、社区固件来源与校验。刷写前确认备份和恢复方案。
3. **AI 应用架构：** 选取具体玩法，核实音频链路、手机/设备/云端职责、模型及语音服务供应商、密钥和费用；区分作者宣称与实测结果。
4. **能力和性能：** 实测续航、充电时间、Wi-Fi/BLE 距离与共存、音频质量、内存与 Flash 占用；当前公开资料不足以给出可靠数值。
5. **隐私与商业条件：** 核查用户资料、录音、凭据的保存与传输方式及删除流程；确认当前售价、供货、定制费用和售后条款。官网链接：[用户协议](https://ai-passport.folotoy.cn/)（页面底部入口）、[隐私与安全](https://ai-passport.folotoy.cn/)（页面底部入口）。
6. **开源范围：** 分别核对固件、第三方库、硬件设计资料及商业使用条款；仓库标注 MIT 不应直接推断全部硬件设计或所有玩法均以相同许可开放。
