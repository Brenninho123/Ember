#!/usr/bin/env bash
set -euo pipefail

QEMU_SRC_DIR="$1"
OUTPUT_DIR="$2"
NDK_HOME="$3"

if [ ! -d "$QEMU_SRC_DIR" ]; then
    echo "QEMU source directory not found: $QEMU_SRC_DIR"
    exit 1
fi

if [ ! -d "$NDK_HOME" ]; then
    echo "Android NDK not found: $NDK_HOME"
    exit 1
fi

API_LEVEL=26
HOST_TAG="linux-x86_64"
TOOLCHAIN="$NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG"

declare -A ABI_TARGETS=(
    ["arm64-v8a"]="aarch64-linux-android"
    ["armeabi-v7a"]="armv7a-linux-androideabi"
    ["x86_64"]="x86_64-linux-android"
)

declare -A QEMU_TARGETS=(
    ["arm64-v8a"]="i386-softmmu"
    ["armeabi-v7a"]="i386-softmmu"
    ["x86_64"]="i386-softmmu"
)

mkdir -p "$OUTPUT_DIR"

for ABI in "${!ABI_TARGETS[@]}"; do
    TARGET_TRIPLE="${ABI_TARGETS[$ABI]}"
    QEMU_TARGET="${QEMU_TARGETS[$ABI]}"

    echo "Building QEMU for $ABI ($TARGET_TRIPLE, target=$QEMU_TARGET)"

    BUILD_DIR="build-$ABI"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"

    export CC="$TOOLCHAIN/bin/${TARGET_TRIPLE}${API_LEVEL}-clang"
    export CXX="$TOOLCHAIN/bin/${TARGET_TRIPLE}${API_LEVEL}-clang++"
    export AR="$TOOLCHAIN/bin/llvm-ar"
    export STRIP="$TOOLCHAIN/bin/llvm-strip"
    export RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
    export LD="$TOOLCHAIN/bin/ld"

    if [ ! -f "$CC" ]; then
        echo "Compiler not found for $ABI: $CC"
        exit 1
    fi

    (
        cd "$BUILD_DIR"

        "../$QEMU_SRC_DIR/configure" \
            --target-list="$QEMU_TARGET" \
            --cross-prefix="" \
            --cc="$CC" \
            --cxx="$CXX" \
            --ar="$AR" \
            --strip="$STRIP" \
            --ranlib="$RANLIB" \
            --static \
            --disable-tools \
            --disable-docs \
            --disable-guest-agent \
            --disable-gtk \
            --disable-vnc \
            --disable-sdl \
            --enable-kvm=no \
            --disable-werror

        make -j"$(nproc)"
    )

    BINARY_NAME="qemu-system-i386"
    BUILT_BINARY="$BUILD_DIR/$BINARY_NAME"

    if [ ! -f "$BUILT_BINARY" ]; then
        echo "Build failed, binary not found: $BUILT_BINARY"
        exit 1
    fi

    ABI_OUTPUT_DIR="$OUTPUT_DIR/$ABI"
    mkdir -p "$ABI_OUTPUT_DIR"

    cp "$BUILT_BINARY" "$ABI_OUTPUT_DIR/libqemu_system_i386.so"
    "$STRIP" "$ABI_OUTPUT_DIR/libqemu_system_i386.so"

    echo "Finished $ABI -> $ABI_OUTPUT_DIR/libqemu_system_i386.so"
done

echo "All ABIs built successfully"
