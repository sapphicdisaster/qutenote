# QuteNote

qutenote is a simple but joyful note app.

it's built on the qt framework, designed for both desktop and android. 
there are no ads and no hidden features, just notes. 
it's forever free but you can support development with a donation.

## Features

- **Rich Text Editing**: Format your notes with bold, italic, underline, colors, lists, links, and images (Camera support on Android)
- **Image Management**: Resize and align images with an intuitive touch-friendly popup dialog
- **Touch-Friendly**: Built from the ground up for touch devices with large, comfortable buttons
- **Cross-Platform**: built for Android (Qt 6.10+)
- **Theme System**: Choose between Pink and Purple themes with customizable zoom levels (100%/150%/200%)
- **File Browser**: Navigate your notes with an intuitive file tree
- **Auto-Backup**: Automatic backup system to keep your notes safe
- **Offline First**: No cloud, no accounts, just local files you control

## Building

### Requirements

- Qt 6.10 or later
- CMake 3.16 or later
- C++17 compiler

### Desktop Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Linux Build

On Ubuntu/Debian, install the required Qt 6 dependencies first:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
```

Then build as usual:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```


### Manual Windows Build (CLI)

If Qt Creator is unavailable or you prefer not to use it, you can install your desired kits with the maintenance tool, then you can build manually using `qt-cmake` and Ninja.

1. **Configure**:
   Adjust paths for your specific user and Qt version (e.g., `6.10.1`).

   ```powershell
   & "C:\Qt\6.10.1\android_arm64_v8a\bin\qt-cmake.bat" -S . -B build/Qt_6_10_1_for_Android_arm64_v8a-Debug -G Ninja -DCMAKE_MAKE_PROGRAM="C:\Qt\Tools\Ninja\ninja.exe" -DANDROID_SDK_ROOT="C:\Users\skye\AppData\Local\Android\Sdk" -DANDROID_NDK_ROOT="C:\Users\skye\AppData\Local\Android\Sdk\ndk\27.2.12479018" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-35
   ```

2. **Build**:
   ```powershell
   & "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build/Qt_6_10_1_for_Android_arm64_v8a-Debug --target all
   ```
## Architecture

QuteNote is built with a modular architecture:

- **ComponentBase**: Base class for all major UI components with lifecycle management
- **ThemeManager**: Centralized theming system with colors, metrics, and fonts
- **MainView**: Primary view hosting the editor, file browser, and toolbars
- **TextEditor**: Rich text editor with formatting toolbar
- **FileBrowser**: Touch-optimized file navigation
- **SettingsView**: Tabbed settings interface

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