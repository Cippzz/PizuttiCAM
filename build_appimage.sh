#!/usr/bin/env bash
set -e

# --- Configuração de Caminhos ---
BUILD_DIR="${HOME}/GERBER_X3/bin/Qt6._GNU_Release_x64/bin"
APPDIR_OUTPUT="${HOME}/AppDir_Build"
APPDIR="${APPDIR_OUTPUT}/AppDir"
FINAL_APPIMAGE="${APPDIR_OUTPUT}/PizuttiCAM-x86_64.AppImage"

echo "==> [1/5] Preparando estrutura do AppDir..."
rm -rf "${APPDIR_OUTPUT}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/plugins"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"

echo "==> [2/5] Copiando executável, plugins do Qt6 e recursos do PizuttiCAM..."
# Executável principal
cp "${BUILD_DIR}/GGEasy" "${APPDIR}/usr/bin/"
chmod +x "${APPDIR}/usr/bin/GGEasy"

# Plugins e scripts próprios
cp -r "${BUILD_DIR}/plugins" "${APPDIR}/usr/bin/" 2>/dev/null || true
cp -r "${BUILD_DIR}/scripts" "${APPDIR}/usr/bin/" 2>/dev/null || true
cp -r "${BUILD_DIR}/tools" "${APPDIR}/usr/bin/" 2>/dev/null || true
cp -r "${BUILD_DIR}/settings" "${APPDIR}/usr/bin/" 2>/dev/null || true

# Plugins de exibição do Qt 6 do Arch Linux (X11 / Wayland)
cp -r /usr/lib/qt6/plugins/platforms "${APPDIR}/usr/plugins/"

# Ícone e Desktop File
cp "${HOME}/GERBER_X3/ggeasy/res/256.png" "${APPDIR}/pizutticam.png"
cp "${HOME}/GERBER_X3/ggeasy/res/256.png" "${APPDIR}/.DirIcon"

cat <<EOF > "${APPDIR}/pizutticam.desktop"
[Desktop Entry]
Type=Application
Name=PizuttiCAM
Exec=GGEasy
Icon=pizutticam
Categories=Utility;Engineering;
Comment=PizuttiCAM PCB Viewer e CAM
Terminal=false
EOF

echo "==> [3/5] Criando o lançador AppRun com aceleração OpenGL..."
cat << 'EOF' > "${APPDIR}/AppRun"
#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"

export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/bin:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins:${HERE}/usr/bin/plugins"

# Forçar aceleração gráfica por hardware na GPU (AMD / Mesa)
export GGEASY_OPENGL=desktop

# Mudar para a pasta dos binários para carregar recursos relativos
cd "${HERE}/usr/bin"

exec "./GGEasy" "$@"
EOF

chmod +x "${APPDIR}/AppRun"

echo "==> [4/5] Baixando e executando appimagetool..."
cd "${APPDIR_OUTPUT}"
if [ ! -f "appimagetool-x86_64.AppImage" ]; then
    curl -sLO https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x appimagetool-x86_64.AppImage
fi

./appimagetool-x86_64.AppImage "${APPDIR}" "${FINAL_APPIMAGE}"

echo "==> [5/5] Garantindo diretório de dados persistentes do usuário..."
mkdir -p "${HOME}/.config/GGEasy/tools"
mkdir -p "${HOME}/.config/PizuttiCAM/tools"

echo "=========================================="
echo " Concluído! AppImage gerado com sucesso em:"
echo " ${FINAL_APPIMAGE}"
echo "=========================================="
