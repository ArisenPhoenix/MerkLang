#!/bin/bash

EXT_NAME="merk"
VSIX_NAME="$EXT_NAME-0.0.1.vsix"


# Go to extension folder
cd "$(dirname "$0")/merk-extension/merk" || exit 1

echo "🔁 Uninstalling previous version of $EXT_NAME..."
code --uninstall-extension $EXT_NAME

echo "Recompiling Extension"
yarn run compile

echo "📦 Packaging new version..."
vsce package

echo "📥 Reinstalling extension from $VSIX_NAME..."
code --install-extension "./$VSIX_NAME" --allow-missing-repository --force

echo "🔄 Reloading VS Code window..."
# code --reload-window

echo "✅ Extension reinstalled and window reloaded!"
