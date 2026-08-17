# Contributing to QuteNote

## C++/Qt6 Syntax and Best Practices

**Always follow these rules to avoid syntax errors:**

- All C++ member functions (including inline getters/setters) must be declared inside the class definition in the header file, not at the top or outside the class.
- Do not place function definitions or statements outside of class/struct/namespace scope in header files.
- Use `public:`, `private:`, and `protected:` access specifiers only inside class/struct definitions.
- Always include necessary Qt headers (e.g., `#include <QString>`, `#include <QWidget>`, etc.) and custom class headers when referencing types from other files.
- When using Qt signals/slots, use the `Q_OBJECT` macro at the top of QObject-derived classes.
- Use Qt6 signal/slot syntax: `connect(sender, &Sender::signal, receiver, &Receiver::slot);`
- When referencing Qt or C++ features, follow Qt 6 and modern C++ (C++17 or later) conventions.
- If unsure, refer to the Qt 6 documentation: https://doc.qt.io/qt-6/

**If you are generating or editing C++/Qt code, always check for:**
- Proper class/struct/namespace scoping
- Correct header guards or `#pragma once`
- No duplicate or misplaced function definitions
- No global function definitions in headers unless intended

**If you are unsure about C++ or Qt syntax, consult the Qt 6 documentation or C++ reference.**

---

## Build Commands

### Linux Desktop

```bash
cd QuteNote
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.10.2/gcc_64 -G Ninja
cmake --build . -j$(nproc)
```

Or use the convenience script in the repository root:

```bash
# Set QT_PATH env var or edit the script
QT_PATH=/path/to/Qt/6.10.2/gcc_64 ./build.sh
```

After initial configuration, rebuild with:

```bash
cmake --build build --parallel $(nproc)
```

The project uses Allman brace style, 4-space indentation, `m_` prefix for member variables, and camelBack for methods.

## Common Extension Points

**Add a new setting:**
1. Add UI in `SettingsView::setupUI()`
2. Update `loadSettings()` and `saveSettings()`

**Add a file operation:**
1. Add action in `MainView::setupToolbar()`
2. Implement handler method
3. Connect signals in `setupConnections()`

**Adding a new theme:** Implement `createXTheme()` in `thememanager.cpp`, set all required color/metric fields.

## Key Files & Directories

- `mainview.cpp`, `mainwindow.cpp`: Main UI logic
- `filebrowser.cpp`: File navigation, touch adaptation
- `settingsview.cpp`: Settings UI and persistence
- `thememanager.cpp`: Global theming system, handles colors, fonts, metrics, and Android system UI
- `themesettingspage.cpp`: Theme customization UI with zoom slider and font controls
- `android/`: Android build/config
- `resources/`: Icons, translations
