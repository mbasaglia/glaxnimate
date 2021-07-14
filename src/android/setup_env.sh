QT_VERSION=5.12.11
ANDROID_ABI=x86
ANDROID_ABI_QT=x86
ANDROID_HOME="$HOME/Android/Sdk"
QT_HOME="$HOME/Qt/$QT_VERSION"
ANDROID_PLATFORM=29
Qt5_android="$QT_HOME/android_$ANDROID_ABI_QT"


JAVA_HOME=/usr/lib/jvm/default-java
ANDROID_SDK="$ANDROID_HOME"
ANDROID_NDK="$(echo "$ANDROID_SDK/ndk/"*)"


CMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"

function cmake_android()
{
    set -x
    cmake \
        -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
        -DCMAKE_PREFIX_PATH="$Qt5_android/lib/cmake" \
        -DCMAKE_FIND_ROOT_PATH="$Qt5_android" \
        -DANDROID_STL=c++_shared \
        -DANDROID_ABI="$ANDROID_ABI" \
        -DANDROID_NDK="$ANDROID_NDK" \
        -DANDROID_SDK="$ANDROID_SDK" \
        -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
        -DJAVA_HOME="$JAVA_HOME" \
        "$@"
    set +x
}
