#!/bin/bash
# wowdecomp macOS 一键脚本：1 配置 / 2 编译 / 3 启动
#
# 用法：
#   ./play.sh                       显示菜单
#   ./play.sh 1|2|3                 直接执行对应步骤
#   ./play.sh 3 --client <路径>      启动并指定 3.3.5a 客户端目录（或其 Data 目录）
#
# 每一步会自动补齐前置步骤：编译时若未配置会先配置，启动时若未编译会先编译。

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$REPO_ROOT/build/mac"
OUTPUT_DIR="$REPO_ROOT/bin/Mac"
EXECUTABLE="$OUTPUT_DIR/Wow"
LOG_FILE="$OUTPUT_DIR/Wow.log"
CLIENT_PATH=""

die() {
    echo "错误：$*" >&2
    exit 1
}

info() {
    echo "==> $*"
}

# 询问是/否，默认“是”；非交互环境下返回“否”。
confirm() {
    local answer
    if [ ! -t 0 ]; then
        return 1
    fi
    read -r -p "$1 [Y/n] " answer
    case "$answer" in
        [nN]*) return 1 ;;
        *) return 0 ;;
    esac
}

usage() {
    sed -n '2,9p' "$0" | sed 's/^# \{0,1\}//'
}

brew_prefix() {
    brew --prefix 2>/dev/null
}

check_tools() {
    [ "$(uname -s)" = "Darwin" ] || die "此脚本仅用于 macOS。"

    if ! xcode-select -p >/dev/null 2>&1; then
        info "未找到 Xcode 命令行工具，正在打开安装程序……"
        xcode-select --install || true
        die "请在弹出的窗口中完成安装后重新运行本脚本。"
    fi

    if ! command -v brew >/dev/null 2>&1; then
        die "未找到 Homebrew。请先安装：https://brew.sh
  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    fi

    if ! command -v cmake >/dev/null 2>&1; then
        if confirm "未找到 CMake，是否通过 Homebrew 安装？"; then
            brew install cmake
        else
            die "需要 CMake 才能继续。"
        fi
    fi

    if [ ! -f "$(brew_prefix)/lib/cmake/SDL2/SDL2Config.cmake" ]; then
        if confirm "未找到 SDL2，是否通过 Homebrew 安装？"; then
            brew install sdl2
        else
            die "需要 SDL2 才能继续。"
        fi
    fi

    info "依赖就绪：$(cmake --version | head -n 1)，SDL2 位于 $(brew_prefix)"
}

step_configure() {
    check_tools
    info "生成构建（$BUILD_DIR）"
    cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew_prefix)"
    info "配置完成。"
}

step_build() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        step_configure
    fi
    info "编译客户端（首次编译需要几分钟）"
    cmake --build "$BUILD_DIR" --target Wow --parallel "$(sysctl -n hw.ncpu)"
    [ -x "$EXECUTABLE" ] || die "编译结束但未找到 $EXECUTABLE。"
    info "编译完成：$EXECUTABLE"
}

link_game_data() {
    local data_link="$OUTPUT_DIR/Data"
    local source

    if [ -L "$data_link" ] && [ ! -e "$data_link" ]; then
        info "原有的游戏数据链接已失效（$(readlink "$data_link")），将重新选择。"
        rm "$data_link"
    fi

    if [ -e "$data_link" ]; then
        if [ -L "$data_link" ]; then
            info "使用游戏数据：$(readlink "$data_link")"
        fi
        return
    fi

    source="$CLIENT_PATH"
    if [ -z "$source" ]; then
        [ -t 0 ] || die "未链接游戏数据。请用 --client <路径> 指定 3.3.5a 客户端目录。"
        echo "仓库不包含游戏数据，请使用你自己的 WoW 3.3.5a (12340) 客户端。"
        read -r -p "客户端目录或其 Data 目录的路径（可直接把文件夹拖进来，留空取消）：" source
    fi

    # 去掉首尾空白、引号和拖放产生的转义空格，并展开 ~。
    source="$(printf '%s' "$source" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e "s/^[\"']//" -e "s/[\"']\$//" -e 's/\\ / /g')"
    [ -n "$source" ] || die "未选择游戏数据，客户端未启动。"
    case "$source" in
        "~"*) source="$HOME${source#\~}" ;;
    esac

    if [ -d "$source/Data" ]; then
        source="$source/Data"
    fi
    [ -d "$source" ] || die "游戏数据目录不存在：$source"
    source="$(cd "$source" && pwd)"

    if [ -z "$(find "$source" -maxdepth 2 -iname '*.mpq' -print -quit)" ]; then
        die "$source 中没有 MPQ 文件，请选择 WoW 3.3.5a 的 Data 目录。"
    fi

    mkdir -p "$OUTPUT_DIR"
    # 符号链接，避免复制数 GB 的 Data 目录。
    ln -s "$source" "$data_link"
    info "已链接游戏数据：$data_link -> $source"
}

step_run() {
    local pid

    if [ ! -x "$EXECUTABLE" ]; then
        step_build
    fi
    link_game_data

    info "使用 Data/<语言>/realmlist.wtf 中已有的服务器设置（脚本不会修改）。"
    cd "$OUTPUT_DIR"
    nohup "$EXECUTABLE" >"$LOG_FILE" 2>&1 &
    pid=$!
    sleep 3
    if ! kill -0 "$pid" 2>/dev/null; then
        tail -n 20 "$LOG_FILE" >&2 || true
        die "客户端启动后立即退出，详见 $LOG_FILE"
    fi
    info "客户端已启动（PID $pid），输出记录在 $LOG_FILE"
}

run_choice() {
    case "$1" in
        1 | setup) step_configure ;;
        2 | build) step_build ;;
        3 | run) step_run ;;
        *) die "未知选项：$1（可选 1、2、3）" ;;
    esac
}

CHOICE=""
while [ $# -gt 0 ]; do
    case "$1" in
        -h | --help)
            usage
            exit 0
            ;;
        --client)
            [ $# -ge 2 ] || die "--client 需要一个路径。"
            CLIENT_PATH="$2"
            shift 2
            ;;
        *)
            [ -z "$CHOICE" ] || die "只能指定一个步骤。"
            CHOICE="$1"
            shift
            ;;
    esac
done

if [ -z "$CHOICE" ]; then
    echo "wowdecomp macOS"
    echo "  1) 配置：检查/安装依赖（Xcode 命令行工具、Homebrew、CMake、SDL2）并生成构建"
    echo "  2) 编译：编译客户端 bin/Mac/Wow"
    echo "  3) 启动：链接游戏数据并启动客户端（需要时自动编译）"
    echo "  q) 退出"
    read -r -p "请选择 [1/2/3/q]：" CHOICE
    case "$CHOICE" in
        q | Q | "") exit 0 ;;
    esac
fi

run_choice "$CHOICE"
