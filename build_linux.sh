YICES_VERSION=2.7.0

apt-get install -y autoconf gperf

autoconf

export CFLAGS="-O3"
export CXXFLAGS="-O3"
export CPPFLAGS="-I/libgmp_static/dist/include"
export LIBS="/libgmp_static/dist/lib/libgmp.a"
export LDFLAGS="-L/libgmp_static/dist/lib"

rm -rf dist-linux

./configure --enable-thread-safety --disable-mcsat \
    --prefix=/mnt/dist-linux \
    --with-pic-gmp=/libgmp_static/dist/lib/libgmp.a \
    --with-pic-gmp-include-dir=/libgmp_static/dist/include

make MODE=release show-details clean dist install

mkdir -p dist-linux/dist
cp dist-linux/lib/libyices.so."$YICES_VERSION" dist-linux/dist/libyices.so

cd java-bindings
INSTALL_DIR=/mnt/dist-linux/dist JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 YICES_DIST=/mnt/dist-linux/ OS="linux" LIB_EXTENSION="so" YICES_VERSION="$YICES_VERSION" ./build_linux.sh
cd ..

find dist-linux/dist -type f -name "*.so" -exec ldd {} +
find dist-linux/dist -type f -name "*.so" -exec du -h {} +
