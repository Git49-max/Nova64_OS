#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Nova Language Installer (Beta 1.3 Edition)
# ─────────────────────────────────────────────────────────────────────────────

set -e

BOLD="\033[1m"
CYAN="\033[1;36m"
GREEN="\033[1;32m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
GRAY="\033[0;90m"
RESET="\033[0m"

info()    { echo -e "${CYAN}  →${RESET} $1"; }
success() { echo -e "${GREEN}  ✓${RESET} ${BOLD}$1${RESET}"; }
warn()    { echo -e "${YELLOW}  ⚠${RESET} $1"; }
fail()    { echo -e "${RED}  ✗ error:${RESET} ${BOLD}$1${RESET}"; exit 1; }

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── Dependency Check (LLVM 19) ────────────────────────────────────────────────
info "Checking for LLVM 19..."

if ! command -v llvm-config-19 &>/dev/null && ! llvm-config --version | grep -q "19" 2>/dev/null; then
    warn "LLVM 19 not found. This is required for Nova to compile code."
    echo -ne "${YELLOW}  ?${RESET} Would you like to install llvm-19-dev now? (sudo required) [y/N]: "
    read -r response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
        info "Updating packages and installing LLVM 19..."
        sudo apt update && sudo apt install -y llvm-19-dev clang-19 libncurses-dev
        success "LLVM 19 installed successfully."
    else
        fail "LLVM 19 is required. Please install it manually: sudo apt install llvm-19-dev"
    fi
else
    success "LLVM 19 detected."
fi

# ── Arguments ─────────────────────────────────────────────────────────────────
PREFIX="/usr/local"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --prefix) PREFIX="$2"; shift 2 ;;
        --help)
            echo "Usage: ./install.sh [--prefix /path]"
            echo "  --prefix   Installation base directory (default: /usr/local)"
            exit 0 ;;
        *) fail "unknown argument: $1" ;;
    esac
done

BIN_DIR="$PREFIX/bin"
LIB_DIR="$PREFIX/lib/nova"

echo ""
echo -e "${BOLD}Nova Language Installer${RESET}"
echo -e "${GRAY}  repository: $REPO${RESET}"
echo -e "${GRAY}  binary:     $BIN_DIR/n++${RESET}"
echo -e "${GRAY}  stdlib:     $LIB_DIR/${RESET}"
echo ""

# ── Check package structure ───────────────────────────────────────────────────
# Se o binário não existir, tenta dar um 'make' antes de falhar
if [[ ! -f "$REPO/n++" ]]; then
    warn "Binary 'n++' not found. Trying to build it..."
    if [[ -f "$REPO/Makefile" ]]; then
        make -C "$REPO" || fail "Failed to build n++. Run 'make' manually."
    else
        fail "Neither binary 'n++' nor 'Makefile' found."
    fi
fi
[[ -d "$REPO/stdlib" ]] || fail "'stdlib/' directory not found in package"

# ── Install the binary ────────────────────────────────────────────────────────
info "Installing binary..."

sudo mkdir -p "$BIN_DIR"
sudo cp "$REPO/n++" "$BIN_DIR/n++"
sudo chmod +x "$BIN_DIR/n++"

success "Binary installed at $BIN_DIR/n++"

# ── Install the stdlib ────────────────────────────────────────────────────────
info "Installing standard library..."

sudo mkdir -p "$LIB_DIR"
sudo cp "$REPO/stdlib/"*.nh  "$LIB_DIR/" 2>/dev/null || warn "No .nh files found"
sudo cp "$REPO/stdlib/"*.npp "$LIB_DIR/" 2>/dev/null || warn "No .npp files found"

NH_COUNT=$(ls "$LIB_DIR"/*.nh  2>/dev/null | wc -l)
NPP_COUNT=$(ls "$LIB_DIR"/*.npp 2>/dev/null | wc -l)
success "Standard library installed ($NH_COUNT headers, $NPP_COUNT modules)"

# ── Configure shell RC ────────────────────────────────────────────────────────
info "Configuring environment variables..."

if [ -n "$SUDO_USER" ]; then
    USER_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
else
    USER_HOME="$HOME"
fi

if   [[ -f "$USER_HOME/.zshrc" ]];   then SHELL_RC="$USER_HOME/.zshrc"
elif [[ -f "$USER_HOME/.bashrc" ]];  then SHELL_RC="$USER_HOME/.bashrc"
elif [[ -f "$USER_HOME/.profile" ]]; then SHELL_RC="$USER_HOME/.profile"
else SHELL_RC=""; fi

add_to_rc() {
    local line="$1"
    local comment="$2"
    if [[ -n "$SHELL_RC" ]] && ! grep -qF "$line" "$SHELL_RC"; then
        echo "" >> "$SHELL_RC"
        echo "# $comment" >> "$SHELL_RC"
        echo "$line" >> "$SHELL_RC"
        [ -n "$SUDO_USER" ] && chown "$SUDO_USER" "$SHELL_RC"
    fi
}

if [[ -n "$SHELL_RC" ]]; then
    add_to_rc "export NOVA_STDLIB_PATH=\"$LIB_DIR\""  "Nova standard library path"
    # Apenas adiciona ao PATH se já não estiver lá
    add_to_rc "export PATH=\"\$PATH:$BIN_DIR\""       "Nova compiler"
    success "Environment variables added to $SHELL_RC"
else
    warn "Could not detect shell RC file. Add these manually:"
    echo "    export NOVA_STDLIB_PATH=\"$LIB_DIR\""
    echo "    export PATH=\"\$PATH:$BIN_DIR\""
fi

# ── Final Message ─────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}${GREEN}Installation complete!${RESET}"
echo ""
echo -e "  Try running: ${CYAN}n++ --version${RESET}"
echo -e "  To start:    ${CYAN}source $SHELL_RC${RESET} (or restart terminal)"
echo ""