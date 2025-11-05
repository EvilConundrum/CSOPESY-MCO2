#pragma once
#include <iostream>
#include <string>
#include <vector>

class CommandLineInterface
{
public:
    CommandLineInterface()
    {
        this->displayWelcomeMessage();
    }

    void displayMessage(const std::string &message = "")
    {
        std::cout << message << std::endl;
    }

    std::string getUserInput(std::string currentDirectory)
    {
        std::string input;
        std::cout << currentDirectory << ">";
        std::getline(std::cin, input);
        return input;
    }

private:
    void displayWelcomeMessage()
    {
        // clear screen
        std::cout << "\033[2J\033[1;1H";

        this->displayMessage(
            "   ____                             ___  ____  \n"
            "  / ___|_ __ ___  __ _  __ _ _   _ / _ \\/ ___| \n"
            " | |  _| '__/ _ \\/ _` |/ _` | | | | | | \\___ \\ \n"
            " | |_| | | |  __/ (_| | (_| | |_| | |_| |___) |\n"
            "  \\____|_|  \\___|\\__, |\\__, |\\__, |\\___/|____/ \n"
            "                 |___/ |___/ |___/             \n");

        this->displayMessage("Welcome to GreggyOS");

        this->displayMessage("Type 'help' to see available commands.");
        this->displayMessage();
    }
};

// int main()
// {
//     std::string currentDirectory = "C:\\GreggyOS";
//     CommandLineInterface handler;
//     handler.getUserInput(currentDirectory);
//     return 0;
// }