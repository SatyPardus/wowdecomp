# macOS 原生移植笔记

本文记录把 wowdecomp（WoW 3.3.5a 客户端反编译工程）移植为 macOS 原生可执行程序的过程、设计决策、
启动调试中定位到的问题及其根因。

当前状态：Apple Silicon（arm64）原生构建 `bin/Mac/Wow` 可以加载 3.3.5 zhCN 游戏数据并完整显示登录界面
（场景、模型动画、中文字体、粒子），正常退出。登录服务器、进入世界、声音与输入的完整验证尚未进行。

## 1. 架构与依赖替换

| Windows | macOS 替代 |
|---|---|
| Win32 窗口 / WGL | SDL2 窗口 + OpenGL 2.1 legacy 上下文（系统 OpenGL.framework） |
| DirectInput / WM_* 消息 | SDL2 事件 → `OSINPUT` 事件（`Engine/Source/Os/Mac/Input.cpp`） |
| Direct3D 9 / 9Ex | 不可用，macOS 仅使用 OpenGL 设备（ARB vertex/fragment program 路径） |
| WinINet | libcurl |
| Winsock / IOCP | BSD sockets |
| 注册表 | `~/Library/Preferences/WowDecomp/registry.ini` |
| FMOD Ex Win32 输出 | 基于 SDL2 音频的 FMOD 输出插件（`fmodex/mac/src`） |
| CRITICAL_SECTION / Win32 线程 | pthread（见第 4 节的语义对齐） |

约定：

- 平台代码放在与 `Win32/`、`Win64/` 同级的 `Mac/` 目录（FMOD 为 `fmodex/mac/src/`），源文件整体用
  `#if defined(__APPLE__)` 包裹。各模块 CMake 在 macOS 上排除 `Win32/Win64`，在 Windows 上排除 `Mac/`。
- 移植以 Win64 实现为准逐函数对齐语义；仅对 macOS 上确实不存在的功能（Logitech LCD、SteelSeries HID、
  Windows 反作弊内存扫描、补丁下载器、DivX 解码）实现"设备/功能不存在"路径。
- 共享代码中的平台差异一律 `#if` 隔离，Windows 行为保持不变。
- `Engine/Source/Platform/Compat/` 在非 Windows 上是全局 include 目录，存放兼容层：
  `malloc.h`、`SSE.h`（x86 SSE → ARM NEON）、`RecursiveMutex.h`、`ThreadStack.h`。

SDL2 平台约定：

- `main()`（`WoW/Source/Mac/main.cpp`）在 `CommonMain()` 前调用 `SDL_Init`，退出后 `SDL_Quit`。
- 原生窗口句柄（Windows 的 `HWND`）在 macOS 上是 `SDL_Window*`。
- Gx OpenGL 设备创建窗口与 GL 上下文并调用 `OsGuiSetGxWindow`；窗口尺寸变化由设备通过
  `SDL_AddEventWatch` 自行跟踪。
- Os 层在主线程用 `SDL_PollEvent` 泵消息并翻译为与 Win64 `OsWindowProc` 一致的 `OSINPUT` 事件。

## 2. 构建与运行

一键脚本 `./play.sh`（仓库根目录）：

| 选项 | 内容 |
|---|---|
| `1` 配置 | 检查 Xcode 命令行工具、Homebrew；缺少 CMake / SDL2 时询问后用 Homebrew 安装；生成 `build/mac` |
| `2` 编译 | 编译 `Wow` 目标到 `bin/Mac/Wow`（未配置时先执行 1） |
| `3` 启动 | 首次询问客户端目录并把其 `Data` 软链接为 `bin/Mac/Data`，后台启动客户端，输出写入 `bin/Mac/Wow.log`（未编译时先执行 2） |

也可直接 `./play.sh 2`、`./play.sh 3 --client <客户端目录>`。脚本兼容 macOS 自带的 bash 3.2。手动方式：

```sh
brew install cmake sdl2
cmake -S . -B build/mac -DCMAKE_BUILD_TYPE=Release
cmake --build build/mac --target Wow --parallel
```

输出为 `bin/Mac/Wow`。把它放在 3.3.5a 客户端目录（与 `Data/` 同级）运行；数据目录按可执行文件所在位置查找。

