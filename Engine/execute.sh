#!/bin/bash
chmod +x "$0"
# Function to display an error message and exit
function error_exit {
    echo "$1" 1>&2
    exit 1
}

GAME_PATH="${1:-}"
MODE1="${2:-}"
MODE2="${3:-}"

# Check if the build directory exists
if [ ! -d "build" ]; then
    echo "Build directory does not exist. Creating build directory...."
    mkdir build || error_exit "Failed to create build directory."

    echo "Build directory created."
    cd build || error_exit "Failed to change to build directory."

    echo "Building project..."
    cmake .. || error_exit "Failed to run CMake."
else 
    # Change to the build directory
    echo "Changing to the build directory..."
    cd build || error_exit "Failed to change to build directory."
fi

echo "Building the project with make..."
make -j || error_exit "Failed to build project."
echo "Project built."
cd ../
./Engine "$GAME_PATH" "$MODE1" "$MODE2" || error_exit "Failed to launch the project."
