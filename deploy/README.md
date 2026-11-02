# 单机容器部署（待容器验证）

镜像尚未发布；以下步骤需先完成 CI、镜像构建、容器重建和恢复验收。服务是单实例 SQLite 应用，不支持多个副本或共享网络文件系统。生产使用前须完成设备联调、安全审计、证书管理及备份策略。

1. 按 `server/Dockerfile` 的 node 用户 UID/GID（官方镜像通常为 1000，实际应通过 `docker run --rm --entrypoint id IMAGE` 核对）在宿主机准备独占的空数据目录，确保目录所有权与容器用户相同且权限为 0700。目录保存数据库、音频块、WAL、锁和任务；不允许只挂载 SQLite 文件。准备证书目录，其中 `cert.pem` 和 `key.pem` 能被容器用户读取，证书主机名、有效期和信任链须匹配客户端。
2. 在受保护位置创建权限 0600 的 `meeting.env`，提供 `MEETING_ADMIN_TOKEN`、`ZERONE_AGENT_HTTP_API_KEY`（互不相同且至少 32 个字符）及 `STEPFUN_API_KEY`；可选变量见 `server/OPERATIONS.md`。不要把文件或值提交到仓库。Compose 环境变量会被 Docker 管理员访问，运行主机须受信任。
3. 在本目录设置 `MEETING_IMAGE` 为 `ghcr.io/OWNER/meeting-recorder@sha256:...`，`MEETING_DATA_DIR`、`MEETING_TLS_DIR` 和 `MEETING_ENV_FILE` 为绝对路径。执行 `docker compose config -q`，再执行 `docker compose up -d`。默认仅监听本机 3443；设备访问需明确配置 `MEETING_BIND`、防火墙、证书和首次可信校时。
4. 用受信任 CA 和匹配的主机名访问 `/meeting-service/ui`；管理员状态接口需要令牌。停止用 `docker compose stop`，等待任务排水完成。

升级前执行 [备份与恢复](../server/BACKUP.md)：保全完整数据库和最新安全状态。确认新版兼容矩阵、镜像摘要以及备份可恢复；停止旧实例后升级。仅在证明旧版能读迁移后数据库时允许直接切回旧摘要，否则离线执行恢复工具；缺失最新安全状态或已确认数据时拒绝上线。备份频率、异地复制和灾难恢复目标须由部署方确定，工具本身不提供这些能力。

发布工作流只在手动选择版本 Git 标签时执行测试、构建并发布，摘要记录在 Actions 运行记录。标签保护、GHCR 写权限与版本标签不可变性须由仓库管理员配置并验证；工作流当前在推送前主动失败：GHCR 的版本标签不可变性和可区分不存在与认证/网络错误的检查尚未验证。必须落实这些保障后才可启用发布。部署固定镜像 digest，不使用可变标签。
