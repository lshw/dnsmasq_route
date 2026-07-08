# 交接文档

## 我们在做什么

这个仓库是一个仅支持 OpenWrt 24+ 的 `dnsmasq_route` 插件，作用是：

- 读取 `/etc/dnsmasq_route.ini` 里的域名列表
- 把这些域名交给指定 DNS 服务器解析
- 把解析出来的 IPv4 地址加入 `nftset`
- 对命中这些地址的流量打 `mark`
- 让这些流量走单独的策略路由表

这次会话的核心任务有 3 件：

1. 保持 `nftset` 条目固定 `24h` 过期，避免集合无限增长
2. 修复 LuCI 页面改配置后服务没有真正重启的问题
3. 不再支持 `iptables/ipset`，代码收敛为只支持 OpenWrt 24+ 的 `nftables/nftset`

## 已经完成了什么

### 1. `nftset` 固定 24 小时过期

已经完成，并且之前已提交：

- `59ca101` `nftset 固定 24 小时过期`

实际状态：

- 文件：[/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route](/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route:1)
- `setup_nftset_rules()` 创建的 set 带：
  - `flags timeout;`
  - `timeout 24h;`
- 过期时间是固定值，不是 UCI 配置项，也没有 LuCI 输入框

已经确认过的结论：

- 同一个 IP 不会重复插入成多条元素
- 同一个 IP 在未过期前再次解析到，剩余过期时间通常不会自动刷新
- 条目过期后再被解析到，会重新加入，并重新获得 `24h` 生命周期

### 2. 修复 LuCI 修改后未重启服务

已经完成，并且之前已提交：

- `547239d` `修复 LuCI 修改后未重启服务`

实际状态：

- 文件：[/home/liushiwei/dnsmasq_route/files/luci/model/dnsmasq_route.lua](/home/liushiwei/dnsmasq_route/files/luci/model/dnsmasq_route.lua:1)
- 新增统一的 `restart_service()`
- 在 `Map.on_after_commit()` 里统一执行：
  - `/etc/init.d/dnsmasq_route restart >/dev/null`

这样做的原因：

- 不能依赖某个字段自己的 `write()` 回调来重启服务
- `Flag` 类型字段，尤其 `localnet_only` 从 `1` 改到 `0` 时，不保证走原先的 `write()` 路径

当前预期行为：

- 只要这个页面提交 UCI 成功，就会统一 `restart`
- `localnet_only`、`dns_server`、`remote_ip` 和域名列表修改都应该立即生效

### 3. 移除 `iptables/ipset` 支持，只保留 `nftables/nftset`

这次会话已经完成，并已提交：

- `e01efc2` `移除 iptables 和 ipset 支持`

实际改动：

- 文件：[/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route](/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route:1)
  - 删除 `ipset` 和 `iptables` 相关分支
  - 删除后端自动探测逻辑
  - 启动时只生成 `nftset=` 配置
  - 增加最小运行时检查：没有 `nft` 就直接报仅支持 OpenWrt 24+
  - 把可缺省的 `uci get` 改成了 `uci -q get`，避免 `uci: Entry not found`
- 文件：[/home/liushiwei/dnsmasq_route/Makefile](/home/liushiwei/dnsmasq_route/Makefile:1)
  - `DEPENDS` 改为 `+luci-compat +nftables +dnsmasq-full`
  - 描述改成 `use dnsmasq nftset, auto set route.`
  - `PKG_RELEASE` 从 `4` 升到 `5`

### 4. 本机已做的基本检查

已经做过：

- `sh -n etc/init.d/dnsmasq_route`

结果：

- 语法检查通过

没有做过：

- 真实 OpenWrt 24+ 设备上的安装测试
- 真实 LuCI 页面回归测试
- 实机路由/NFT 规则验证

## 当前卡在哪里

现在不再卡在代码实现上，代码修改已经完成，当前主要缺的是运行环境验证。

还没做的关键验证是：

- 没有在真实 OpenWrt 24+ 设备上安装这个包
- 没有验证 LuCI 页面改配置后服务是否真的重启并重新生效
- 没有确认 `localnet_only=0` 时生成的 nft 规则是否确实去掉源地址限制
- 没有确认 `remote_ip` 在真实网络环境中能否作为合法下一跳
- 没有确认 `dnsmasq-full` 在目标固件里确实支持当前使用的 `nftset=` 写法

也就是说，当前不是“代码没写完”，而是“设备侧没有验”。

## 下一步应该做什么

优先顺序建议如下：

1. 在目标 OpenWrt 24+ 设备上安装当前包
2. 确认设备上实际具备：
   - `nft`
   - `dnsmasq-full`
   - LuCI 依赖正常
