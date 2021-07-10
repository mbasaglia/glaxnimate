export QT_VERSION=5.12.11
export ANDROID_ABI=x86_64
export ANDROID_ABI_QT=x86
export ANDROID_HOME="$HOME/Android/Sdk"
export QT_HOME="$HOME/Qt/$QT_VERSION"
export ANDROID_NDK="$HOME/Android/Ndk"


export JAVA_HOME=/usr/lib/jvm/default-java
export ANDROID_SDK="$ANDROID_HOME"
# export ANDROID_NDK="$(echo "$ANDROID_SDK/ndk/"*)"
export ANDROID_NDK_TOOLCHAIN_ROOT="$ANDROID_NDK/toolchains"


export Qt5_host=/usr
export Qt5_android="$QT_HOME/android_$ANDROID_ABI_QT"

CMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"

export PATH="$ANDROID_SDK/tools/bin:$Qt5_android/bin:$PATH"

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
        "$@"
    set +x
}
