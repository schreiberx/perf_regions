#!/bin/bash
set -e

VENV_DIR=".venv"

if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment in $VENV_DIR..."
    python3 -m venv "$VENV_DIR"
else
    echo "Virtual environment already exists in $VENV_DIR."
fi

echo "Activating virtual environment..."
source "$VENV_DIR/bin/activate"

if [ -f "requirements.txt" ]; then
    echo "Installing requirements..."
    pip install --upgrade pip
    pip install -r requirements.txt
else
    echo "requirements.txt not found. Skipping dependency installation."
fi

echo "Setup complete. To activate run: source $VENV_DIR/bin/activate"