## 3. 链接阶段

最初链接 `Wow` 时有 352 个未定义符号，全部来自非 Windows 下被排除的平台实现。按模块并行补齐：
Storm、BlizzardCore、Os、FMOD、Gx（OpenGL/SDL2）、WoW 客户端平台件。补齐后修复的链接问题：

- `CGxDeviceOpenGl::DeviceSetBaseMipLevel` / `IShaderConstantsClear` 被误放进 Windows 专用 `#if` 块，移出。
- `MinimapFrame.cpp` 的 `s_mapId`：原始二进制中与 WorldClient 的 `s_mapId` 是两个不同变量
  （0x00AF4E48 vs 0x00ADFBC4）。MSVC 接受 `extern` 后再 `static` 的重声明，Clang 不接受，
  因此把小地图的文件内变量改名为 `s_minimapMapId`，语义不变。
- Mac `Streaming.cpp` 中的空 `SetZone` 覆盖了 `StreamingManager.cpp` 的正式实现，删除。
- 补充 macOS 版 `StringConversion_CodepageToUTF8::ToNative`（macOS 路径原生即 UTF-8）。

## 4. 用真实数据启动时定位的问题

| 现象 | 根因 | 修复 |
|---|---|---|
| "Failed to find a suitable display device" | Mac 版 `DeviceAdapterInfer` 恒返回失败，`VideoHardware.dbc` 选不到行 | 用临时 OpenGL 上下文按 Windows 版 D3D caps 的同等规则分级（T&L、纹理单元数、像素着色器版本）；Apple Silicon 无 PCI ID，落到通用 0xFFFF 行 |
| 退出时 SIGABRT（`recursive_mutex::lock` 抛异常） | Mac 上锁类型是 `std::recursive_mutex`，`exit()` 析构静态对象时仍有线程在用；Windows 的 `CRITICAL_SECTION` 没有析构 | 新增 `WowRecursiveMutex`（常量初始化、平凡析构的递归 pthread 互斥量），`CRITICAL_SECTION` 与 `Blizzard::Lock::Mutex` 在非 Windows 上都用它；其余静态 `std::mutex` 改为永不析构 |
| `Fonts\FRIZQT__.TTF` 读取失败（`LoadSectorTable failed`） | 块表在原地从 16 字节扩展为 49 字节 `BLOCKENTRY`，条目 0 的 `int64 m_offset` 写入与 `uint32` compressedSize 读取重叠；Clang 按 strict aliasing 重排读写，读到 0。MSVC 不做基于类型的别名分析 | 根 `CMakeLists.txt` 对非 MSVC 编译器全局加 `-fno-strict-aliasing -fwrapv`，与反编译代码依赖的 MSVC 语义一致 |
| FMOD `FMOD_NONBLOCKING` 线程栈溢出（EXC_BAD_ACCESS） | Windows `CreateThread` 的栈大小只是初始提交量，保留量至少为 1 MB；pthread 栈大小是硬上限（macOS 默认 512 KB） | `ThreadStack.h` 的 `WowThreadStackSize` 统一为至少 1 MB，FMOD、BlizzardCore、Storm 三处线程创建均使用 |
| 文字字距过大 | Retina 上全屏切换到 3024×1964，设备格式仍按 1512×982，字体光栅化与布局尺寸不一致 | `CreateGameWindow` 进入全屏后采用实际后备缓冲尺寸（与已有的最大化窗口路径一致） |
| 中文字体每个字符宽度相同（全角宽度） | 字体 `post.isFixedPitch = 1` 但实为比例字体；随仓库附带的 FreeType 2.0 对此类字体强制使用 `advanceWidthMax` | `ThirdParty/freetype/src/truetype/ttgload.c` 改为使用逐字形 `hmtx` 宽度，与原版客户端显示一致 |
| 登录 3D 场景只出现在屏幕中间的小矩形 | 反编译错误：`IXformSetProjection` 用 `int32 >= 0`（第 31 位）判断，原版测的是低字节最高位，即第 7 位 `GxMasterEnable_NormalProjection`；结果固定功能透视投影总被乘以 0.2 的调试缩放 | 改为与两个 D3D 设备相同的位判断 |
| 提示"今后版本将不再支持此系统（SSE）" | Mac 版 CPU 特性位与 Windows 位布局不一致，未报告 SSE（bit `0x4`） | 按 Windows 位定义：arm64 报告 TSC/SSE/SSE2（相关代码均为纯 C++ 或经 NEON 兼容层），x86_64 报告 TSC/MMX/SSE/SSE2 |
| 密码框无法输入 | 焦点离开账号框时 `DisableIME()` 使 IME 计数归零，Mac 版 `OsIMEEnable(0)`/`OsIMESync()` 调用了 `SDL_StopTextInput()`，关闭了全部文字事件；而 Windows 关闭 IME 只解除输入法上下文，`WM_CHAR` 照常到达。密码框（标志 `0x8`）本身不会再开启 IME | IME 开关只管理输入法组合状态，SDL 文字输入在整个会话期间保持开启；`OsInputEnableSecureMode`（仅在密码框获得/失去焦点时成对调用）改为 macOS Secure Event Input（`EnableSecureEventInput`），密码输入绕过输入法并防止其他进程监听按键，对应 Windows 上关闭 IME 的效果 |
| 点登录后"正在连接"时闪退（Network 线程 `EXC_ARM_DA_ALIGN`） | `WowConnectionNet::WORKER` 用 `#pragma pack(push, 1)` 复刻 32 位 `Wow.exe` 布局，`SCritSect m_lock` 位于奇数偏移 0x15。x86 上 MSVC 容忍未对齐的 `CRITICAL_SECTION`，arm64 上 pthread 互斥量必须 8 字节对齐，加锁即总线错误 | 仅 32 位构建（`WOWDECOMP_ARCH_32`，用于对照原版布局）保留 1 字节打包；64 位构建本就与注释偏移不同，改为自然对齐。已扫描全部 `pack(1)` 区域，仅此一处内嵌锁 |

