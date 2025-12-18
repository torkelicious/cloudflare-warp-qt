#!/bin/bash

set -e

APP_NAME="CloudflareWarpQt"
BINARY_NAME="CloudflareWarpQt"
DESKTOP_FILE="CloudflareWarpQt.desktop"
ICON_FILE="cloudflare-warp-qt.png"

echo "Uninstalling $APP_NAME..."

# Remove the Binary
if [ -f "/usr/bin/$BINARY_NAME" ]; then
    echo "Removing binary from /usr/bin..."
    sudo rm -f "/usr/bin/$BINARY_NAME"
fi

if [ -f "/usr/local/bin/$BINARY_NAME" ]; then
    echo "Removing binary from /usr/local/bin..."
    sudo rm -f "/usr/local/bin/$BINARY_NAME"
fi

# Remove desktop entry
if [ -f "/usr/share/applications/$DESKTOP_FILE" ]; then
    echo "Removing desktop entry..."
    sudo rm -f "/usr/share/applications/$DESKTOP_FILE"
fi

if [ -f "/usr/local/share/applications/$DESKTOP_FILE" ]; then
    echo "Removing local desktop entry..."
    sudo rm -f "/usr/local/share/applications/$DESKTOP_FILE"
fi

# Remove icon file
if [ -f "/usr/share/pixmaps/$ICON_FILE" ]; then
    echo "Removing icon..."
    sudo rm -f "/usr/share/pixmaps/$ICON_FILE"
fi

if [ -f "/usr/local/share/pixmaps/$ICON_FILE" ]; then
    echo "Removing local icon..."
    sudo rm -f "/usr/local/share/pixmaps/$ICON_FILE"
fi

# Update dekstop db
echo "Updating desktop database..."
if command -v update-desktop-database >/dev/null 2>&1; then
    sudo update-desktop-database
fi

echo "$APP_NAME has been uninstalled"
