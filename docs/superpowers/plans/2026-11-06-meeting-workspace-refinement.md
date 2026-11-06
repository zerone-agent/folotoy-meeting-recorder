# Meeting Workspace Refinement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** 在会议工作台同时展示转录与纪要，提供服务端全量搜索分页、主题标题和 Router 风格控件。

**Architecture:** 扩展管理员会议列表的可选查询参数，同时维持原游标接口；服务端计算标题、过滤和分页，页面使用自定义无障碍选择器和响应式双栏，沿用现有安全的 Markdown 渲染和编辑流程。

**Tech Stack:** Node.js、SQLite、Hono、原生 HTML/CSS/JavaScript、node:test。

**Spec:** `docs/superpowers/specs/2026-11-06-meeting-workspace-refinement-design.md`

## Global Constraints

- 搜索和筛选覆盖全部会议；每页可选 10、20、50、100，默认 20。
- 不能改变既有音频、纪要、认证和旧客户端游标接口语义。
- 标题优先使用模型生成的会议主题；缺失或无效时回退时间。
- 继承 Router Dashboard 的 Zerone 标志与细线图标；移动端可操作。

## Review Focus

- 中文搜索、特殊字符及超长查询必须按字面处理并限制输入。
- 并发更新和删除导致页码超出范围时回退到可用页。
- 快速切换筛选或分页时旧响应不得覆盖新状态。
- 编辑未保存时切换会议、刷新或退出须继续保护草稿。
- 缺失、损坏或未经校验的主题不能破坏列表或注入 HTML。

### Task 1: 服务端查询与标题

**Files:** Modify `server/src/store.mjs`, `server/src/routes.mjs`; test `server/test/store.test.mjs` and route tests as appropriate.

**Interfaces:** 为 `GET /admin/meetings` 增加显式分页模式，接受受限 `page`, `pageSize`, `search`, `state`，返回 `meetings`, `page`, `pageSize`, `total`, `totalPages`；原无分页参数的 `before` 游标模式保持不变。标题由已完成纪要的 `meeting.topic` 安全提取。

- [ ] 编写跨多页、每页大小、全量标题/ID 搜索、状态组合、非法参数、空结果、并发删除后越界及旧游标兼容测试。
- [ ] 运行针对性测试，确认新增用例先失败。
- [ ] 实现参数校验、参数化 SQLite 查询和安全标题提取；将总数与页面读取放在一致的查询快照中。
- [ ] 运行针对性测试并确认通过。

### Task 2: 工作台布局、选择器与分页

**Files:** Modify `server/src/ui.mjs`; add/update UI workflow tests under `server/test/`.

**Interfaces:** 页面调用分页接口，默认每页 20；搜索或状态变更重置页码，页大小变化重置页码；采用请求代次抑制乱序结果。标题优先服务端 `title`，回退原时间格式。

- [ ] 添加针对快速筛选、切页、旧请求返回、空页、编辑未保存保护及标题回退的测试并确认先失败。
- [ ] 将详情改为转录在左、纪要在右，窄屏纵向排列；保留告警、编辑和下载。
- [ ] 实现分页控件及 Router 样式状态/页大小选择器，支持鼠标、键盘、Escape、焦点和外部点击关闭；应用参考 Logo 和图标。
- [ ] 运行 UI 测试并确认通过。

### Task 3: 集成与视觉验证

**Files:** `server/test/`, implementation above.

- [ ] 运行 `npm test`，确认全部服务端测试通过。
- [ ] 使用隔离测试数据启动本地服务，验证 10/20/50/100、跨页搜索筛选、异常页面、桌面/手机双栏、选择器键盘操作、纪要编辑刷新及转录下载。
- [ ] 浏览器对照 Router Dashboard 检查 Logo、图标和选择框；记录测试结果与未覆盖范围。
