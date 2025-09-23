export MACOSX_DEPLOYMENT_TARGET=11.1
apt-get install -y openjdk-8-jdk
omp install openjdk11

apt-get install -y autoconf gperf

export CC=oa64-clang
export CXX=oa64-clang++

rm -rf dist-mac

autoconf

export CFLAGS="-O3"
export CXXFLAGS="-O3"
export CPPFLAGS="-I/libgmp_static/dist/include"
export LIBS="/libgmp_static/dist/lib/libgmp.a"
export LDFLAGS="-L/libgmp_static/dist/lib"

./configure --enable-thread-safety --disable-mcsat --host=arm64-apple-darwin20.2 \
    --prefix=$(realpath dist-mac) \
    --with-pic-gmp=/libgmp_static/dist/lib/libgmp.a \
    --with-pic-gmp-include-dir=/libgmp_static/dist/include

make MODE=release ARCH=arm64-apple-darwin20.2 POSIXOS=darwin show-details clean dist install

llvm-install-name-tool-18 -id libyices.2.dylib dist-mac/lib/libyices.2.dylib

mkdir -p dist-mac/dist
cp dist-mac/lib/libyices.2.dylib dist-mac/dist/

cd java-bindings
INSTALL_DIR=/mnt/dist-mac/dist ./build_macos.sh
cd ..

llvm-install-name-tool-18 -id libyices2java.dylib dist-mac/dist/libyices2java.dylib

llvm-otool-18 -L dist-mac/dist/*
