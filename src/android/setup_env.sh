


export JAVA_HOME=/usr/lib/jvm/default-java
export ANDROID_HOME="$HOME/Android/Sdk"
export ANDROID_SDK="$ANDROID_HOME"
export ANDROID_NDK="$(echo "$ANDROID_SDK/ndk/"*)"
export ANDROID_NDK_TOOLCHAIN_ROOT="$ANDROID_NDK/toolchains"
export ANDROID_PLATFORM=10

export Qt5_host=/usr
export Qt5_android="$(echo "$HOME/Qt/"5.*)/android_armv7"
# export Qt5_android="$(echo "$HOME/Qt/"5.*)/android_x86"

CMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake"
Qt5_DIR="$Qt5_android/lib/cmake/Qt5"

export PATH="$ANDROID_SDK/tools/bin:$Qt5_android/bin:$PATH"

function cmake_android()
{
    cmake \
        -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
        -DQt5_DIR="$Qt5_android/lib/cmake/Qt5/" \
        -DQt5Core_DIR="$Qt5_android/lib/cmake/Qt5Core" \
        -DQt5Widgets_DIR="$Qt5_android/lib/cmake/Qt5Widgets" \
        -DQt5Xml_DIR="$Qt5_android/lib/cmake/Qt5Xml" \
        -DQt5Concurrent_DIR="$Qt5_android/lib/cmake/Qt5Concurrent" \
        -DQt5UiTools_DIR="$Qt5_android/lib/cmake/Qt5UiTools" \
        -DQt5Gui_DIR="$Qt5_android/lib/cmake/Qt5Gui" \
        -DANDROID_STL=c++_shared \
        "$@"
}
