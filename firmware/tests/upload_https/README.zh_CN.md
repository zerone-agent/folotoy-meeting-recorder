<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 本机 HTTPS 上传集成测试

此可选测试验证错误 CA 被主机传输层拒绝，以及固件拒绝摘要被篡改的确认响应。它还运行固件的 SCR1 扫描、投影、哈希、分块协议和确认校验，通过本机 HTTPS 对接仓库中的服务端路由及临时 SQLite 数据库。测试在上传中途模拟传输失败并重启服务；服务端确认后丢弃响应，再次重启并验证幂等重试。

另用独立数据库模拟没有分块窗口接口的旧服务：在状态响应超过设备缓冲后中断上传，并在同一客户端进程内重试，验证只补传尚未确认的分块并获得最终确认。游标保存在内存，设备重启后会从头核对。

另一轮在游标已记录后从临时数据库删除一个分块，验证最终提交拒绝缺块、设备清除游标并从头补传，之后才接受完整确认。

运行前需要服务端支持的 Node.js、已安装的 `server/node_modules`、C 编译器、curl 开发文件、OpenSSL 开发文件及命令行工具，以及 ESP-IDF 5.5.3 源码。设置 `IDF_PATH`；如果无法通过 `brew --prefix openssl@3` 定位 OpenSSL，请设置 `OPENSSL_PREFIX`。

```sh
export IDF_PATH=/path/to/esp-idf-5.5.3
export OPENSSL_PREFIX=/path/to/openssl
./firmware/tools/test_upload_https.sh /absolute/path/to/recording.dat
```

输入须为有效的 90,000 帧 SCR1 文件，例如已授权的设备只读导出录音，且足以分成至少两个 4,096 字节的块。测试不修改输入；测试凭据、证书和 SQLite 数据存放于私有临时目录，结束时删除。输出仅包含计数与结果，不包含音频或凭据。

主机使用 libcurl 和 OpenSSL 完成网络与 SHA-256，不覆盖设备 ESP-TLS、Wi-Fi、Flash 或确认后的清理；测试通过不代表设备上传或断电恢复验收通过。
