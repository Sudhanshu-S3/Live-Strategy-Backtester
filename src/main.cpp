#define _HAS_STD_BYTE 0
#include <iostream>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../include/ui/ConsoleUI.h"

int main() {
    try {
        ConsoleUI ui;
        ui.displayMainMenu();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}