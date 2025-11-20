# QuteNote 1.0 Roadmap

**CRITICAL NOTE (Android Input):** Based on recent findings, custom `TouchHandler` implementations (like `TextEditorTouchHandler`) can interfere with native Android input methods. The back button handling has been refactored to use `keyReleaseEvent` instead of `keyPressEvent` to improve the interaction with the Android system. A thorough investigation is required to identify and, if necessary, disable or adapt other custom `*TouchHandler` classes (e.g., `FileBrowserTouchHandler`, `SettingsViewTouchHandler`, `ColorPickerTouchHandler`) for Android to ensure native behavior.

This roadmap outlines the key steps to reach feature parity and a polished 1.0 release for QuteNote. It addresses current pain points and sets out actionable milestones.

## 1. File Browser Improvements
- [x] Refactor drag-and-drop logic for reliability and intuitive UX (fixed deprecated pointer usage)
- [x] Fixed segfault when deleting divider items
- [x] Customized delete confirmation dialog for divider items
- [x] Refined file and folder styling (icon left, name right, no extra lines)
- [x] Adjusted file and folder item height to match buttons
- [ ] Improve QTreeWidget performance and responsiveness
- [ ] Polish icons, context menus, and touch interactions
- [ ] Ensure file operations (create, rename, delete) are robust and error-tolerant

## 2. Android Integration
- [x] Fixed blank/overlapping tabs and layout issues due to unsupported Qt stylesheet properties
- [x] Fix keyboard focus loss on Enter key in text editor (Note: This is a known bug in Qt 6.9.3 and cannot be fixed within the application)
- [x] Correct text selection handles (offset, visibility, and usability)
- [x] Enable reliable text selection and clipboard operations
- [ ] Test and optimize for common Android devices (screen sizes, input methods)
- [ ] Address any platform-specific bugs in file browser and settings

## 3. Settings & Theme Management
- [x] Design and implement layouts for all settings pages (backup, license, theme, etc.)
- [x] Fixed About tab layout and stacking issues
- [x] Added custom QSlider theme using app colors
- [x] Removed global QWidget stylesheet overrides that caused layout issues
- [x] Removed duplicate theme preview widgets in Appearance tab
- [x] Fixed license tab text browser to fill all available viewport space
- [x] Cleaned up blank widgets and layout spacing issues
## 4. TextEditor Enhancements
 [x] Implement custom divider rendering: full-width line with centered, rounded title overlay
 [x] Implement both list options (bulleted and numbered lists)
 [x] Refactor ThemePreview to show stub FileBrowser preview (theme colors, no content)
 [x] Fixed status bar preview rendering in ThemePreview
 [ ] Add support for inserting images from Android camera
 [ ] Ensure image insertion works on desktop and Android
 [ ] Polish text selection, formatting, and undo/redo behavior
## 5. General Polish & QA

- [ ] Add smoke/unit tests for core features (file ops, editor, settings)
- [ ] Review and optimize performance (startup, memory usage)
- [ ] Update documentation and help screens
- [ ] Finalize icons, translations, and resource files
- [ ] Prepare release builds for desktop and Android



---

**Goal:** Achieve a stable, beautiful, and touch-friendly notes app with feature parity across platforms. Prioritize reliability, polish, and user experience for 1.0.
