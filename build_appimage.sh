#!/usr/bin/env bash
set -e

echo "==> Compilando PizuttiCAM em Ubuntu 22.04 (GLIBC 2.35 + Boost 1.85)..."

docker run --rm -v "${HOME}/GERBER_X3:/workspace" -w /workspace ubuntu:22.04 bash -c "
    set -e
    apt-get update && apt-get install -y \
        build-essential ninja-build qt6-base-dev qt6-tools-dev \
        libgl1-mesa-dev libglu1-mesa-dev curl git file gpg wget

    # 1. Instala CMake 4+ atualizado da Kitware
    test -f /usr/share/doc/kitware-archive-keyring/copyright || wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor -o /etc/apt/trusted.gpg.d/kitware.gpg
    echo 'deb https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list
    apt-get update && apt-get install -y cmake

    # 2. Garante a estrutura do Boost 1.85 pré-compilado oficial
    if [ ! -f '/workspace/third_party/boost_1_85_0/CMakeLists.txt' ]; then
        echo '==> Baixando e configurando suporte completo ao Boost 1.85.0...'
        rm -rf /workspace/third_party/boost_1_85_0
        mkdir -p /workspace/third_party
        wget -q https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.gz -O /tmp/boost.tar.gz
        tar -xf /tmp/boost.tar.gz -C /workspace/third_party/
        rm -f /tmp/boost.tar.gz
    fi

    # 3. Limpa e compila do zero com Boost em modo Module/Headers
    rm -rf bin/docker_build
    cmake -S . -B bin/docker_build -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBOOST_ROOT=/workspace/third_party/boost_1_85_0 \
        -DBoost_NO_WARN_NEW_VERSIONS=ON \
        -DCMAKE_DISABLE_FIND_PACKAGE_Boost=OFF

    ninja -C bin/docker_build

    # 4. Prepara a estrutura do AppDir
    mkdir -p AppDir/usr/bin AppDir/usr/lib AppDir/usr/plugins AppDir/usr/share/icons/hicolor/256x256/apps

    cp bin/docker_build/bin/GGEasy AppDir/usr/bin/
    chmod +x AppDir/usr/bin/GGEasy

    # Copia bibliotecas do projeto (.so)
    find bin/docker_build -name '*.so*' -exec cp {} AppDir/usr/lib/ \; 2>/dev/null || true

    # Copia plugins e recursos
    cp -r bin/docker_build/bin/plugins AppDir/usr/bin/ 2>/dev/null || true
    cp -r bin/docker_build/bin/scripts AppDir/usr/bin/ 2>/dev/null || true
    cp -r bin/docker_build/bin/tools AppDir/usr/bin/ 2>/dev/null || true
    cp -r bin/docker_build/bin/settings AppDir/usr/bin/ 2>/dev/null || true

    # Copia bibliotecas e plugins do Qt6
    cp -r /usr/lib/x86_64-linux-gnu/qt6/plugins/* AppDir/usr/plugins/ 2>/dev/null || true
    cp -d /usr/lib/x86_64-linux-gnu/libQt6*.so* AppDir/usr/lib/ 2>/dev/null || true

    # Desktop e Ícones
    cp ggeasy/res/256.png AppDir/pizutticam.png
    cp ggeasy/res/256.png AppDir/.DirIcon

    cat <<EOF > AppDir/pizutticam.desktop
[Desktop Entry]
Type=Application
Name=PizuttiCAM
Exec=GGEasy
Icon=pizutticam
Categories=Utility;Engineering;
Comment=PizuttiCAM PCB Viewer e CAM
Terminal=false
EOF

    # AppRun
    cat << 'EOF_RUN' > AppDir/AppRun
#!/bin/sh
HERE=\"\$(dirname \"\$(readlink -f \"\${0}\")\")\"
export PATH=\"\${HERE}/usr/bin:\${PATH}\"
export LD_LIBRARY_PATH=\"\${HERE}/usr/lib:\${HERE}/usr/bin:\${LD_LIBRARY_PATH}\"
export QT_PLUGIN_PATH=\"\${HERE}/usr/plugins\"
export GGEASY_OPENGL=desktop
cd \"\${HERE}/usr/bin\"
exec \"./GGEasy\" \"\$@\"
EOF_RUN
    chmod +x AppDir/AppRun

    # Baixa appimagetool e gera o pacote final
    curl -sLO https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x appimagetool-x86_64.AppImage
    ./appimagetool-x86_64.AppImage AppDir PizuttiCAM-x86_64.AppImage

    # Limpeza da pasta temporária do AppDir
    rm -rf AppDir appimagetool-x86_64.AppImage
"

echo "=========================================="
echo " Concluído! O AppImage UNIVERSAL foi gerado em:"
echo " ${HOME}/GERBER_X3/PizuttiCAM-x86_64.AppImage"
echo "=========================================="
