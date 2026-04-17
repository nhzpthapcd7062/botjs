# 开发流程（需求 → 开发 → 测试 → 编译 → 发布）

本文档以本仓库（macOS + Node.js 原生插件 N-API）为例，给出一套可执行的端到端流程。

兼容与打包编译目标（以此为准）：macOS Apple Silicon（arm64）与 Windows x64。

备注：代码中保留了 Linux（X11）实现，但不作为兼容/打包编译目标。

## 1. 需求（Requirement）

- 明确 API 形态：同步/异步、参数格式、返回值
- 明确平台：当前实现仅支持 macOS（CoreGraphics）
- 明确权限：需要“辅助功能”授权（系统设置 → 隐私与安全性 → 辅助功能）

建议把需求写成验收点，例如：

- `getMousePos()` 返回 `{x:number, y:number}`
- `moveMouse(x,y)` 参数不合法时抛错

## 2. 开发（Development）

### 环境准备

- 安装 Xcode Command Line Tools：
  - `xcode-select --install`
- Node.js：建议 18/20/22

（可选）如果你本地要在 Linux 上尝试：需要 X11 相关依赖，但不保证兼容。

### 本地开发命令

- 安装依赖：`npm install`
- 编译插件（Release）：`npm run build`
- 编译插件（Debug）：`npm run build:debug`
- 运行示例：`npm run demo`

核心原生实现：

- C++：`src/addon.cc`
- Node 入口：`index.js`

## 3. 测试（Testing）

### 测试策略

由于真实“移动/点击”会影响用户环境，自动化测试只做：

- 模块能否加载
- 导出函数是否存在
- `getMousePos()` 返回类型正确
- 对 `moveMouse/leftClick/rightClick` 做参数校验测试（只测会抛错的分支，不触发真实动作）

### 运行测试

- `npm test`

测试文件位置：`test/addon.test.js`

## 4. 编译（Build/Compile）

- 统一用 `node-gyp`：`npm run build`
- 产物位置（node-gyp 默认）：`build/Release/crobot.node`

常见问题排查：

- 没有权限：去系统设置开启“辅助功能”
- Xcode 工具链缺失：重新执行 `xcode-select --install`

## 5. 发布版本（Release）

这里给两种常见发布方式：

### A) Git Tag 版本发布（推荐作为起步）

推荐使用一键脚本（不需要额外确认步骤）：

- 发布“当前版本”（仅打 tag，不改版本号）：`npm run release:tag`
- 发布补丁版本：`npm run release:patch`
- 发布次版本：`npm run release:minor`
- 发布主版本：`npm run release:major`

这些命令会：测试 →（可选）升级版本并打 tag → 推送提交/tag（如果配置了 remote）→ `npm pack` 产出 `*.tgz`。

仓库里提供了 GitHub Actions：当推送形如 `v1.2.3` 的 tag 时，会运行构建、打包，并自动创建 GitHub Release 且附带 `*.tgz`（工作流：`.github/workflows/release.yml`）。

### B) 发布到 npm（可选）

如果你希望 `npm publish`：

- 需要把 `package.json` 里的 `private` 改为 `false`
- 然后执行：
  - `npm login`
  - `npm publish`

> 原生插件跨平台/跨 Node 版本分发通常需要预编译产物（prebuild），本仓库当前以“源码发布、使用方本地编译”为主。
