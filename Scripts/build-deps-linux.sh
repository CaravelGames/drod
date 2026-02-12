#!/bin/bash

set -e

FORCE_REBUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--force)
            FORCE_REBUILD=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -f, --force    Force rebuild all libraries even if they exist"
            echo "  -h, --help     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

SCRIPT_DIR="$(pwd)"
DROD_ROOT_DIR="$SCRIPT_DIR/../Master/Linux"
DEPS_DIR="$DROD_ROOT_DIR/deps"
INSTALL_DIR="$DROD_ROOT_DIR/static-libs"

RPG_ROOT_DIR="$SCRIPT_DIR/../drodrpg/Master/Linux"
RPG_DEPS_DIR="$RPG_ROOT_DIR/deps"
RPG_INSTALL_DIR="$RPG_ROOT_DIR/static-libs"

mkdir -p "$DEPS_DIR"
mkdir -p "$INSTALL_DIR"

cd "$DEPS_DIR"

# Function to download and extract
download_extract() {
    local url="$1"
    local filename="$2"
    local dirname="$3"

    if [ ! -f "$filename" ]; then
        echo "Downloading $filename..."
        wget "$url" -O "$filename"
    fi

    if [ ! -d "$dirname" ]; then
        echo "Extracting $filename..."
        case "$filename" in
            *.tar.gz|*.tgz) tar -xzf "$filename" ;;
            *.tar.bz2) tar -xjf "$filename" ;;
            *.tar.xz) tar -xJf "$filename" ;;
            *.zip) unzip "$filename" ;;
        esac
    fi
}

# Download sources
echo "=== Downloading dependencies ==="

# SDL2 libraries
download_extract "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz" \
    "SDL2-2.32.10.tar.gz" "SDL2-2.32.10"

download_extract "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz" \
    "SDL2_mixer-2.6.3.tar.gz" "SDL2_mixer-2.6.3"

download_extract "https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-2.20.2.tar.gz" \
    "SDL2_ttf-2.20.2.tar.gz" "SDL2_ttf-2.20.2"

# Audio/Video libraries
download_extract "https://ftp.osuosl.org/pub/xiph/releases/theora/libtheora-1.1.1.tar.bz2" \
    "libtheora-1.1.1.tar.bz2" "libtheora-1.1.1"

download_extract "https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-1.3.7.tar.xz" \
    "libvorbis-1.3.7.tar.xz" "libvorbis-1.3.7"

download_extract "https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-1.3.5.tar.xz" \
    "libogg-1.3.5.tar.xz" "libogg-1.3.5"

# Graphics libraries
download_extract "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.1/libjpeg-turbo-3.0.1.tar.gz" \
    "libjpeg-turbo-3.0.1.tar.gz" "libjpeg-turbo-3.0.1"

download_extract "https://download.savannah.gnu.org/releases/freetype/freetype-2.13.0.tar.xz" \
    "freetype-2.13.0.tar.xz" "freetype-2.13.0"

download_extract "https://download.sourceforge.net/libpng/libpng-1.6.39.tar.xz" \
    "libpng-1.6.39.tar.xz" "libpng-1.6.39"

# Other libraries
download_extract "https://curl.se/download/curl-7.88.1.tar.gz" \
    "curl-7.88.1.tar.gz" "curl-7.88.1"

download_extract "https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.xz" \
    "expat-2.5.0.tar.xz" "expat-2.5.0"

# OpenSSL for static linking
download_extract "https://www.openssl.org/source/openssl-3.0.8.tar.gz" \
    "openssl-3.0.8.tar.gz" "openssl-3.0.8"

# JSON library
download_extract "https://github.com/open-source-parsers/jsoncpp/archive/1.9.5.tar.gz" \
    "jsoncpp-1.9.5.tar.gz" "jsoncpp-1.9.5"

# Check if a library exists
library_exists() {
    local lib_name="$1"
    [ -f "$INSTALL_DIR/lib/lib${lib_name}.a" ] || [ -f "$INSTALL_DIR/lib64/lib${lib_name}.a" ]
}

