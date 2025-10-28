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

    std::vector<std::string> getUserInput(std::string currentDirectory)
    {
        std::string input;
        std::cout << currentDirectory << ">";
        std::getline(std::cin, input);
        return {input};
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

    /**
     * Splits a string by the given delimiter and returns a vector of tokens.
     */
    std::vector<std::string> splitString(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        for (char ch : str)
        {
            if (ch == delimiter)
            {
                if (!token.empty())
                {
                    tokens.push_back(token);
                    token.clear();
                }
            }
            else
            {
                token += ch;
            }
        }
        if (!token.empty())
        {
            tokens.push_back(token);
        }
        return tokens;
    }
};

int main()
{
    std::string currentDirectory = "C:\\GreggyOS";
    CommandLineInterface handler;
    handler.getUserInput(currentDirectory);
    return 0;
}