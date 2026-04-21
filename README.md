# botjs

通过 Node.js 调用 C++ 原生插件，实现鼠标和键盘自动化。

## 支持平台

当前只支持以下目标：

- macOS Apple Silicon（arm64）
- Windows x64

## 前置条件

- Node.js 18+ 或 20+
- macOS：安装 Xcode Command Line Tools，执行 `xcode-select --install`
- macOS：在“系统设置 → 隐私与安全性 → 辅助功能”里给 Terminal、VS Code 或你的 Node 进程授权
- Windows：安装可用的 C++ 构建环境，例如 Visual Studio Build Tools

开发流程（需求 → 开发 → 测试 → 编译 → 发布）见 [DEVELOPMENT.md](DEVELOPMENT.md)。

## 安装与构建

```bash
npm i
npm run build
```

## 快速开始

```js
const robot = require("./index");

const { x, y } = robot.getMousePos();
console.log(x, y);

robot.moveMouse(200, 200);
robot.leftClick();
robot.leftDoubleClick();
robot.rightClick(200, 200);
robot.leftClick({ x: 300, y: 400 });

robot.keyDown("a");
robot.keyUp("a");
robot.keyTap("a", { meta: true });
robot.keyTap("c", { ctrl: true });
```

也可以直接运行示例：

```bash
npm run demo
```

## API

### 鼠标

- `getMousePos()`：返回当前鼠标位置 `{ x, y }`
- `moveMouse(x, y)` / `moveMouse({ x, y })`：移动鼠标到指定坐标
- `leftClick()` / `leftClick(x, y)` / `leftClick({ x, y })`：左键单击
- `leftDoubleClick()` / `leftDoubleClick(x, y)` / `leftDoubleClick({ x, y })`：左键双击
- `rightClick()` / `rightClick(x, y)` / `rightClick({ x, y })`：右键单击

### 键盘

- `keyDown(key)`：按下键
- `keyUp(key)`：抬起键
- `keyTap(key, [modifiers])`：点击键，可附带修饰键

### 参数说明

- `key`：支持字符串键名或数字，数字表示平台原生键码，不建议跨平台复用
- `modifiers`：`{ shift?: boolean, ctrl?: boolean, alt?: boolean, meta?: boolean }`

### 常用键名

- 字母：`a`-`z`
- 数字：`0`-`9`
- 功能键：`enter`/`return`、`tab`、`space`、`backspace`、`esc`/`escape`
- 方向键：`left`、`right`、`up`、`down`

## 发布

自动发布流程已接入 GitHub Actions。需要发新版本时，直接执行：

```bash
npm run release:patch
```

如果要发布次版本或主版本，可使用：

```bash
npm run release:minor
npm run release:major
```