# Copy lib64 libraries to lib for consistency
copy_lib64_to_lib() {
    if [ -d "$INSTALL_DIR/lib64" ]; then
        echo "Copying lib64 libraries to lib directory..."
        cp -f "$INSTALL_DIR/lib64"/*.a "$INSTALL_DIR/lib/" 2>/dev/null || true
    fi
}

# Build function
build_library() {
    local src_dir="$1"
    local config_opts="$2"
    local make_opts="$3"
    local lib_name="$4"

    if ! $FORCE_REBUILD && library_exists "$lib_name"; then
        echo "=== Skipping $src_dir (lib${lib_name}.a already exists) ==="
        return 0
    fi

    echo "=== Building $src_dir ==="
    cd "$DEPS_DIR/$src_dir"

    # Clean up any previous failed builds
    if [ -f "Makefile" ]; then
        make clean 2>/dev/null || true
    fi

    if [ ! -f "Makefile" ] && [ ! -f "configure" ]; then
        if [ -f "autogen.sh" ]; then
            ./autogen.sh
        elif [ -f "bootstrap" ]; then
            ./bootstrap
        fi
    fi

    if [ -f "Configure" ]; then
        # OpenSSL uses Configure (capital C) instead of configure
        ./Configure --prefix="$INSTALL_DIR" linux-x86_64 $config_opts
        make -j$(nproc) $make_opts
        make install
    elif [ -f "configure" ]; then
        ./configure --prefix="$INSTALL_DIR" --enable-static --disable-shared $config_opts
        make -j$(nproc) $make_opts
        make install
    elif [ -f "CMakeLists.txt" ]; then
        mkdir -p build && cd build
        cmake .. -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS=OFF -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 $config_opts
        make -j$(nproc) $make_opts
        make install
        # Some CMake projects don't install libraries correctly, copy manually if needed
        if [ -f "lib${lib_name}.a" ] && [ ! -f "$INSTALL_DIR/lib/lib${lib_name}.a" ]; then
            cp "lib${lib_name}.a" "$INSTALL_DIR/lib/"
        fi
        cd ..
    else
        make -j$(nproc) $make_opts
        make install
    fi

    # Verify the library was built successfully
    if ! library_exists "$lib_name"; then
        echo "ERROR: Failed to build lib${lib_name}.a"
        return 1
    fi

    echo "✓ Successfully built lib${lib_name}.a"
}

# Build order matters due to dependencies
build_library "libogg-1.3.5" "" "" "ogg"
build_library "libvorbis-1.3.7" "--with-ogg=$INSTALL_DIR" "" "vorbis"
build_library "libtheora-1.1.1" "--with-ogg=$INSTALL_DIR --with-vorbis=$INSTALL_DIR --disable-examples" "" "theora"
build_library "libjpeg-turbo-3.0.1" "" "" "jpeg"
build_library "libpng-1.6.39" "" "" "png16"
build_library "freetype-2.13.0" "" "" "freetype"
build_library "expat-2.5.0" "" "" "expat"

# Build OpenSSL first (needed by curl)
build_library "openssl-3.0.8" "no-shared no-dso" "" "ssl"
copy_lib64_to_lib

build_library "curl-7.88.1" "--disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-manual --without-librtmp --without-libidn2 --without-libpsl --without-zstd --without-brotli --enable-static --with-openssl=$INSTALL_DIR" "" "curl"

# Build jsoncpp
build_library "jsoncpp-1.9.5" "-DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DJSONCPP_WITH_TESTS=OFF" "" "jsoncpp"

# Build SDL2 first (required by SDL2_mixer and SDL2_ttf)
build_library "SDL2-2.32.10" "" "" "SDL2"

build_library "SDL2_mixer-2.6.3" "--with-ogg=$INSTALL_DIR --with-vorbis=$INSTALL_DIR --with-sdl-prefix=$INSTALL_DIR" "" "SDL2_mixer"
build_library "SDL2_ttf-2.20.2" "--with-freetype-prefix=$INSTALL_DIR --with-sdl-prefix=$INSTALL_DIR" "" "SDL2_ttf"

# Handle metakit separately (it's in a different location)
if ! $FORCE_REBUILD && library_exists "mk4"; then
    echo "=== Skipping metakit (libmk4.a already exists) ==="
else
    echo "=== Building metakit ==="
    METAKIT_DIR="$SCRIPT_DIR/../../metakit/unix"
    cd "$METAKIT_DIR"

    # Clean up any previous builds
    if [ -f "Makefile" ]; then
        make clean 2>/dev/null || true
    fi

    if [ -f "configure" ]; then
        ./configure --prefix="$INSTALL_DIR" --enable-static --disable-shared
        make
        make install
    else
        echo "ERROR: metakit configure script not found"
        exit 1
    fi

    # Verify metakit was built successfully
    if ! library_exists "mk4"; then
        echo "ERROR: Failed to build libmk4.a"
        exit 1
    else
        echo "✓ Successfully built libmk4.a"
    fi
fi

echo "=== Copying libraries over to DROD RPG ==="

mkdir -p "$RPG_DEPS_DIR"
mkdir -p "$RPG_INSTALL_DIR"

rsync -a "$DEPS_DIR" "$(dirname "$RPG_DEPS_DIR")"
rsync -a "$INSTALL_DIR" "$(dirname "$RPG_INSTALL_DIR")"

echo "=== Build complete ==="
echo "Static libraries installed in: $INSTALL_DIR"
echo "Include headers in: $INSTALL_DIR/include"
echo "Library files in: $INSTALL_DIR/lib"
