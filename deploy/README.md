# 单机容器部署（待容器验证）

当前 HTTP 模式尚未发布镜像；必须先发布包含该改动的新镜像再使用此 Compose。生产使用前仍需完成实际部署环境验收。服务是单实例 SQLite 应用，不支持多个副本或共享网络文件系统。生产使用前须完成设备联调、安全审计、证书管理及备份策略。

1. Compose 使用 Docker 管理的 `meeting_data` 命名 volume 保存数据库、录音块、WAL 和任务。新 volume 初始化时从镜像复制非 root 用户所有的 0700 目录；已有 volume 必须保证目录由容器内 node 用户拥有且权限为 0700。HTTPS 由反向代理终止。
2. 在受保护位置创建权限 0600 的 `meeting.env`，提供 `MEETING_ADMIN_TOKEN`、`ZERONE_AGENT_HTTP_API_KEY`（互不相同且至少 32 个字符）及 `STEPFUN_API_KEY`；可选变量见 `server/OPERATIONS.md`。不要把文件或值提交到仓库。Compose 环境变量会被 Docker 管理员访问，运行主机须受信任。
3. 默认使用 Docker Hub 的 `zeroneai/meeting-recorder:latest`和本目录下的 `meeting.env`。无需手动创建数据目录。该镜像须包含 HTTP 模式；旧版 v0.1.0 不支持。执行 `docker compose config -q`，再执行 `docker compose up -d`。默认仅监听本机 3443；反向代理通过 `http://127.0.0.1:3443` 转发 `/meeting-service/`，对外提供有效 HTTPS 证书。保持本机绑定，不要将明文服务直接暴露到网络。
4. 通过反向代理，使用受信任 CA 和匹配的主机名访问 `/meeting-service/ui`；管理员状态接口需要令牌。停止用 `docker compose stop`，等待任务排水完成。

不要使用 `docker compose down -v`，它会删除持久数据卷。命名 volume 不是备份，须将数据库一致性快照及最新安全状态保存到独立位置。升级前执行 [备份与恢复](../server/BACKUP.md)：保全完整数据库和最新安全状态。确认新版兼容矩阵、镜像摘要以及备份可恢复；停止旧实例后升级。仅在证明旧版能读迁移后数据库时允许直接切回旧摘要，否则离线执行恢复工具；缺失最新安全状态或已确认数据时拒绝上线。备份频率、异地复制和灾难恢复目标须由部署方确定，工具本身不提供这些能力。

发布流程与 agent-hub 一致：推送 `v*` Git 标签后运行测试并构建，向配置的 Docker Hub 和 SWR 推送版本标签；稳定版本还更新 `latest`，预发布版本不更新 `latest`。配置变量 `REGISTRY_HOST`、`REGISTRY_IMAGE`、`DOCKERHUB_IMAGE` 和对应 secrets `DOCKER_REGISTRY_USER`、`DOCKER_REGISTRY_PASSWORD`、`DOCKERHUB_USERNAME`、`DOCKERHUB_TOKEN`。与 agent-hub 相同，该流程不检查重复版本，仓库没有配置不可变规则时已有标签可能被覆盖；生产部署应固定镜像 digest。

2026-11-02 本机容器检查：使用此前构建的测试镜像和临时卷，分别运行并销毁容器；合成的已确认字节与待上传块在新容器中可读取。容器内备份和恢复工具成功运行，恢复库保留上述合成数据。此测试未启动 HTTPS 服务，未验证真实 Opus 解码、删除/撤销交错、跨版本迁移或断电；临时卷已清理。

Docker Hub 文档提供不可变标签设置；华为 SWR 官方文档中的不可变规则适用于企业版。尚未核实目标仓库类型、实际规则和发布凭据；不能假设基础版具备此能力，发布流程与 agent-hub 一致，不提供重复版本保护。
