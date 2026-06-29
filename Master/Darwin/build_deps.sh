#!/usr/bin/env bash
#
# build_deps.sh — Build DROD's third-party dependencies from source as
# UNIVERSAL (x86_64 + arm64) dylibs targeting an old macOS deployment version,
# into a local prefix (Master/Darwin/deps). This lets the resulting .app run on
# older macOS / Intel Macs, which the Homebrew dylibs (built for the host's
# current macOS) cannot.
#
# Uses real SDL2 (not Homebrew's sdl2-compat shim), so no SDL3 is involved.
# zlib + curl come from the macOS SDK (present and back-compatible on all macOS).
#
# Usage: ./build_deps.sh [lib ...]      (no args = all, in dependency order)
#
set -euo pipefail

DARWIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$DARWIN_DIR/../.." && pwd)"
METAKIT_DIR="$REPO_DIR/metakit"
PREFIX="$DARWIN_DIR/deps"
SRC="$PREFIX/src"
DEPLOY="${MACOSX_DEPLOYMENT_TARGET:-15.0}"
ARCHS="x86_64;arm64"
JOBS="$(sysctl -n hw.ncpu)"

mkdir -p "$SRC"

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

# fetch <url> <dirname> : download+extract into $SRC, echo the extracted path
fetch() {
	local url="$1" dir="$2" dest="$SRC/$2" tarball="$SRC/${2}.tar.gz"
	if [[ ! -d "$dest" ]]; then
		log "Fetching $dir"
		curl -fL --retry 3 -o "$tarball" "$url"
		# Extract into a scratch dir and rename on success, so an interrupted
		# download/extract never leaves a half-populated source tree that later
		# runs would mistake for a complete checkout.
		rm -rf "$dest.partial"
		mkdir -p "$dest.partial"
		tar -xzf "$tarball" -C "$dest.partial" --strip-components=1
		mv "$dest.partial" "$dest"
	fi
	echo "$dest"
}

# cmake_install <srcdir> [extra cmake args...] : configure+build+install universal
cmake_install() {
	local src="$1"; shift
	rm -rf "$src/build-uni"
	cmake -S "$src" -B "$src/build-uni" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY" \
		-DCMAKE_INSTALL_PREFIX="$PREFIX" \
		-DCMAKE_INSTALL_NAME_DIR="@rpath" \
		-DCMAKE_SKIP_INSTALL_RPATH=ON \
		-DCMAKE_PREFIX_PATH="$PREFIX" \
		-DBUILD_SHARED_LIBS=ON \
		"$@"
	cmake --build "$src/build-uni" -j "$JOBS"
	cmake --install "$src/build-uni"
}

