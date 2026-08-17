# QuteNote

qutenote is a simple but joyful note app.

it's built on the qt framework, designed for both desktop and android. 
there are no ads and no hidden features, just notes. 
it's forever free but you can support development with a donation.

## Features

- **Rich Text Editing**: Format your notes with bold, italic, underline, colors, lists, links, and images (Camera support on Android)
- **Image Management**: Resize and align images with an intuitive touch-friendly popup dialog
- **Touch-Friendly**: Built from the ground up for touch devices with large, comfortable buttons
- **Cross-Platform**: Runs on Windows, Linux, macOS, and Android (Qt 6.10+)
- **Theme System**: Choose between Pink and Purple themes with customizable zoom levels (100%/150%/200%)
- **File Browser**: Navigate your notes with an intuitive file tree
- **Auto-Backup**: Automatic backup system to keep your notes safe
- **Offline First**: No cloud, no accounts, just local files you control

## Building

### Requirements

- Qt 6.10 or later
- CMake 3.16 or later
- C++17 compiler
- Ninja (optional but recommended)
- **Android only:** Android SDK + NDK 27.2, JDK 17

### Linux Desktop Build

```bash
cd QuteNote
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.2/gcc_64 -G Ninja
cmake --build . -j$(nproc)
# Binary: build/QuteNote
```

A convenience script `build.sh` is provided in the repository root that
automates configuration and building with Ninja. Edit the `QT_PATH`
variable inside it to match your Qt installation before running.

```bash
./build.sh
```

### Android Build (Linux host)

#### 1. Install Qt Android kit (no sudo needed)

```bash
# Install aqtinstall via pipx
pipx install aqtinstall

# Install Qt 6.10.2 Android arm64-v8a
aqt install-qt linux android 6.10.2 android_arm64_v8a \
  --outputdir /path/to/Qt
```

#### 2. Android SDK & NDK

```bash
# Download cmdline-tools
mkdir -p ~/Android/Sdk/cmdline-tools
wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
unzip commandlinetools-linux-*.zip -d ~/Android/Sdk/cmdline-tools
mv ~/Android/Sdk/cmdline-tools/cmdline-tools ~/Android/Sdk/cmdline-tools/latest

# Install NDK, platform, build-tools
~/Android/Sdk/cmdline-tools/latest/bin/sdkmanager --sdk_root=$HOME/Android/Sdk \
  "ndk;27.2.12479018" "platforms;android-35" "build-tools;35.0.0"
```

#### 3. Configure & build

```bash
cd QuteNote
mkdir -p build/android

/path/to/Qt/6.10.2/android_arm64_v8a/bin/qt-cmake \
  -S . -B build/android -G Ninja \
  -DQT_HOST_PATH=/path/to/Qt/6.10.2/gcc_64 \
  -DANDROID_SDK_ROOT=$HOME/Android/Sdk \
  -DANDROID_NDK_ROOT=$HOME/Android/Sdk/ndk/27.2.12479018 \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-35

cmake --build build/android -j$(nproc)
# APK: build/android/android-build/build/outputs/apk/debug/android-build-debug.apk
```

### Windows Build (CLI)

If Qt Creator is unavailable, build manually using `qt-cmake` and Ninja:

**Desktop:**
```powershell
& "C:\Qt\6.10.1\mingw_64\bin\qt-cmake.bat" -S . -B build/desktop -G Ninja
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/desktop
```

**Android:**
```powershell
& "C:\Qt\6.10.1\android_arm64_v8a\bin\qt-cmake.bat" -S . -B build/android -G Ninja `
  -DCMAKE_MAKE_PROGRAM="C:\Qt\Tools\Ninja\ninja.exe" `
  -DANDROID_SDK_ROOT="C:\Users\<user>\AppData\Local\Android\Sdk" `
  -DANDROID_NDK_ROOT="C:\Users\<user>\AppData\Local\Android\Sdk\ndk\27.2.12479018" `
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-35

& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/android
```
## Architecture

QuteNote is built with a modular architecture:

- **ComponentBase**: Base class for all major UI components with lifecycle management
- **ThemeManager**: Centralized theming system with colors, metrics, and fonts
- **MainView**: Primary view hosting the editor, file browser, settings panel, and toolbar
- **TextEditor**: Rich text editor with formatting toolbar
- **FileBrowser**: Touch-optimized file navigation (left slide-in panel)
- **SettingsView**: Tabbed settings interface (right slide-in panel)
- **Panel System**: Unified dual-panel layout with animated slide transitions and mutual exclusion — opening one panel auto-closes the other

### Theme System

The theme system (v1.0) provides:
- **Colors**: Primary, secondary, background, text, accent, and more
- **Metrics**: Touch targets, icon sizes, spacing, border radius (zoom-controlled)
- **Fonts**: Editor font and UI header font with automatic scaling
- **Android Integration**: Status bar and navigation bar colors sync with theme

The zoom slider affects all UI elements proportionally:
- 100%: 48px touch targets, 24px icons, 12pt UI font
- 150%: 72px touch targets, 36px icons, 18pt UI font
- 200%: 96px touch targets, 48px icons, 24pt UI font

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

The codebase follows Qt 6 conventions with modern C++17.
## License

QuteNote is free software, licensed under the GNU General Public License (GPL) version 3 or later.

This program uses Qt, which is available under GPL v3. You have the right to:
- Use this software for any purpose
- Study how the software works and modify it
- Distribute copies of the software
- Distribute your modified versions

### Source Code

- QuteNote: [https://github.com/sapphicdisaster/QuteNote](https://github.com/sapphicdisaster/QuteNote)
- GPL v3 License: [https://www.gnu.org/licenses/gpl-3.0.html](https://www.gnu.org/licenses/gpl-3.0.html)
- Qt source code: [https://code.qt.io/cgit/qt/qtbase.git/](https://code.qt.io/cgit/qt/qtbase.git/)

## Support

If you enjoy using QuteNote and want to support its development, you can make a donation at:
[https://ko-fi.com/411omen/tip](https://ko-fi.com/411omen/tip)

---

made with 💜 by skye