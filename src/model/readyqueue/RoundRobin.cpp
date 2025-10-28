#include "ReadyQueue.cpp"

class RoundRobin : public ReadyQueue
{
    int timeQuantum;

public:
    RoundRobin(int tq) : timeQuantum(tq)
    {
    }

    /**
     * Gets the time quantum for the round-robin scheduler
     */
    int getTimeQuantum() const
    {
        return timeQuantum;
    }
};