3. 进入 LuCI 页面，逐项测试：
   - `localnet_only: 1 -> 0`
   - `localnet_only: 0 -> 1`
   - 修改 `dns_server`
   - 修改 `remote_ip`
   - 修改域名列表内容
4. 每次修改后检查是否真的重启并生效：
   - `logread | grep dnsmasq_route`
   - `ps | grep dnsmasq_route`
   - `nft list table ip dnsmasq_route`
   - `ip rule`
   - `ip route show table 107`
5. 检查 `/tmp/dnsmasq.d/dnsmasq_nftset.conf` 内容是否符合预期
6. 确认 `nftset` 条目确实带 `timeout 24h`
7. 如果用户要发版，再补：
   - `README` 或使用说明
   - changelog 或版本说明

## 绝对不要再踩的坑

### 1. 不要再恢复 `iptables/ipset` 双后端逻辑

这次方向已经明确了：

- 只支持 OpenWrt 24+
- 只支持 `nftables/nftset`

除非用户再次明确要求，否则不要把 `ipset`、`iptables -m set`、后端自动探测这些逻辑加回来。

### 2. 不要依赖 LuCI 某个字段自己的 `write()` 回调来重启服务

这是之前踩过的最关键的坑。

- `Flag` 字段取消勾选时，不保证走你以为会走的回调
- 尤其 `localnet_only: 1 -> 0`，最容易出现“页面改了，运行态没改”

正确做法：

- 统一放在 `Map.on_after_commit()` 里重启服务

### 3. 不要把 `nftset` 过期时间重新做成配置项

用户之前已经明确要求：

- 不需要设置项
- 固定 `24h`

这个入口之前短暂做成过可配置，后来已经撤掉。不要再恢复，除非用户再次明确要求。

### 4. 不要误判“重复解析会刷新 timeout”

这点已经查过资料，不要再凭经验说错。

- 重复 `add` 已存在元素，通常不会刷新剩余过期时间
- 元素过期删除后再次加入，才会重新拿到完整生命周期

如果新会话还要解释这个问题，应该按 `nftables set element timeout` 的官方语义来回答，不要靠记忆。

### 5. 不要继续使用普通 `uci get` 读取可缺省字段

之前会出现这种噪音：

- `uci: Entry not found`

原因不是功能坏了，而是字段本来允许缺省，比如 `table`。  
对这类字段应该用：

- `uci -q get ...`

### 6. 不要把 `remote_ip` 当成“任意远端地址”

脚本这里最终会执行策略路由：

- `ip route replace table "$table" default via "$remote_ip" [dev ...]`

所以 `remote_ip` 必须在真实环境里能作为合法下一跳网关。  
如果它不是网关，很容易报：

- `Error: Nexthop has invalid gateway.`

这不是 nft 的问题，是路由配置值本身不成立。

### 7. 提交前注意 `.git/index.lock`

之前遇到过一次瞬时残留：

- `git commit` 报 `.git/index.lock` 已存在

如果再遇到：

- 先 `ls -l .git/index.lock`
- 再确认有没有活跃 `git` 进程
- 不要第一反应就做破坏性操作

## 现在仓库的重要状态

最近关键提交：

- `e01efc2` 移除 iptables 和 ipset 支持
- `547239d` 修复 LuCI 修改后未重启服务
- `59ca101` nftset 固定 24 小时过期
- `7cce3ca` Clean up stale policy rules by priority
- `4d82b42` Add mark-based routing for ipset and nftset
- `dca6323` Auto-select dnsmasq ipset or nftset backend

当前工作区状态：

- 这次交接时，主要代码改动已经提交
- 仓库方向已经切换为仅支持 OpenWrt 24+ 的 `nftables/nftset`

## 给完全没有上下文的新会话的最短接手说明

如果你是一个完全没有上下文的新会话，先做这几步：

1. 读启动脚本：[/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route](/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route:1)
2. 读 LuCI 模型：[/home/liushiwei/dnsmasq_route/files/luci/model/dnsmasq_route.lua](/home/liushiwei/dnsmasq_route/files/luci/model/dnsmasq_route.lua:1)
3. 看这 3 个提交：
   - `git show e01efc2`
   - `git show 547239d`
   - `git show 59ca101`
4. 明确当前最需要做的是“真实 OpenWrt 24+ 设备验证”，不是继续猜代码逻辑
5. 如果设备上再次出现报错，优先区分是：
   - `nft/dnsmasq-full` 能力缺失
   - `remote_ip` 不是合法网关
   - LuCI 提交后服务没有真正重启
