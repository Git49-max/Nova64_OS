#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# Nova64 Build System
# ─────────────────────────────────────────────────────────────────────────────

VERSION_FILE=".build_no"

# 1. Gerenciar versão
if [ ! -f "$VERSION_FILE" ]; then
    echo 0 > "$VERSION_FILE"
fi
OLD_VERSION=$(cat "$VERSION_FILE")
NEW_VERSION=$((OLD_VERSION + 1))
echo $NEW_VERSION > "$VERSION_FILE"

echo "----------------------------------------"
echo "🛠️  Nova64 Build System | Build #$NEW_VERSION"
echo "----------------------------------------"

# 2. Compilar n++
echo "[1/5] Compiling n++..."
make -s 2>&1 | grep -i "error:"
MAKE_RET=${PIPESTATUS[0]}
if [ $MAKE_RET -ne 0 ]; then
    echo "❌ n++ build failed!"
    exit 1
fi

# 3. Compilar orbit
echo "[2/5] Compiling orbit..."
make -s -C orbit 2>&1 | grep -i "error:"
ORBIT_RET=${PIPESTATUS[0]}
if [ $ORBIT_RET -ne 0 ]; then
    echo "❌ orbit build failed!"
    exit 1
fi

# 4. Instalar ambos
echo "[3/5] Installing binaries..."
./install.sh > /dev/null 2>&1

# 5. Atualizar ambiente
echo "[4/5] Updating shell..."
source ~/.bashrc 2>/dev/null || true

# 6. Validar versões
echo -n "[5/5] n++ version:    "
n++ --version 2>/dev/null || echo "(not in PATH yet — restart terminal)"
echo -n "      orbit version:  "
orbit version 2>/dev/null || echo "(not in PATH yet — restart terminal)"

echo "----------------------------------------"
echo "✅ Build #$NEW_VERSION complete."
