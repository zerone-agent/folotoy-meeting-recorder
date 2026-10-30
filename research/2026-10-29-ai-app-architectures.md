# AI Passport：AI 玩法与应用架构

调研日期：2026-10-29。本文基于各项目公开说明和源码仓库的作者描述，**未安装固件、运行服务或实机验证**；项目版本和线上服务可能变动。产品及硬件背景见[初步调研](2026-10-29-ai-passport-overview.md)。下文“作者描述”不等于本次复现通过，也不代表所有 AI Passport 都具备该功能。

## 一、先区分三个层次

AI Passport 是输入输出硬件和可刷写固件的平台：麦克风与按键采集输入，屏幕和扬声器呈现输出；选用的玩法决定数据走向。手机可能只用于配网或绑定，也可能本身承担识别；电脑可能是必需的语音网关；模型可以由远程服务提供，具体取决于玩法。官网出厂身份卡通过小程序本地蓝牙同步个人资料，不应推断出厂固件自带所列 AI 玩法。[官网](https://ai-passport.folotoy.cn/)

| 玩法 / 来源 | 设备的角色 | 手机 / 电脑角色 | 云端或模型 | 关键约束 |
| --- | --- | --- | --- | --- |
| [Qoder-Passport](https://github.com/kunlun322/Qoder-Passport)，社区固件 | 采音、播放、显示文字和状态；Wi-Fi 直连服务 | 手机或电脑打开设备热点网页配网和填写令牌；对话链路未描述为经手机中转 | Qoder Cloud Agents 实时 WebSocket；具体模型待确认 | 作者描述全双工和打断；需 Qoder 账号、PAT 和 Wi-Fi |
| [千问语音豆](https://github.com/QwenAudio/qwen-audio-agent/blob/main/examples/ai-passport/README_ZH.md)，QwenAudio 示例及外部固件 | 采音、播放、显示、控制；经 Wi-Fi 连接电脑 | 电脑同时运行设备转发器和 Gateway，必须保持运行 | Gateway 使用所配置的语音前台，可调用工具和可选后台 Agent；实际模型、服务端及费用取决于 Gateway 配置 | 当前示例半双工，播放时暂停上行；按键手动打断 |
| [语音输入与 AI 控制面](https://github.com/zhaohuaxiaoy/folo-ai-passport-voice)，社区项目 | 采音并显示状态/审批；BLE 或 USB 与电脑通信 | Mac/Windows companion 转发音频、显示候选字并向当前焦点输入框注入文本 | 火山引擎流式 ASR；目标 Agent 使用何种模型取决于用户打开的应用，ASR 本身不是对话模型 | 需要电脑和 ASR Key；审批功能默认关闭 |
| [AI Passport Muse](https://github.com/RongleCat/ai-passport-muse)，社区移植 | 按住录音、发送并显示文本回复；连接 Wi-Fi | 手机 Muse App 用于配对；作者还描述可按网络情况使用局域网代理 | Muse 服务；具体模型及语音处理位置待确认 | 作者明确称此分支无语音播报、图片推送或家庭网络隧道；不能替其他 Muse 版本推断 |
| [小智 AI 对话机器人](https://ai-passport.folotoy.cn/plays/72/?v=118-4)，FoloToy 适配玩法 | 官方页面介绍刷入后进行语音对话 | 具体配网和绑定流程待核实 | 具体部署、服务商、模型与数据留存待核实 | [FoloToy 相关仓库](https://github.com/FoloToy/folo-ai-passport-xiaozhi)存在；本次未核对该玩法安装包与仓库版本是否一致 |
| [动物护照](https://ai-passport.folotoy.cn/plays/)，社区玩法展示 | 角色孵化、展示及互动 | 作者称需 Android App；拍照后的识别和穿搭处理在手机本地，结果送设备 | 作者称无需付费 AI，角色由预绘素材组合 | 页面声明原图不上传或保存；未独立核验，具体模型和传输协议未公开 |

这些方案说明同一硬件可以采用不同分工；不能把某个社区玩法的模型、权限和能力当成平台默认配置。

## 二、具体链路

### 1. Qoder-Passport：设备直连云端实时 Agent

```text
手机或电脑浏览器 ──配置──> 设备热点页面（Wi-Fi 信息 + Qoder PAT）
设备麦克风 ──Wi-Fi / WebSocket──> Qoder Cloud Agents
设备扬声器、屏幕 <──语音及转写状态── 云端实时会话
```

据[作者 README](https://github.com/kunlun322/Qoder-Passport)，设备启动后连接 Wi-Fi，在屏幕上选择 Agent 模板和用户身份，打开实时语音会话；项目称支持双向打断，上行麦克风 PCM，下行为通过 WebSocket 传输的 24 kHz PCM。未配置网络或 PAT 时设备开启 WPA2 配置热点并显示二维码、临时密码和地址。PAT 保存在设备 NVS；整包刷写可能清除已保存设置。手机在这里是**配置入口**，不能据此称手机是运行时语音中继。

**待确认：** 真实版本所用模型、音频存储及服务端处理细节、令牌撤销与本地加密、延迟/打断效果和计费；README 中的能力未经本次运行验证。

### 2. 千问语音豆：电脑承担设备网关

```text
Passport 录音/播放
    ⇅ 局域网 Wi-Fi，ws://电脑:3101/api/realtime
电脑 device-relay：校验设备令牌、拆包和缓冲
    ⇅ 本机连接
电脑 qwen-audio-agent Gateway：实时对话、工具与可选后台 Agent
    ⇅ 按 Gateway 配置连接语音模型/服务
```

[QwenAudio 的 AI Passport 示例](https://github.com/QwenAudio/qwen-audio-agent/blob/main/examples/ai-passport/README_ZH.md)说明，卡片固件由外部项目维护，设备连接电脑转发器，转发器再连接同一电脑上的 Gateway；设备令牌与模型 API Key 是两种凭据，后者保存在 Gateway 配置。示例使用明文 WebSocket，官方示例明确限定可信局域网，并要求不暴露公网。电脑需保持唤醒。

当前版本作者说明是**半双工**：播放时停止麦克风上传，按键可手动打断。项目列有设备鉴权、音频拆包和恢复的自动测试，但示例也要求验证真实录音和 Wi-Fi 稳定性；本次未运行这些测试。Gateway 连接何种具体模型及服务取决于实际配置，不应仅凭“千问语音豆”名称指定型号或服务位置。

### 3. 语音输入与 AI 控制面：电脑中转 ASR，再交给当前应用

```text
设备采集语音 ── BLE 或 USB ──> 电脑 companion ──流式音频──> 火山引擎 ASR
设备屏幕 <──状态/审批── 电脑 <──部分/最终文本── ASR
                           └──最终文本注入当前焦点输入框──> 用户选择的应用 / Agent
设备按键 ──审批结果──> 电脑 / Agent（依具体配置）
```

[作者 README](https://github.com/zhaohuaxiaoy/folo-ai-passport-voice)描述设备上行 16 kHz 音频、100 ms 分帧；电脑负责转发云 ASR、显示候选字以及注入最终文本。桌面端需配置火山 ASR Key；macOS 注入需辅助功能授权。Agent 状态及审批可回传设备，但作者注明桌面审批默认关闭。项目作者报告过实机测试，也列出 Windows 焦点、长句和断线等待验收事项；本次没有复现。

这里需要区分 **ASR（音频转文字）** 与接收文字的 Agent：后者可以使用不同模型，ASR 成功并不证明 Agent 已生成回答或正确完成操作。

### 4. Muse：手机配对、设备发语音、屏幕显示回复

[社区移植仓库](https://github.com/RongleCat/ai-passport-muse)的 `passport-port` 分支说明：使用手机 Muse App 进行设备配对，固件需 Muse SDK token 与网络；按住 OK 录音、松开发送，回复以文本显示。作者说明可直连 Muse，或通过局域网代理经 HTTP CONNECT 再建立 TLS；具体网络配置视用户环境而定。该版本不支持语音播报、图片推送或家庭网络隧道，且作者提示配对缺少厂商证书。具体模型、音频与对话在服务端的处理和保存方式仍待核实。

## 三、共性与验证重点

1. **配对和运行分开看：** Qoder 的手机热点页、Muse 的手机配对、千问的电脑网关各有不同角色；配网用到手机不代表对话经手机中转。
2. **凭据与数据分开核对：** 分别记录 Wi-Fi 密码、设备接入令牌、模型 API Key/PAT 存放位置与删除方法；音频、转写、对话、设备标识和日志的去向由各服务决定。上述资料不足以判断所有项目的加密或留存策略。
3. **体验指标需要实测：** 对每个玩法记录固件来源和版本、启动/配网步骤、是否依赖电脑、首字及首音延迟、手动/语音打断、掉线恢复、耗电和费用；对可能删除设备内容的安装先记录恢复方式。
4. **架构图不等于安全审计：** 关注设备身份认证、明文链路范围、证书校验、密钥导出及日志，尤其是局域网转发器、设备热点和社区固件；逐项目验证后再下结论。
5. **后续深入：** 对官方小智安装包与对应源码版本建立可追溯关系，核实具体网络协议、语音/模型服务、绑定流程、收费与数据政策；对千问、Qoder 和 Muse 分别进行端到端实测并留存截图及版本信息。
