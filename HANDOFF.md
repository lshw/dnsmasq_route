# 交接文档

## 项目在做什么

这个仓库是一个 OpenWrt 24+ `dnsmasq_route` 插件，作用是：

- 根据 `/etc/dnsmasq_route.ini` 里的域名列表，把这些域名交给指定 DNS 服务器解析。
- 把解析出来的 IPv4 地址加入 `nftset`。
- 对命中这些地址的流量打 mark，并走单独的策略路由表。

当前主要在处理两件事：

- 给 `nftset` 条目增加固定过期时间，避免集合无限增长。
- 修复 LuCI Web 页面修改配置后，没有正确重启/重载 `dnsmasq_route`，导致配置看起来改了但运行态没生效。
- 清理旧的 `iptables/ipset` 兼容代码，只保留 `nftables/nftset` 实现。

## 已经完成了什么

### 1. `nftset` 固定 24 小时过期

已经提交：

- 提交 `59ca101` `nftset 固定 24 小时过期`

实际改动：

- 文件 [etc/init.d/dnsmasq_route](/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route:8)
- `setup_nftset_rules()` 创建 nft set 时，已经改为：
  - `flags timeout;`
  - `timeout 24h;`
- 这个过期时间是固定值，不再提供 UCI 配置项，也不再暴露 LuCI 输入框。
- 代码里已经补了中文注释，明确说明：
  - `nftset` 条目固定保留 24 小时
  - 重复解析不会刷新剩余过期时间

关键结论：

- 同一个 IP 不会重复插入成多条。
- 同一个 IP 在条目未过期前再次解析到，剩余过期时间通常不会自动刷新。
- 条目过期删除后，再次解析到同一 IP，会重新加入，并重新拿到 24 小时生命周期。

### 2. 修复 LuCI 修改配置后未重启服务

已经提交：

- 提交 `547239d` `修复 LuCI 修改后未重启服务`

实际改动：

- 文件 [files/luci/model/dnsmasq_route.lua](/home/liushiwei/dnsmasq_route/files/luci/model/dnsmasq_route.lua:1)
- 新增统一的 `restart_service()`
- 在 `Map.on_after_commit()` 里统一执行：
  - `/etc/init.d/dnsmasq_route restart >/dev/null`

修复原因：

- 原来只有个别字段在 `write()` 路径里才会执行 `start`
- `Flag` 类型字段，尤其 `localnet_only` 从 `1` 改到 `0` 时，不一定会走原来的 `write()` 分支
- 结果就是：
  - Web 页面显示已经关闭
  - 但运行中的 `dnsmasq_route` 规则仍然还是旧的

现在的行为：

- 只要这个页面的 UCI 配置提交成功，就统一 `restart`
- `localnet_only`、`dns_server`、`remote_ip` 等 web 修改都应该立即生效

### 3. 本机已经补了 Lua 语法检查环境

为了后续检查 LuCI Lua 文件，已经在当前机器上安装：

- `lua 5.4.7`
- `luac 5.4.7`

可直接使用：

```bash
luac -p files/luci/model/dnsmasq_route.lua
```

## 当前卡在哪里

代码层面目前**没有卡住的未完成修改**。

真正还没做的是：

- 没有在真实 OpenWrt 设备上回归验证 LuCI 页面修改后的运行效果
- 没有现场确认 `localnet_only=0` 时，最终生成的 nft 规则是否确实去掉源地址限制
- 没有做打包安装测试

也就是说，现在不是“代码卡住”，而是“缺运行环境验证”。

## 下一步应该做什么

优先顺序建议如下：

1. 在目标 OpenWrt 设备上安装当前包，进入 LuCI 页面测试以下场景：
   - `localnet_only: 1 -> 0`
   - `localnet_only: 0 -> 1`
   - 修改 `dns_server`
   - 修改 `remote_ip`
   - 修改域名列表内容
2. 每次修改后检查服务是否真的重启并重新生效：
   - `logread | grep dnsmasq_route`
   - `ps | grep dnsmasq_route`
   - `nft list table ip dnsmasq_route`
3. 确认 `nftset` 条目确实带 `timeout 24h`
4. 如果用户后面要求发版，再补：
   - `readme.MD` 说明
   - 版本号或 changelog

## 绝对不要再踩的坑

### 1. 不要以为 LuCI `Flag.write()` 一定会在取消勾选时触发

这是这次最关键的坑。

- `localnet_only` 从 `1` 改成 `0` 时，不能依赖字段自己的 `write()` 回调来做服务重启
- 正确做法是放到 `Map.on_after_commit()` 这种“提交后统一处理”的位置

### 2. 不要把 `nftset` 过期时间继续做成可配置项

用户已经明确要求：

- 不需要设置项
- 固定为 24 小时即可

之前短暂做过 UCI/LuCI 可配置，后来已经撤掉。不要再把这个入口加回来，除非用户再次明确要求。

### 3. 不要误判“重复解析会刷新过期时间”

这点已经查过资料，不要再按经验乱说。

- 同一个 key 不会生成重复元素
- 再次 `add` 已存在元素，通常不会刷新其 timeout
- 只有特定 `update` 语义才会刷新 timeout

如果新会话还要回答这个问题，先沿着 `nftables element timeout` / `update sets from packet path` 的官方资料回答，不要凭记忆。

### 4. 不要用 `start` 代替统一的 `restart`

LuCI Web 配置修改后，目标是“重新应用配置”，不是“尽量启动一下”。

- 当前实现已经改成 `restart`
- 如果以后继续改 LuCI 行为，优先保持这个语义

### 5. 提交前注意 `.git/index.lock`

之前遇到过一次：

- `git commit` 报 `.git/index.lock` 已存在
- 后来检查时锁文件又消失了，像是瞬时残留

如果再遇到：

- 先检查锁文件是否真的还在
- 不要第一反应就做破坏性操作
- 先 `ls -l .git/index.lock`
- 再确认是否有活跃 git 进程

## 现在仓库的重要状态

最近相关提交：

- `547239d` 修复 LuCI 修改后未重启服务
- `59ca101` nftset 固定 24 小时过期
- `7cce3ca` Clean up stale policy rules by priority
- `4d82b42` Add mark-based routing for ipset and nftset
- `dca6323` Auto-select dnsmasq ipset or nftset backend

当前工作区状态：

- 已切换为仅支持 OpenWrt 24+ 的 `nftables/nftset`

## 给新会话的最短接手说明

如果你是一个完全没有上下文的新会话，先做这几步：

1. 读 [etc/init.d/dnsmasq_route](/home/liushiwei/dnsmasq_route/etc/init.d/dnsmasq_route:1)
2. 读 [files/luci/model/dnsmasq_route.lua](/home/liushiwei/dnsmasq_route/files/luci/model/dnsmasq_route.lua:1)
3. 看最近两个提交：
   - `git show 59ca101`
   - `git show 547239d`
4. 明确现在最需要的是“设备侧验证”，不是继续猜代码逻辑
