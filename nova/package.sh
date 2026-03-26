#!/usr/bin/env bash

NAME="nova-lang-v2.0.1beta"
DIST_DIR="dist/$NAME"

echo "📦 Packaging Nova..."

# 1. Limpa builds antigos
rm -rf dist
mkdir -p "$DIST_DIR"

# 2. Copia os arquivos essenciais
cp n++ "$DIST_DIR/"
cp install.sh "$DIST_DIR/"
cp -r stdlib "$DIST_DIR/"

# 3. Garante permissões de execução
chmod +x "$DIST_DIR/n++"
chmod +x "$DIST_DIR/install.sh"

# 4. Comprime
cd dist
tar -czvf "${NAME}.tar.gz" "$NAME"

echo "✅ Done! Package created at: dist/${NAME}.tar.gz"