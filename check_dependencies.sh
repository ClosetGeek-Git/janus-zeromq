#!/bin/bash
#
# Dependency checker for Janus ZeroMQ plugins
# This script checks if all required dependencies are installed

echo "Checking dependencies for Janus ZeroMQ plugins..."
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

all_ok=true

# Check for gcc
echo -n "Checking for gcc... "
if command -v gcc &> /dev/null; then
    version=$(gcc --version | head -1)
    echo -e "${GREEN}✓${NC} $version"
else
    echo -e "${RED}✗ Not found${NC}"
    all_ok=false
fi

# Check for pkg-config
echo -n "Checking for pkg-config... "
if command -v pkg-config &> /dev/null; then
    version=$(pkg-config --version)
    echo -e "${GREEN}✓${NC} version $version"
else
    echo -e "${RED}✗ Not found${NC}"
    all_ok=false
fi

# Check for ZeroMQ
echo -n "Checking for ZeroMQ (libzmq)... "
if pkg-config --exists libzmq; then
    version=$(pkg-config --modversion libzmq)
    echo -e "${GREEN}✓${NC} version $version"
    
    # Check if version is at least 4.3.4
    required="4.3.4"
    if [ "$(printf '%s\n' "$required" "$version" | sort -V | head -n1)" = "$required" ]; then
        echo "  Version requirement (>= 4.3.4) satisfied"
    else
        echo -e "  ${YELLOW}⚠${NC} Version $version is older than recommended 4.3.4"
    fi
else
    echo -e "${RED}✗ Not found${NC}"
    echo "  Install: sudo apt-get install libzmq3-dev  # Ubuntu/Debian"
    echo "           sudo dnf install zeromq-devel      # Fedora/RHEL"
    echo "           brew install zeromq                # macOS"
    all_ok=false
fi

# Check for GLib 2.0
echo -n "Checking for GLib 2.0... "
if pkg-config --exists glib-2.0; then
    version=$(pkg-config --modversion glib-2.0)
    echo -e "${GREEN}✓${NC} version $version"
else
    echo -e "${RED}✗ Not found${NC}"
    echo "  Install: sudo apt-get install libglib2.0-dev  # Ubuntu/Debian"
    echo "           sudo dnf install glib2-devel         # Fedora/RHEL"
    echo "           brew install glib                    # macOS"
    all_ok=false
fi

# Check for Jansson
echo -n "Checking for Jansson... "
if pkg-config --exists jansson; then
    version=$(pkg-config --modversion jansson)
    echo -e "${GREEN}✓${NC} version $version"
else
    echo -e "${RED}✗ Not found${NC}"
    echo "  Install: sudo apt-get install libjansson-dev  # Ubuntu/Debian"
    echo "           sudo dnf install jansson-devel       # Fedora/RHEL"
    echo "           brew install jansson                 # macOS"
    all_ok=false
fi

echo ""
if [ "$all_ok" = true ]; then
    echo -e "${GREEN}✓ All dependencies are installed!${NC}"
    echo "You can now run 'make' to build the plugins."
    exit 0
else
    echo -e "${RED}✗ Some dependencies are missing.${NC}"
    echo "Please install the missing dependencies and try again."
    exit 1
fi
