# 参与贡献

欢迎通过 Issue 讨论问题或提出改进，再提交范围清晰的 Pull Request。请说明复现条件、预期和实际行为；固件变更应注明设备、ESP-IDF、分区和测试范围，避免把模拟或主机测试表述为实机通过。

- 服务端：使用 Node.js 22.13.0+，在 `server/` 执行 `npm ci`、`npm test`；测试可能需要 FFmpeg、ffprobe、OpenSSL 和本机回环端口。
- 固件：先读 [firmware/AGENTS.md](firmware/AGENTS.md) 和[构建测试文档](firmware/docs/development/engineering/build-and-test.zh_CN.md)，按变更范围运行对应检查；刷写设备需明确目标及数据影响。
- 文档：尽量链接原始证据，区分当时的设计、推测、模拟和设备实测；不要提交真实录音、未授权转录、凭据、私钥或未脱敏日志。
- 请保留现有第三方版权和许可证声明。提交贡献即表明你有权按本项目及相关第三方许可证提供该贡献。

安全漏洞请按 [SECURITY.md](SECURITY.md) 私下报告，不要在公开 Issue 中贴出可利用细节或个人数据。
