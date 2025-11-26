#!/bin/bash
chmod +x "$0"
# Function to display an error message and exit
function error_exit {
    echo "$1" 1>&2
    exit 1
}

echo "Cleaning up..."
if [ -d "build" ]; then
    rm -rf build || error_exit "Failed to clean up."
fi