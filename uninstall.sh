#!/bin/bash
set -e

APP_NAME="cloudflare-warp-qt"
BINARY_NAME="${APP_NAME}"
DESKTOP_FILE="${APP_NAME}.desktop"
ICON_FILE="${APP_NAME}.png"

echo "Uninstalling $APP_NAME..."

# Remove the Binary
for BIN_DIR in "/usr/bin" "/usr/local/bin"; do
  if [ -f "$BIN_DIR/$BINARY_NAME" ]; then
    echo "Removing binary from $BIN_DIR..."
    sudo rm -f "$BIN_DIR/$BINARY_NAME"
  fi
done

# Remove Desktop Entry
for DESKTOP_DIR in "/usr/share/applications" "/usr/local/share/applications"; do
  if [ -f "$DESKTOP_DIR/$DESKTOP_FILE" ]; then
    echo "Removing desktop entry from $DESKTOP_DIR..."
    sudo rm -f "$DESKTOP_DIR/$DESKTOP_FILE"
  fi
done

# Remove Icon
for ICON_DIR in "/usr/share/icons/hicolor/256x256/apps" "/usr/local/share/icons/hicolor/256x256/apps"; do
  if [ -f "$ICON_DIR/$ICON_FILE" ]; then
    echo "Removing icon from $ICON_DIR..."
    sudo rm -f "$ICON_DIR/$ICON_FILE"
  fi
done

# Update desktop database
echo "Updating desktop database..."
if command -v update-desktop-database >/dev/null 2>&1; then
  sudo update-desktop-database
fi

echo "$APP_NAME has been uninstalled"
