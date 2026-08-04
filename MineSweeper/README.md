# 扫雷 MineSweeper

使用 C++ 与 Qt 5（Widgets）实现的经典扫雷游戏。

## 功能

- 三种预设难度：初级 9×9/10 雷、中级 16×16/40 雷、高级 30×16/99 雷
- 自定义难度（行数、列数、地雷数）
- 首次点击安全（并排除首格周围 3×3 区域布雷）
- 左键翻开、右键插旗、中键/双击数字快速翻格（chord）
- 计时器与剩余雷数统计，笑脸按钮一键重开
- 失败时显示全部地雷与插错的旗，胜利时自动插旗
- 经典 Windows 扫雷风格（立体凸起/凹陷格子、彩色数字、自绘旗子与地雷）

## 环境

- Windows
- Qt 5.15.2（本机使用 Anaconda 内置的 MSVC 版 Qt）
- Visual Studio（MSVC x64 工具链）或任何支持 Qt 5 的编译器

## 构建

### 方式一：脚本构建（本机已验证）

直接双击或运行：

```bat
build.bat
```

脚本会调用 vcvarsall 初始化 MSVC 环境，再用 qmake + nmake 编译，
并尝试用 windeployqt 把 Qt 运行库部署到 `bin\` 目录。
产物：`bin\MineSweeper.exe`。

### 方式二：qmake 手动构建

```bat
call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
set PATH=D:\Develop\anaconda3\Library\bin;%PATH%
qmake MineSweeper.pro
nmake
```

### 方式三：CMake

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

> 使用自己的 Qt 安装时，请相应修改 `build.bat` 中的 `QTDIR`、`VCVARSALL` 路径，
> 或直接配置 `CMAKE_PREFIX_PATH` 指向 Qt 目录。

## 运行

```bat
run.bat
```

或直接运行 `bin\MineSweeper.exe`（若已执行过 windeployqt，无需配置 PATH）。

## 操作说明

- 左键：翻开格子；首次点击一定安全
- 右键：插旗 / 取消旗
- 中键或左键点击已翻开的数字格：当其周围旗子数等于数字时快速翻开周围格子
- F2：重新开始

## AI 帮解（自动解局）

主界面有一个「AI帮解」按钮和模型选择下拉栏。点击后游戏会把当前棋盘每个格子的
状态（隐藏/插旗/数字）通过本地 HTTP 接口发送给同目录下的 `AIForMineSweeper`
Python 服务，服务按所选引擎推理出下一步（翻开/插旗/快速翻格），游戏自动执行并
回传新状态，循环直到通关。通关后会把用时、步数、尝试次数等记录写入
`AIForMineSweeper/solve_records.txt`。

下拉栏两个选项：

- **API Key 模型（DeepSeek）**：使用你配置的 API Key 调用 DeepSeek（默认）。
- **网页默认模型（内置求解器）**：本地启发式算法，不需要 API Key。

使用步骤：

1. 若使用 DeepSeek，先配置 API Key（二选一）：

   ```bat
   set DEEPSEEK_API_KEY=sk-xxxxxxxx
   ```

   或编辑 `AIForMineSweeper/config.json` 中的 `api_key` 字段。

2. 启动 Python 服务（游戏找不到时会尝试自动启动，也可手动运行）：

   ```bat
   python ..\AIForMineSweeper\server.py
   ```

3. 回到游戏，先在下拉栏选择模型，再点击「AI帮解」。解局过程中每一步都会以
   可见的方式自动执行，踩雷后会自动重开一局继续解，直到通关或手动点击「停止」。

详细说明见 `AIForMineSweeper/README.md`。

## 目录结构

```text
MineSweeper/
├── MineSweeper.pro       # qmake 工程文件
├── CMakeLists.txt        # CMake 工程文件
├── build.bat             # 一键构建脚本
├── run.bat               # 运行脚本
└── src/
    ├── main.cpp
    ├── MainWindow.*      # 主窗口：菜单、计时、雷数、棋盘管理
    ├── MineField.*       # 核心游戏逻辑（不依赖 Qt）
    ├── CellButton.*      # 格子控件：自绘外观与鼠标交互
    └── CustomGameDialog.*# 自定义难度对话框
```
