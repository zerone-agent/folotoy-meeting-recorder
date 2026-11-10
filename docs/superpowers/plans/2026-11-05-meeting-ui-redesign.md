# Meeting UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox syntax.

**Goal:** 用 Agent Hub 风格重做会议录音工作台。

**Architecture:** 保留 `server/src/ui.mjs` 的服务端生成 HTML、原生浏览器代码和现有 API。重构界面及请求状态，保留令牌缓存、CSP 和文本安全渲染。

**Tech Stack:** Node.js、原生 HTML/CSS/JavaScript、node:test。

**Spec:** `docs/superpowers/specs/2026-11-05-meeting-ui-redesign-design.md`

## Global Constraints

- 不引入前端构建链或外部资源；保留七天令牌缓存及现有 API。
- 沿用 Agent Hub 暖灰和陶土橙主题；适配桌面和手机及键盘操作。
- 显示真实时间可信状态、上传完整性、处理状态和删除清理状态；搜索筛选限定已加载项。
- 不执行线上删除、重试或发布操作。

## Review Focus

- 认证失效或退出时旧请求晚到：不恢复已清除的敏感详情。
- 切换会议、刷新列表或删除时响应乱序：不显示过时详情。
- 录音未确认或完整性未验证：不可下载，提示原因。
- 数据跨页或时间未校准：保持服务端顺序并明确筛选仅限已加载项。
- 空列表、失败、截断、转录中及屏幕窄尺寸：有明确且可操作的状态。

---

### Task 1: 工作台与状态管理

**Files:** Modify `server/src/ui.mjs`; Test `server/test/ui-token-cache.test.mjs` and new `server/test/ui-interactions.test.mjs`.

**Interfaces:** `meetingUi()` 继续返回带 CSP 的 `Response`；继续调用现有 `admin/meetings`、`admin/deletions` 和详情、下载、重试、删除接口。

- [ ] 增加针对认证过期、退出和会议切换时晚到响应、删除失败和音频不可下载的测试；先运行确认失败。
- [ ] 重做登录、列表、搜索筛选、分页和详情，保留内容文本安全渲染、时间标注、删除警示、纪要/转录切换及管理动作。
- [ ] 使用请求代次和必要的取消机制防止晚到响应恢复已清除或过时的页面状态。
- [ ] 运行 `node --test server/test/ui-token-cache.test.mjs server/test/ui-interactions.test.mjs`，确认通过。

### Task 2: 外观与浏览器验收

**Files:** Modify `server/src/ui.mjs`; tests in `server/test/`.

**Interfaces:** 继续使用 Task 1 的页面和接口；不改服务端协议。

- [ ] 根据 Agent Hub 的主题实现桌面双栏、手机单栏、焦点及响应式状态，检查中英文长文本和减少动画设置。
- [ ] 使用本地测试数据在浏览器验证登录、刷新恢复、筛选分页、详情、下载可用性、重试和删除确认；不操作线上真实数据。
- [ ] 运行 `cd server && npm test`；记录实际通过和未验证范围，检查工作区差异。
