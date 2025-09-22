
# Use MSYS2 MINGW64, Python must be installed on the host windows system
export PATH="$PATH:/c/Python312/:/c/Python312/Scripts/"

# JDK must be installed on the host windows system
export JAVA_HOME="/c/jdk-1.8"

pacman -S \
  make \
  git \
  autoconf \
  gperf \
  mingw-w64-x86_64-ccache \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-gmp

DIST_DIR="$(pwd)/dist-win"

rm -rf $DIST_DIR

autoconf

export LDFLAGS="-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lstdc++ -lwinpthread -Wl,-Bdynamic,--no-whole-archive -Wl,--allow-multiple-definition"
export LIBS="/mingw64/lib/libgmp.a"

./configure --enable-thread-safety --disable-mcsat \
    --build=x86_64-pc-mingw64 \
    --prefix=$DIST_DIR \
    --with-pic-gmp=/mingw64/lib/libgmp.a \
    --with-pic-gmp-include-dir=/mingw64/include \
    --with-static-gmp=/mingw64/lib/libgmp.a \
    --with-static-gmp-include-dir=/mingw64/include

LIBS_LINE="LIBS=$LIBS"
sed -i 's#.*LIBS=.*#${LIBS_LINE}#' configs/make.include.x86_64-pc-mingw64

make MODE=release show-details clean dist install

mkdir -p $DIST_DIR/dist
cp $DIST_DIR/bin/libyices.dll $DIST_DIR/dist/

cd java-bindings
YICES_DIST=$DIST_DIR INSTALL_DIR=$DIST_DIR/dist/ ./build_windows.sh
cd ..

ldd $DIST_DIR/dist/*
