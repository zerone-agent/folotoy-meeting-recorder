# 会议纪要分享 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** 为会议创建有效期可配置、可选包含逐字稿的只读分享链接，展示最新纪要。

**Architecture:** SQLite 保存凭证哈希和权限；读事务在同一快照校验和读取内容。匿名分享页面使用 fragment 凭证访问独立的只读接口；恢复以最新安全库分享状态为准。

**Tech Stack:** Node.js 22.13+、Hono、node:sqlite、原生浏览器 JavaScript、node:test。

**Spec:** `docs/superpowers/specs/2026-11-10-meeting-share-design.md`。

## Global Constraints

- 默认 7 天，允许 1–365 整数天；纪要必选，逐字稿默认不选。
- 仅存随机 256 位凭证的 SHA-256 哈希；不记录凭证或会议正文。
- 不新增前端构建链；不提交、推送或部署，除非用户授权。
- 删除提交后新读不可访问；在途快照允许完成。分享数据不能返回结构化证据原文。
- 恢复依赖可信最新安全库；无标记且无表时清空分享并报告，状态不一致或无效时失败。

## Review Focus

- 无法复制链接时仍能手动复制：任务 4 的浏览器验证。
- 重转写期间不能泄露不匹配的逐字稿：任务 1 的事务测试。
- 创建与删除竞争不能产生有效孤儿分享：任务 1 的并发测试。
- 匿名例外不能绕过管理员认证：任务 3 的路由测试。
- 备份后删除的链接不能恢复：任务 2 的恢复测试。

### Task 1: 分享持久化和原子读取

**Files:** Modify `server/src/store.mjs`, `server/src/maintenance.mjs`; create `server/src/share-store.mjs` and `server/test/share-store.test.mjs`。

**Interfaces:** `initializeShares(db)` 原子建立和校验表及标记；`createShare(db, meetingId, includeTranscript, days, now)` 返回 `{token, expiresAt}`；`readShare(db, token, now)` 返回授权的纪要 Markdown、可选逐字稿或不可用状态；`purgeShares(db, now)` 删除过期记录。可按现有 store 事务约定调整封装，但接口语义不变。

- [ ] 添加测试：迁移、标记与表不一致、参数边界、随机凭证和哈希存储、到期边界、禁选逐字稿及证据不泄露。
- [ ] 添加两个连接的受控交错测试：读取快照分别先于和晚于删除、重转写、结果发布；创建与删除竞争；验证快照完整及删除后新读取失败。
- [ ] 运行 `cd server && node --test test/share-store.test.mjs`，确认新增测试先失败。
- [ ] 实现事务内创建与读取，校验纪要关联的逐字稿版本；删除清理和定期过期清理纳入现有维护流程。
- [ ] 再运行上述测试并运行 `node --test test/store.test.mjs test/maintenance.test.mjs test/retention.test.mjs`，全部通过。

### Task 2: 恢复分享状态

**Files:** Modify `server/src/restore.mjs`, `server/BACKUP.md`, `server/src/safe-error.mjs`; test `server/test/restore.test.mjs`。

**Interfaces:** 恢复结果包含分享状态警示；读取安全库同一快照，替换目标分享集合，只保留合法、未过期且会议仍有效的记录。

- [ ] 添加恢复测试：备份后创建、删除和过期的分享；会议删除；表和标记均缺失时清空并报告；仅缺一项、未知标记、无效记录、身份或完整性失败时回滚并移除输出。
- [ ] 添加迁移及降级后恢复测试，断言未验证状态不会从备份复活链接；记录一致性回滚不可检测的运维前提。
- [ ] 运行 `cd server && node --test test/restore.test.mjs`，确认新增测试先失败。
- [ ] 在现有恢复事务中校验并替换分享记录，向 CLI 报告失效状态；更新备份文档及安全错误白名单。
- [ ] 重跑 `node --test test/restore.test.mjs test/backup.test.mjs`，全部通过。

### Task 3: 创建和匿名访问接口

**Files:** Modify `server/src/routes.mjs`, `server/API.md`; test `server/test/routes.test.mjs` and `server/test/share-routes.test.mjs`。

**Interfaces:** 管理员 `POST /admin/meetings/:id/shares` 接收 `{includeTranscript, days}`，返回 `{token, expiresAt}`；匿名 `GET /share` 提供页面，`GET /share/data` 用专用 Bearer 分享凭证返回授权内容；其他路由保留原认证。

- [ ] 测试管理员鉴权、严格 JSON 字段、参数错误、有效分享、过期与删除、暂不可用、逐字稿权限和安全响应头。
- [ ] 测试 `/share` 和 `/share/data` 之外的路径、方法不能匿名绕过认证；页面和接口均不暴露凭证到 URL 请求。
- [ ] 运行 `cd server && node --test test/share-routes.test.mjs`，确认新增测试先失败。
- [ ] 在认证中间件之前精确注册两个 GET；实现创建接口、数据投影和统一不可访问响应，更新 API 文档。
- [ ] 运行 `node --test test/share-routes.test.mjs test/routes.test.mjs test/public-fetch.test.mjs`，全部通过。

### Task 4: 管理弹窗和分享页面

**Files:** Modify `server/src/ui.mjs`; create `server/src/share-ui.mjs`; test `server/test/ui-workflow.test.mjs` and share UI tests。

**Interfaces:** `shareUi()` 返回带 CSP 和 no-referrer 的页面；管理页基于当前 origin 生成 `/meeting-service/share#<token>`，调用创建接口。

- [ ] 添加测试：纪要必选且禁用、逐字稿默认不选、默认 7 天、错误保留输入、成功展示链接与到期时间；复制失败时可手动复制。
- [ ] 添加分享页测试：fragment 凭证只经请求头发送；加载、失效、暂不可用、人工复核、截断提示、可选逐字稿及安全文本渲染。
- [ ] 运行相关 UI 测试并确认新增测试先失败；实现可键盘操作的弹窗和只读分享页面。
- [ ] 运行相关 UI 测试及 `cd server && npm test`，全部通过。
- [ ] 使用本地合成数据浏览器检查桌面及窄屏、键盘焦点、复制和错误状态；记录实际验证范围。

### Task 5: 最终验证

**Files:** Modify `server/README.md`, `server/OPERATIONS.md` as needed。

- [ ] 检查设计逐项覆盖、恢复报告、凭证不进入日志及链接的公网可达性说明。
- [ ] 运行完整服务端测试；记录环境、结果和任何未完成的浏览器或并发验证。
- [ ] 检查 `git diff --check`、工作区状态及变更范围，向用户报告结果。
