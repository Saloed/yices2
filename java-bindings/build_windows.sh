echo "Found java: $JAVA_HOME"
echo "Found yices: $YICES_DIST"

OS=win32
LIB_EXTENSION=dll

JAVAC="$JAVA_HOME/bin/javac"
CXX="g++"

echo "Java compiler $JAVAC"
echo "C++ compiler $CXX"

CPPFLAGS="-I $JAVA_HOME/include -I $JAVA_HOME/include/$OS -I $YICES_DIST/include -I/mingw64/include"
CXXFLAGS="-fpermissive -g -fPIC -O3 -m64"

# note: libgcc is gcc specific option. Remove if not compiling with gcc
LD_STATIC_FLAGS="-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lstdc++ -lwinpthread -Wl,-Bdynamic,--no-whole-archive -Wl,--allow-multiple-definition"
LIBS="$YICES_DIST/lib/libyices.dll.a /mingw64/lib/libgmp.a"
LD_FLAGS="-L $YICES_DIST/lib -L/mingw64/lib/"

YICES_2_JAVA_LIB_NAME="libyices2java.$LIB_EXTENSION"

cd yices2_java_bindings

rm -rf build-win
mkdir build-win
cd build-win

cp ../src/main/java/com/sri/yices/yicesJNI.cpp .

$JAVAC -d . -h . ../src/main/java/com/sri/yices/*.java

sed -i 's|.*#include <jni.h>.*|#include <jni.h>\n#define jint int32_t\n|' com_sri_yices_Yices.h

$CXX $LD_STATIC_FLAGS $CPPFLAGS $CXXFLAGS -c yicesJNI.cpp

$CXX $LD_STATIC_FLAGS $LD_FLAGS -s -shared -o $YICES_2_JAVA_LIB_NAME yicesJNI.o $LIBS

cp $YICES_2_JAVA_LIB_NAME $INSTALL_DIR/
