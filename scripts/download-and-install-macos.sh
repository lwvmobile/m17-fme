#!/bin/bash

# M17 Project - Florida Man Edition
# macOS Install Script using Homebrew
#
# This script installs all dependencies and builds M17-FME on macOS
# Requires Homebrew to be installed first (https://brew.sh)

echo ""
echo "==================================================================="
echo "M17 Project - Florida Man Edition - macOS Installation Script"
echo "==================================================================="
echo ""

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew is not installed. Please install it first:"
    echo "   /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
fi

echo "✅ Homebrew found: $(brew --version | head -n1)"
echo ""

# Update Homebrew
echo "📦 Updating Homebrew..."
brew update

echo ""
echo "📦 Installing required dependencies..."

# Required dependencies
echo "   Installing cmake, make, git..."
brew install cmake make git

echo "   Installing libsndfile (required)..."
brew install libsndfile

echo ""
echo "📦 Installing recommended dependencies..."

# Optional but recommended dependencies
echo "   Installing codec2 (for M17 voice support)..."
brew install codec2

echo "   Installing ncurses (for terminal interface)..."
brew install ncurses

echo "   Installing PulseAudio (for audio I/O)..."
brew install pulseaudio

echo "   Installing wget and socat (utilities)..."
brew install wget socat

echo ""
echo "📦 Installing build tools..."
brew install gcc pkg-config

echo ""
echo "✅ All dependencies installed successfully!"
echo ""

# Check if we're in the m17-fme directory or need to download
if [ ! -f "CMakeLists.txt" ] || [ ! -d "src" ]; then
    echo "📥 Downloading M17-FME source code..."
    if [ -d "m17-fme" ]; then
        rm -rf m17-fme
    fi
    git clone --recursive https://github.com/lwvmobile/m17-fme.git
    cd m17-fme
else
    echo "📁 Found M17-FME source in current directory"
fi

echo ""
echo "🔨 Building M17-FME..."

# Create build directory
if [ -d "build" ]; then
    echo "   Cleaning previous build..."
    rm -rf build
fi

mkdir build
cd build

echo "   Running cmake..."
cmake ..

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

echo "   Compiling..."
make

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed"
    exit 1
fi

echo ""
echo "🔧 Installing M17-FME..."
sudo make install

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 M17-FME has been successfully installed!"
    echo ""
    echo "📝 macOS-specific notes:"
    echo "   • PulseAudio may need to be started manually: 'pulseaudio --start'"
    echo "   • For first-time PulseAudio use, you may need to configure audio permissions"
    echo "   • OSS audio is not available on macOS - use PulseAudio or file I/O instead"
    echo "   • If you encounter audio permission issues, check System Preferences > Security & Privacy > Microphone"
    echo ""
    echo "📖 Usage: Run 'm17-fme --help' for command line options"
    echo "📖 Documentation: See docs/Example_Usage.md for detailed usage examples"
    echo ""
    echo "🔊 To verify your installation, try:"
    echo "   m17-fme --version"
else
    echo "❌ Installation failed"
    exit 1
fi
