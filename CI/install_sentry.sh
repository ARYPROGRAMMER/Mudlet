#!/bin/bash
############################################################################
#   Copyright (C) 2024 by ARYPROGRAMMER                                     #
#   Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com          #
#                                                                          #
#   This program is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by   #
#   the Free Software Foundation; either version 2 of the License, or      #
#   (at your option) any later version.                                    #
############################################################################

set -e

SENTRY_VERSION="0.6.7"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-sentry"

# Print system information
echo "System Information:"
uname -a
echo "Compiler Version:"
if [ "$(uname)" == "Darwin" ]; then
    clang --version
else
    gcc --version
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Platform-specific setup
if [ "$(uname)" == "Darwin" ]; then
    # macOS setup
    echo "Setting up Sentry for macOS..."
    
    # Check for Homebrew
    if ! command -v brew >/dev/null 2>&1; then
        echo "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Install dependencies
    brew install cmake ninja pkg-config
    
    # Set up environment
    export MACOSX_DEPLOYMENT_TARGET=12.0
    COMPILE_FLAGS="-arch x86_64 -arch arm64"
    
else
    # Linux setup
    echo "Setting up Sentry for Linux..."
    
    # Install dependencies
    if [ -f /etc/debian_version ]; then
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            pkg-config \
            libcurl4-openssl-dev \
            libssl-dev
    elif [ -f /etc/fedora-release ]; then
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            ninja-build \
            pkg-config \
            libcurl-devel \
            openssl-devel
    else
        echo "Unsupported Linux distribution"
        exit 1
    fi
    
    COMPILE_FLAGS="-fPIC"
fi

# Download and extract Sentry SDK
echo "Downloading Sentry SDK version $SENTRY_VERSION..."
curl -L "https://github.com/getsentry/sentry-native/releases/download/$SENTRY_VERSION/sentry-native.zip" -o sentry-native.zip
unzip -q sentry-native.zip
cd sentry-native

# Configure build
echo "Configuring Sentry build..."
mkdir -p build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$COMPILE_FLAGS" \
    -DCMAKE_CXX_FLAGS="$COMPILE_FLAGS" \
    -DSENTRY_BUILD_SHARED_LIBS=OFF \
    -DSENTRY_BUILD_TESTS=OFF \
    -DSENTRY_BUILD_EXAMPLES=OFF \
    -GNinja

# Build and install
echo "Building Sentry..."
ninja

echo "Installing Sentry..."
if [ "$(uname)" == "Darwin" ]; then
    sudo ninja install
else
    sudo ninja install
    sudo ldconfig
fi

# Verify installation
echo "Verifying Sentry installation..."
if [ -f /usr/local/include/sentry.h ]; then
    echo "Sentry installation successful!"
else
    echo "Sentry installation failed!"
    exit 1
fi

# Cleanup
cd "$SCRIPT_DIR"
rm -rf "$BUILD_DIR"

echo "Sentry SDK installation completed."