# 单机容器部署（待容器验证）

v0.1.0 镜像已发布；生产使用前仍需完成实际部署环境验收。服务是单实例 SQLite 应用，不支持多个副本或共享网络文件系统。生产使用前须完成设备联调、安全审计、证书管理及备份策略。

1. 按 `server/Dockerfile` 的 node 用户 UID/GID（官方镜像通常为 1000，实际应通过 `docker run --rm --entrypoint id IMAGE` 核对）在宿主机准备独占的空数据目录，确保目录所有权与容器用户相同且权限为 0700。目录保存数据库、音频块、WAL、锁和任务；不允许只挂载 SQLite 文件。准备证书目录，其中 `cert.pem` 和 `key.pem` 能被容器用户读取，证书主机名、有效期和信任链须匹配客户端。
2. 在受保护位置创建权限 0600 的 `meeting.env`，提供 `MEETING_ADMIN_TOKEN`、`ZERONE_AGENT_HTTP_API_KEY`（互不相同且至少 32 个字符）及 `STEPFUN_API_KEY`；可选变量见 `server/OPERATIONS.md`。不要把文件或值提交到仓库。Compose 环境变量会被 Docker 管理员访问，运行主机须受信任。
3. 默认使用 Docker Hub 的 `zeroneai/meeting-recorder:v0.1.0`、本目录下的 `data/`、`tls/` 和 `meeting.env`。先创建 `data/` 并按第 1 步设置权限，将证书放入 `tls/`。执行 `docker compose config -q`，再执行 `docker compose up -d`。默认仅监听本机 3443；设备访问时需修改 Compose 的端口绑定，并配置防火墙、证书和首次可信校时。
4. 用受信任 CA 和匹配的主机名访问 `/meeting-service/ui`；管理员状态接口需要令牌。停止用 `docker compose stop`，等待任务排水完成。

升级前执行 [备份与恢复](../server/BACKUP.md)：保全完整数据库和最新安全状态。确认新版兼容矩阵、镜像摘要以及备份可恢复；停止旧实例后升级。仅在证明旧版能读迁移后数据库时允许直接切回旧摘要，否则离线执行恢复工具；缺失最新安全状态或已确认数据时拒绝上线。备份频率、异地复制和灾难恢复目标须由部署方确定，工具本身不提供这些能力。

发布流程与 agent-hub 一致：推送 `v*` Git 标签后运行测试并构建，向配置的 Docker Hub 和 SWR 推送版本标签；稳定版本还更新 `latest`，预发布版本不更新 `latest`。配置变量 `REGISTRY_HOST`、`REGISTRY_IMAGE`、`DOCKERHUB_IMAGE` 和对应 secrets `DOCKER_REGISTRY_USER`、`DOCKER_REGISTRY_PASSWORD`、`DOCKERHUB_USERNAME`、`DOCKERHUB_TOKEN`。与 agent-hub 相同，该流程不检查重复版本，仓库没有配置不可变规则时已有标签可能被覆盖；生产部署应固定镜像 digest。

2026-11-02 本机容器检查：使用此前构建的测试镜像和临时卷，分别运行并销毁容器；合成的已确认字节与待上传块在新容器中可读取。容器内备份和恢复工具成功运行，恢复库保留上述合成数据。此测试未启动 HTTPS 服务，未验证真实 Opus 解码、删除/撤销交错、跨版本迁移或断电；临时卷已清理。

Docker Hub 文档提供不可变标签设置；华为 SWR 官方文档中的不可变规则适用于企业版。尚未核实目标仓库类型、实际规则和发布凭据；不能假设基础版具备此能力，发布流程与 agent-hub 一致，不提供重复版本保护。
