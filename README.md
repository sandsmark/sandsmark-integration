# Framework Integration

Integration of Qt application with Sandsmark's machines (advanced dotfiles I guess).


A fork-ish of the framework-integration when that was dropped in favor of the
plasma-integration plugin, and the maintainer wouldn't accept a patch that made
it work outside of plasma.

And I understand that people don't want to spend effort supporting frankenstein
setups like mine (90% KDE, but no plasma-shell or kwin), so I just keep this up
to date with anything useful from plasma-integration for myself.

## Introduction

Framework Integration is a set of plugins responsible for better integration of
Qt applications when running on Sandsmark's hacky setup.

Based on framework-integration, until that was "removed" in favor of
plasma-integration. And plasma-integration isn't designed to work outside of a
full Plasma desktop, so I soft-forked framework-integration before it was
broken.

Some differences from plasma-framework (there's more, ut things just work by now so I don't remember them):
    - Working system tray outside of Plasma.
    - Working (and better) keyboard navigation in file dialogs.
    - Fix crashing when upgrading packages that might touch palettes.
    - Fix default size of file dialogs (yes, even after the latest fix in plasma-framework).
    - No Wayland support (I don't use it, so it's just clutter).
    - Custom Qt widget style instead of breeze (basically just the default Qt
      Fusion style but with actual contrast on scroll bars).
    - Improved application launch speed (drop some waiting for dbus calls on startup).
    - Less dependencies
    - Unbroken handling of applications that don't set quitOnLastWindowClosed
      to false and also don't run the QCoreApplication main event loop, and
      then try to open a file dialog. Without this these applications would
      just hang on exit.
    - Fix crashing in pure QML apps (i. e. no QApplication)
    - Disabled useless warnings.

Also all fixes from plasma-integration are integrated.

