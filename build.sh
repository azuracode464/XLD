#!/bin/sh
set -e

# Colors
RED="\033[0;31m"
GREEN="\033[0;32m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
NC="\033[0m" # No color

echo -e "${YELLOW}🧹 CLEAN BUILD...${NC}"
make clean
echo -e "${GREEN}✅ Clean complete!${NC}"

echo -e "${BLUE}⚡ BUILD in progress...${NC}"
make TOOLCHAIN=llvm
echo -e "${GREEN}🚀 Build finished successfully!${NC}"

# Ask user if they want to run
read -p "💻 Do you want to run the program now? (y/n) " choice
case "$choice" in 
  y|Y ) 
    echo -e "${BLUE}🏃‍♂️ Running the program...${NC}"
    make run
    ;;
  n|N ) 
    echo -e "${YELLOW}⏸️ Execution canceled by user.${NC}"
    ;;
  * ) 
    echo -e "${RED}❌ Invalid option!${NC}"
    ;;
esac