### 非 Mac 特有的问题

以下三项并非 macOS 专属，Windows 构建同样受影响：

- 字体：Windows 构建配合 zhCN/zhTW 数据会出现同样的字距问题。
- 0.2 倍场景：Windows 上使用 OpenGL 渲染器时同样会出现。
- 编译选项：`-fno-strict-aliasing -fwrapv` 对所有非 MSVC 编译器生效。

## 5. 调试方法

- 原版 `Wow.exe`（zhCN 版布局与 enUS 12340 不同，不能直接用反编译注释里的偏移）可用于核对：按指令特征或常量
  搜索后用 `objdump -d` 反汇编。例如 `FT_Load_Glyph` 的标志为 `0x208A`（单色 `+0x38` = `0x20C2`），
  证实反编译代码与原版一致，从而把问题定位到附带的 FreeType。
- 用 Python 独立解析 MPQ（哈希/块表解密、扇区解压）获取真实数据，与引擎读取结果对比。
- x86_64 构建（Rosetta 运行）与 arm64 构建对比，用于区分 ARM 特有的数值问题与共享代码问题。
- 运行中卡住时用 `sample <pid>` 查看主线程栈；崩溃用 `lldb -b -o run -o bt` 抓现场，或解析
  `~/Library/Logs/DiagnosticReports/Wow-*.ips`。
- SDL 会把 SIGTERM 转为退出事件，进程停在模态对话框时 SIGTERM 无效，需要 SIGKILL。

## 6. 已知限制与待办

- 尚未验证：登录服务器、进入世界、声音输出、键鼠/IME 输入（含密码框修复，因无辅助功能权限无法自动注入按键）、
  窗口模式与分辨率切换。
- 临时 x86_64 测试构建（自编译 SDL 2.30）中出现过无文字的 Yes/No 对话框；正式 arm64 构建（Homebrew SDL）未见，
  其对话框文字是否正常显示尚未确认。
- 声音使用 SDL2 音频输出，未接入 OpenAL Soft。
- 流式下载（Streaming）只实现了本地优先的最简路径，完整客户端安装不依赖它。
- macOS 上不可用：Direct3D、Logitech LCD、SteelSeries HID、DivX 过场动画（跳过）、补丁下载启动器。
- 无法在本机编译验证 Windows；共享文件的改动均以 `#if` 隔离，审阅未见 Windows 行为变化。
