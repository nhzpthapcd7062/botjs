# c-robot

通过 Node.js 调用 C++ 原生插件，实现鼠标左键/右键点击、获取坐标、移动鼠标（多端：macOS / Windows / Linux）。

## 前置条件

- Node.js（建议 18+ 或 20+）

### macOS

- 安装 Xcode Command Line Tools：`xcode-select --install`

> 注意：macOS 需要在「系统设置 → 隐私与安全性 → 辅助功能」里给 Terminal / VS Code / 你的 Node 进程授权，否则无法发送鼠标事件。

### Windows

- 需要可用的 C++ 构建环境（例如安装 Visual Studio Build Tools）

### Linux

- 目前实现基于 X11（Wayland 环境可能不可用）
- 需要安装依赖（以 Debian/Ubuntu 为例）：
	- `sudo apt-get install -y libx11-dev libxtst-dev`

开发流程（需求 → 开发 → 测试 → 编译 → 发布）：见 `DEVELOPMENT.md`。

## 安装与构建

```bash
npm i
npm run build
```

## 使用

```js
const robot = require("./index");

// 获取当前鼠标位置
const { x, y } = robot.getMousePos();
console.log(x, y);

// 移动鼠标到指定坐标（全局屏幕坐标）
robot.moveMouse(200, 200);

// 当前鼠标位置左键点击
robot.leftClick();

// 指定坐标右键点击（全局屏幕坐标）
robot.rightClick(200, 200);

// 也支持对象形式
robot.leftClick({ x: 300, y: 400 });
```

也可以直接跑 demo：

```bash
npm run demo
```

## API

- `leftClick()` / `rightClick()`：点击当前鼠标位置
- `leftClick(x, y)` / `rightClick(x, y)`：点击指定坐标
- `leftClick({x, y})` / `rightClick({x, y})`
- `getMousePos()`：返回 `{x, y}`
- `moveMouse(x, y)` / `moveMouse({x, y})`：移动鼠标到指定坐标
