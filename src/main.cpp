#include <iostream>
#include <vector>
#include <CPU.cpp>

class GreggyOS
{
    std::vector<CPU> cpus;

public:
    GreggyOS()
    {
        // Constructor implementation
    }

    void handleNext()
    {
        for (int i = 0; i < cpus.size(); ++i)
        {
            cpus[i].executeNext();
        }
    }
};

int main()
{
    GreggyOS os;
    std::cout << "Hello, World!" << std::endl;
    return 0;
}