YICES_VERSION="2.6.4"

export CC=oa64-clang
export CXX=oa64-clang++

export GMP_ROOT=/libgmp_static/dist

JAVAC="/usr/bin/javac"
JAVA_HOME="/usr/local/osxcross/macports/pkgs/opt/local/Library/Java/JavaVirtualMachines/openjdk11/Contents/Home"
CPPFLAGS="-I $JAVA_HOME/include -I $JAVA_HOME/include/darwin -I $(realpath ../dist-mac/include) -I$GMP_ROOT/include"
CXXFLAGS="-fpermissive -g -fPIC -O3 -stdlib=libc++"
export LIBS="$GMP_ROOT/lib/libgmp.a $(realpath ../dist-mac/lib/libyices.2.dylib)"
export LDFLAGS="-L$GMP_ROOT/lib -L$(realpath ../dist-mac/lib/)"

YICES_2_JAVA_LIB_NAME="libyices2java.dylib"

cd yices2_java_bindings

rm -rf build-mac
mkdir build-mac
cd build-mac

cp ../src/main/java/com/sri/yices/yicesJNI.cpp .

$JAVAC -d . -h . ../src/main/java/com/sri/yices/*.java

$CXX $LD_STATIC_FLAGS $CPPFLAGS $CXXFLAGS -c yicesJNI.cpp

$CXX $LD_STATIC_FLAGS $LDFLAGS -s -shared -o $YICES_2_JAVA_LIB_NAME yicesJNI.o $LIBS

cp $YICES_2_JAVA_LIB_NAME $INSTALL_DIR