# cmake_lipo_install <srcdir> [extra cmake args...] : for libs whose build system
# refuses a single multi-arch pass (e.g. libjpeg-turbo's assembly). Build each
# arch separately, then lipo the dylibs and merge headers/config into $PREFIX.
cmake_lipo_install() {
	local src="$1"; shift
	local a
	for a in x86_64 arm64; do
		rm -rf "$src/build-$a" "$src/inst-$a"
		cmake -S "$src" -B "$src/build-$a" -G Ninja \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_OSX_ARCHITECTURES="$a" \
			-DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY" \
			-DCMAKE_INSTALL_PREFIX="$src/inst-$a" \
			-DCMAKE_INSTALL_NAME_DIR="@rpath" \
		-DCMAKE_SKIP_INSTALL_RPATH=ON \
			-DBUILD_SHARED_LIBS=ON \
			"$@"
		cmake --build "$src/build-$a" -j "$JOBS"
		cmake --install "$src/build-$a"
	done
	# Headers + cmake/pkgconfig metadata are arch-independent: take x86_64's.
	cp -R "$src/inst-x86_64/include/." "$PREFIX/include/" 2>/dev/null || true
	mkdir -p "$PREFIX/lib"
	[[ -d "$src/inst-x86_64/lib/cmake" ]] && cp -R "$src/inst-x86_64/lib/cmake" "$PREFIX/lib/"
	[[ -d "$src/inst-x86_64/lib/pkgconfig" ]] && cp -R "$src/inst-x86_64/lib/pkgconfig" "$PREFIX/lib/"
	# Real dylibs -> lipo; symlinks -> recreate.
	local f
	for f in "$src"/inst-x86_64/lib/*.dylib; do
		[[ -e "$f" ]] || continue
		local base; base="$(basename "$f")"
		if [[ -L "$f" ]]; then
			ln -sf "$(readlink "$f")" "$PREFIX/lib/$base"
		elif [[ -f "$src/inst-arm64/lib/$base" ]]; then
			lipo -create "$f" "$src/inst-arm64/lib/$base" -output "$PREFIX/lib/$base"
		fi
	done
}

have() { [[ -e "$PREFIX/lib/$1" ]]; }

###############################################################################
# Individual dependencies (dependency order)
###############################################################################

dep_png() {
	have libpng16.dylib && { log "libpng present"; return; }
	local s; s=$(fetch https://github.com/pnggroup/libpng/archive/refs/tags/v1.6.44.tar.gz libpng-1.6.44)
	# The Intel SSE / ARM NEON paths are per-arch and leave undefined symbols in a
	# single universal pass. The CMake options aren't enough because pngpriv.h
	# re-enables NEON from __ARM_NEON, so force the opt macros off via compile flags.
	cmake_install "$s" -DPNG_TESTS=OFF -DPNG_SHARED=ON -DPNG_STATIC=OFF -DPNG_EXECUTABLES=OFF \
		-DPNG_HARDWARE_OPTIMIZATIONS=OFF -DPNG_ARM_NEON=off -DPNG_INTEL_SSE=off \
		-DCMAKE_C_FLAGS="-DPNG_ARM_NEON_OPT=0 -DPNG_INTEL_SSE_OPT=0"
}

dep_jpeg() {
	have libjpeg.dylib && { log "libjpeg present"; return; }
	local s; s=$(fetch https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.4/libjpeg-turbo-3.0.4.tar.gz libjpeg-turbo-3.0.4)
	# libjpeg-turbo's CMake refuses a multi-arch pass (assembly), so build per-arch
	# (SIMD on) and lipo.
	cmake_lipo_install "$s" -DENABLE_STATIC=OFF -DENABLE_SHARED=ON -DWITH_TURBOJPEG=OFF
}

dep_freetype() {
	have libfreetype.dylib && { log "freetype present"; return; }
	local s; s=$(fetch https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-3.tar.gz freetype-VER-2-13-3)
	cmake_install "$s" -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_PNG=ON -DFT_DISABLE_BROTLI=ON \
		-DFT_DISABLE_BZIP2=ON -DFT_DISABLE_ZLIB=ON
}

dep_expat() {
	have libexpat.dylib && { log "expat present"; return; }
	local s; s=$(fetch https://github.com/libexpat/libexpat/releases/download/R_2_6_4/expat-2.6.4.tar.gz expat-2.6.4)
	cmake_install "$s" -DEXPAT_BUILD_TESTS=OFF -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TOOLS=OFF \
		-DEXPAT_BUILD_DOCS=OFF -DEXPAT_SHARED_LIBS=ON
}

dep_ogg() {
	have libogg.dylib && { log "libogg present"; return; }
	local s; s=$(fetch https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.tar.gz libogg-1.3.5)
	cmake_install "$s" -DINSTALL_DOCS=OFF
}

dep_vorbis() {
	have libvorbis.dylib && { log "libvorbis present"; return; }
	local s; s=$(fetch https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.gz libvorbis-1.3.7)
	cmake_install "$s"
}

dep_theora() {
	have libtheora.dylib && { log "libtheora present"; return; }
	# theora is autotools-only. Its 2009 config.sub/guess don't know arm64, so
	# autoreconf to refresh them, then build arm64 natively + x86_64 cross (--host,
	# which keeps configure from running target binaries -> no Rosetta needed) and
	# lipo. --disable-asm avoids per-arch assembly; --with-ogg points at our libogg.
	local s; s=$(fetch https://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz libtheora-1.1.1)
	# The release tarball ships a pre-generated configure (avoids autoreconf's macro
	# hell), but its 2009 config.sub/guess predate arm64 -- refresh them.
	curl -fsSL -o "$s/config.sub"   https://git.savannah.gnu.org/cgit/config.git/plain/config.sub
	curl -fsSL -o "$s/config.guess" https://git.savannah.gnu.org/cgit/config.git/plain/config.guess
	chmod +x "$s/config.sub" "$s/config.guess"
	local a host
	for a in x86_64 arm64; do
		[[ $a == x86_64 ]] && host="--host=x86_64-apple-darwin" || host=""
		( cd "$s" && make distclean >/dev/null 2>&1 || true
		  ./configure $host --prefix="$s/inst-$a" --disable-static --enable-shared \
			--disable-examples --disable-spec --disable-oggtest --disable-vorbistest --disable-asm \
			--with-ogg="$PREFIX" \
			CC=clang CFLAGS="-arch $a -mmacosx-version-min=$DEPLOY -O2" \
			LDFLAGS="-arch $a -mmacosx-version-min=$DEPLOY -L$PREFIX/lib" \
		  && make -j"$JOBS" && make install )
	done
	cp -R "$s/inst-x86_64/include/." "$PREFIX/include/" 2>/dev/null || true
	[[ -d "$s/inst-x86_64/lib/pkgconfig" ]] && cp -R "$s/inst-x86_64/lib/pkgconfig" "$PREFIX/lib/"
	local f base
	for f in "$s"/inst-x86_64/lib/*.dylib; do
		[[ -e "$f" ]] || continue
		base="$(basename "$f")"
		if [[ -L "$f" ]]; then
			ln -sf "$(readlink "$f")" "$PREFIX/lib/$base"
		else
			lipo -create "$f" "$s/inst-arm64/lib/$base" -output "$PREFIX/lib/$base"
			install_name_tool -id "@rpath/$base" "$PREFIX/lib/$base"
		fi
	done
}

dep_jsoncpp() {
	have libjsoncpp.dylib && { log "jsoncpp present"; return; }
	local s; s=$(fetch https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.6.tar.gz jsoncpp-1.9.6)
	cmake_install "$s" -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
		-DBUILD_OBJECT_LIBS=OFF -DBUILD_STATIC_LIBS=OFF
}

dep_sdl2() {
	have libSDL2-2.0.0.dylib && { log "SDL2 present"; return; }
	local s; s=$(fetch https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz SDL2-2.30.9)
	cmake_install "$s" -DSDL_TEST=OFF -DSDL_STATIC=OFF -DSDL_SHARED=ON
}

dep_sdl2_ttf() {
	have libSDL2_ttf.dylib && { log "SDL2_ttf present"; return; }
	local s; s=$(fetch https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz SDL2_ttf-2.22.0)
	cmake_install "$s" -DSDL2TTF_VENDORED=OFF -DSDL2TTF_HARFBUZZ=OFF -DSDL2TTF_SAMPLES=OFF
}

dep_sdl2_mixer() {
	have libSDL2_mixer.dylib && { log "SDL2_mixer present"; return; }
	local s; s=$(fetch https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-2.8.0.tar.gz SDL2_mixer-2.8.0)
	# DROD needs Ogg/Vorbis + WAV; keep MP3 (built-in drmp3, no extra dep). Drop the rest.
	cmake_install "$s" -DSDL2MIXER_VENDORED=OFF -DSDL2MIXER_SAMPLES=OFF \
		-DSDL2MIXER_VORBIS=VORBISFILE \
		-DSDL2MIXER_MP3=ON -DSDL2MIXER_MP3_DRMP3=ON -DSDL2MIXER_MP3_MPG123=OFF \
		-DSDL2MIXER_FLAC=OFF -DSDL2MIXER_OPUS=OFF -DSDL2MIXER_MOD=OFF \
		-DSDL2MIXER_MIDI=OFF -DSDL2MIXER_GME=OFF -DSDL2MIXER_WAVPACK=OFF
}

dep_metakit() {
	if [[ -f "$PREFIX/lib/libmk4.a" ]] && lipo -archs "$PREFIX/lib/libmk4.a" | grep -q arm64; then
		log "metakit present"; return
	fi
	[[ -f "$METAKIT_DIR/unix/configure" ]] || ( cd "$REPO_DIR" && git submodule update --init metakit )
	local a bd host
	for a in x86_64 arm64; do
		bd="$METAKIT_DIR/build-uni-$a"
		# Cross-compile the non-native (x86_64) arch: --build != --host puts
		# configure in cross mode so it never runs a target binary (no Rosetta).
		# Also seed ac_cv_sizeof_long (8 on both 64-bit arches) as a safety net.
		# (Assumes an arm64 build host, which is the supported setup.)
		[[ $a == x86_64 ]] && host="--build=arm64-apple-darwin --host=x86_64-apple-darwin" || host=""
		rm -rf "$bd"; mkdir -p "$bd"
		( cd "$bd" && CC=clang CXX=clang++ \
			CFLAGS="-arch $a -mmacosx-version-min=$DEPLOY" \
			CXXFLAGS="-arch $a -std=c++11 -mmacosx-version-min=$DEPLOY -fno-rtti" \
			../unix/configure $host ac_cv_sizeof_long=8 && make -j"$JOBS" libmk4.a && ranlib libmk4.a )
	done
	mkdir -p "$PREFIX/lib"
	lipo -create "$METAKIT_DIR/build-uni-x86_64/libmk4.a" "$METAKIT_DIR/build-uni-arm64/libmk4.a" \
		-output "$PREFIX/lib/libmk4.a"
}

ALL=(metakit png jpeg freetype expat ogg vorbis theora jsoncpp sdl2 sdl2_ttf sdl2_mixer)

main() {
	local libs=("$@"); [[ ${#libs[@]} -eq 0 ]] && libs=("${ALL[@]}")
	command -v cmake >/dev/null || die "cmake not found (brew install cmake ninja)"
	command -v ninja >/dev/null || die "ninja not found (brew install ninja)"
	log "Prefix: $PREFIX   Deployment target: $DEPLOY   Archs: $ARCHS"
	for l in "${libs[@]}"; do "dep_$l"; done
	log "Done. Built into $PREFIX/lib"
}
main "$@"
