#!/bin/bash
# wowdecomp macOS one-click script: 1 configure / 2 build / 3 launch
#
# Usage:
#   ./play.sh                       Show the menu
#   ./play.sh 1|2|3                 Run the given step directly
#   ./play.sh 3 --client <path>     Launch using a 3.3.5a client folder (or its Data folder)
#
# Each step runs its prerequisites when needed: build configures first, launch builds first.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$REPO_ROOT/build/mac"
OUTPUT_DIR="$REPO_ROOT/bin/Mac"
EXECUTABLE="$OUTPUT_DIR/Wow"
LOG_FILE="$OUTPUT_DIR/Wow.log"
CLIENT_PATH=""

die() {
    echo "Error: $*" >&2
    exit 1
}

info() {
    echo "==> $*"
}

# Ask yes/no, defaulting to yes; returns no when not running interactively.
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
    [ "$(uname -s)" = "Darwin" ] || die "This script is for macOS only."

    if ! xcode-select -p >/dev/null 2>&1; then
        info "Xcode Command Line Tools not found; opening the installer..."
        xcode-select --install || true
        die "Finish the installation in the window that opened, then run this script again."
    fi

    if ! command -v brew >/dev/null 2>&1; then
        die "Homebrew not found. Install it first: https://brew.sh
  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    fi

    if ! command -v cmake >/dev/null 2>&1; then
        if confirm "CMake not found. Install it with Homebrew?"; then
            brew install cmake
        else
            die "CMake is required to continue."
        fi
    fi

    if [ ! -f "$(brew_prefix)/lib/cmake/SDL2/SDL2Config.cmake" ]; then
        if confirm "SDL2 not found. Install it with Homebrew?"; then
            brew install sdl2
        else
            die "SDL2 is required to continue."
        fi
    fi

    # Optional: Xvid decodes the login-screen intro movies; without it they are skipped.
    if [ ! -f "$(brew_prefix)/include/xvid.h" ]; then
        if confirm "xvid (needed for the intro movies) not found. Install it with Homebrew?"; then
            brew install xvid
        else
            info "Skipping xvid: intro movies will not play."
        fi
    fi

    info "Dependencies ready: $(cmake --version | head -n 1), SDL2 in $(brew_prefix)"
}

step_configure() {
    check_tools
    info "Generating build ($BUILD_DIR)"
    cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew_prefix)"
    info "Configuration complete."
}

step_build() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        step_configure
    fi
    info "Building the client (the first build takes a few minutes)"
    cmake --build "$BUILD_DIR" --target Wow --parallel "$(sysctl -n hw.ncpu)"
    [ -x "$EXECUTABLE" ] || die "Build finished but $EXECUTABLE was not found."
    info "Build complete: $EXECUTABLE"
}

link_game_data() {
    local data_link="$OUTPUT_DIR/Data"
    local source

    if [ -L "$data_link" ] && [ ! -e "$data_link" ]; then
        info "The existing game data link is broken ($(readlink "$data_link")); choosing again."
        rm "$data_link"
    fi

    if [ -e "$data_link" ]; then
        if [ -L "$data_link" ]; then
            info "Using game data: $(readlink "$data_link")"
        fi
        return
    fi

    source="$CLIENT_PATH"
    if [ -z "$source" ]; then
        [ -t 0 ] || die "No game data linked. Use --client <path> to specify a 3.3.5a client folder."
        echo "This repository does not include game data; use your own WoW 3.3.5a (12340) client."
        read -r -p "Path to the client folder or its Data folder (you can drag the folder here; leave empty to cancel): " source
    fi

    # Strip surrounding whitespace, quotes and drag-and-drop escaped spaces, and expand ~.
    source="$(printf '%s' "$source" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e "s/^[\"']//" -e "s/[\"']\$//" -e 's/\\ / /g')"
    [ -n "$source" ] || die "No game data selected; the client was not started."
    case "$source" in
        "~"*) source="$HOME${source#\~}" ;;
    esac

    if [ -d "$source/Data" ]; then
        source="$source/Data"
    fi
    [ -d "$source" ] || die "Game data folder does not exist: $source"
    source="$(cd "$source" && pwd)"

    if [ -z "$(find "$source" -maxdepth 2 -iname '*.mpq' -print -quit)" ]; then
        die "No MPQ files in $source. Choose the Data folder of a WoW 3.3.5a client."
    fi

    mkdir -p "$OUTPUT_DIR"
    # Symlink instead of copying the multi-gigabyte Data folder.
    ln -s "$source" "$data_link"
    info "Linked game data: $data_link -> $source"
}

step_run() {
    local pid

    if [ ! -x "$EXECUTABLE" ]; then
        step_build
    fi
    link_game_data

    info "Using the server set in Data/<locale>/realmlist.wtf (this script does not change it)."
    cd "$OUTPUT_DIR"
    nohup "$EXECUTABLE" >"$LOG_FILE" 2>&1 &
    pid=$!
    sleep 3
    if ! kill -0 "$pid" 2>/dev/null; then
        tail -n 20 "$LOG_FILE" >&2 || true
        die "The client exited right after starting; see $LOG_FILE"
    fi
    info "Client started (PID $pid); output is logged to $LOG_FILE"
}

run_choice() {
    case "$1" in
        1 | setup) step_configure ;;
        2 | build) step_build ;;
        3 | run) step_run ;;
        *) die "Unknown option: $1 (choose 1, 2 or 3)" ;;
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
            [ $# -ge 2 ] || die "--client requires a path."
            CLIENT_PATH="$2"
            shift 2
            ;;
        *)
            [ -z "$CHOICE" ] || die "Only one step can be specified."
            CHOICE="$1"
            shift
            ;;
    esac
done

if [ -z "$CHOICE" ]; then
    echo "wowdecomp macOS"
    echo "  1) Configure: check/install dependencies (Xcode Command Line Tools, Homebrew, CMake, SDL2, xvid) and generate the build"
    echo "  2) Build: compile the client bin/Mac/Wow"
    echo "  3) Launch: link game data and start the client (builds first if needed)"
    echo "  q) Quit"
    read -r -p "Choose [1/2/3/q]: " CHOICE
    case "$CHOICE" in
        q | Q | "") exit 0 ;;
    esac
fi

run_choice "$CHOICE"
