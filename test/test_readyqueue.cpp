#include <iostream>
#include <cassert>
#include <memory>
#include "../src/model/readyqueue/ReadyQueue.cpp"
#include "../src/model/readyqueue/FCFS.cpp"
#include "../src/model/readyqueue/RoundRobin.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define ASSERT_NE(x, y) assert((x) != (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

// Test: Basic enqueue and dequeue
TEST(test_basic_enqueue_dequeue) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    
    queue.enqueueProcess(p1);
    ASSERT_FALSE(queue.empty());
    ASSERT_EQ(queue.size(), 1);
    
    auto dequeued = queue.dequeueProcess();
    ASSERT_EQ(dequeued->getPID(), "P1");
    ASSERT_TRUE(queue.empty());
}

// Test: Peek doesn't remove process
TEST(test_peek_functionality) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    
    queue.enqueueProcess(p1);
    queue.enqueueProcess(p2);
    
    auto peeked = queue.peek();
    ASSERT_EQ(peeked->getPID(), "P1");
    ASSERT_EQ(queue.size(), 2); // Size unchanged
    
    auto dequeued = queue.dequeueProcess();
    ASSERT_EQ(dequeued->getPID(), "P1"); // Same process
}

// Test: FCFS ordering
TEST(test_fcfs_ordering) {
    FCFS fcfs;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    fcfs.enqueueProcess(p1);
    fcfs.enqueueProcess(p2);
    fcfs.enqueueProcess(p3);
    
    ASSERT_EQ(fcfs.dequeueProcess()->getPID(), "P1");
    ASSERT_EQ(fcfs.dequeueProcess()->getPID(), "P2");
    ASSERT_EQ(fcfs.dequeueProcess()->getPID(), "P3");
}

// Test: getAllProcesses method
TEST(test_get_all_processes) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    queue.enqueueProcess(p1);
    queue.enqueueProcess(p2);
    queue.enqueueProcess(p3);
    
    auto allProcs = queue.getAllProcesses();
    ASSERT_EQ(allProcs.size(), 3);
    ASSERT_EQ(allProcs[0]->getPID(), "P1");
    ASSERT_EQ(allProcs[1]->getPID(), "P2");
    ASSERT_EQ(allProcs[2]->getPID(), "P3");
    
    // Original queue should be unchanged
    ASSERT_EQ(queue.size(), 3);
}

// Test: contains method
TEST(test_contains_method) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    queue.enqueueProcess(p1);
    queue.enqueueProcess(p2);
    
    ASSERT_TRUE(queue.contains(p1));
    ASSERT_TRUE(queue.contains(p2));
    ASSERT_FALSE(queue.contains(p3));
    
    // Test with nullptr
    ASSERT_FALSE(queue.contains(nullptr));
}

// Test: contains by PID matching
TEST(test_contains_by_pid) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p1_duplicate = std::make_shared<Process>("P1", 5); // Same PID, different instance
    
    queue.enqueueProcess(p1);
    
    ASSERT_TRUE(queue.contains(p1_duplicate)); // Should find by PID
}

// Test: Clear functionality
TEST(test_clear_queue) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    
    queue.enqueueProcess(p1);
    queue.enqueueProcess(p2);
    ASSERT_EQ(queue.size(), 2);
    
    queue.clear();
    ASSERT_TRUE(queue.empty());
    ASSERT_EQ(queue.size(), 0);
}

// Test: Empty queue behavior
TEST(test_empty_queue_operations) {
    ReadyQueue queue;
    
    ASSERT_TRUE(queue.empty());
    ASSERT_EQ(queue.size(), 0);
    ASSERT_EQ(queue.dequeueProcess(), nullptr);
    ASSERT_EQ(queue.peek(), nullptr);
    
    auto allProcs = queue.getAllProcesses();
    ASSERT_EQ(allProcs.size(), 0);
}

// Test: Process state changes on enqueue
TEST(test_process_state_on_enqueue) {
    ReadyQueue queue;
    auto p1 = std::make_shared<Process>("P1", 5);
    
    p1->setState(ProcessState::WAITING);
    queue.enqueueProcess(p1);
    
    ASSERT_EQ(p1->getState(), ProcessState::READY);
}

// Test: Round Robin time quantum
TEST(test_round_robin_time_quantum) {
    RoundRobin rr(10);
    
    ASSERT_EQ(rr.getTimeQuantum(), 10);
    
    rr.setTimeQuantum(5);
    ASSERT_EQ(rr.getTimeQuantum(), 5);
}

// Test: Round Robin enqueue/dequeue
TEST(test_round_robin_enqueue_dequeue) {
    RoundRobin rr(5);
    auto p1 = std::make_shared<Process>("P1", 10);
    auto p2 = std::make_shared<Process>("P2", 10);
    
    rr.enqueueProcess(p1);
    rr.enqueueProcess(p2);
    
    ASSERT_EQ(rr.size(), 2);
    
    auto first = rr.dequeueProcess();
    ASSERT_EQ(first->getPID(), "P1");
    
    auto second = rr.dequeueProcess();
    ASSERT_EQ(second->getPID(), "P2");
}

// Test: Round Robin requeue functionality
TEST(test_round_robin_requeue) {
    RoundRobin rr(5);
    auto p1 = std::make_shared<Process>("P1", 10);
    
    p1->addInstruction(Instruction("PRINT", {"Test"}));
    
    rr.enqueueProcess(p1);
    auto dequeued = rr.dequeueProcess();
    
    ASSERT_TRUE(rr.empty());
    
    // Requeue the process
    rr.requeueProcess(dequeued);
    
    ASSERT_FALSE(rr.empty());
    ASSERT_EQ(rr.size(), 1);
}

// Test: Round Robin doesn't requeue finished process
TEST(test_round_robin_no_requeue_finished) {
    RoundRobin rr(5);
    auto p1 = std::make_shared<Process>("P1", 1);
    
    p1->addInstruction(Instruction("PRINT", {"Test"}));
    p1->executeNextInstruction(); // Execute the instruction
    p1->setState(ProcessState::FINISHED);
    
    ASSERT_TRUE(p1->isFinished());
    
    rr.requeueProcess(p1);
    
    ASSERT_TRUE(rr.empty()); // Shouldn't be enqueued
}

// Test: Multiple enqueue/dequeue cycles
TEST(test_multiple_cycles) {
    ReadyQueue queue;
    
    for (int i = 0; i < 10; i++) {
        auto p = std::make_shared<Process>("P" + std::to_string(i), 5);
        queue.enqueueProcess(p);
    }
    
    ASSERT_EQ(queue.size(), 10);
    
    for (int i = 0; i < 5; i++) {
        queue.dequeueProcess();
    }
    
    ASSERT_EQ(queue.size(), 5);
    
    for (int i = 10; i < 15; i++) {
        auto p = std::make_shared<Process>("P" + std::to_string(i), 5);
        queue.enqueueProcess(p);
    }
    
    ASSERT_EQ(queue.size(), 10);
}

// Test: Large queue operations
TEST(test_large_queue) {
    ReadyQueue queue;
    
    // Add 100 processes
    for (int i = 0; i < 100; i++) {
        auto p = std::make_shared<Process>("P" + std::to_string(i), 5);
        queue.enqueueProcess(p);
    }
    
    ASSERT_EQ(queue.size(), 100);
    
    auto allProcs = queue.getAllProcesses();
    ASSERT_EQ(allProcs.size(), 100);
    
    // Verify ordering
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(allProcs[i]->getPID(), "P" + std::to_string(i));
    }
}

// Test: Destructor clears queue
TEST(test_destructor_clears_queue) {
    {
        ReadyQueue queue;
        auto p1 = std::make_shared<Process>("P1", 5);
        auto p2 = std::make_shared<Process>("P2", 5);
        
        queue.enqueueProcess(p1);
        queue.enqueueProcess(p2);
        
        ASSERT_EQ(queue.size(), 2);
        // Queue goes out of scope and destructor is called
    }
    // If no memory leaks or crashes, test passes
    ASSERT_TRUE(true);
}

// Test: FCFS with single process
TEST(test_fcfs_single_process) {
    FCFS fcfs;
    auto p1 = std::make_shared<Process>("P1", 5);
    
    fcfs.enqueueProcess(p1);
    ASSERT_EQ(fcfs.size(), 1);
    
    auto dequeued = fcfs.dequeueProcess();
    ASSERT_EQ(dequeued->getPID(), "P1");
    ASSERT_TRUE(fcfs.empty());
}

// Test: Thread safety simulation (basic)
TEST(test_concurrent_operations_simulation) {
    ReadyQueue queue;
    
    // Simulate interleaved operations
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    
    queue.enqueueProcess(p1);
    auto size1 = queue.size();
    queue.enqueueProcess(p2);
    auto size2 = queue.size();
    
    ASSERT_EQ(size1, 1);
    ASSERT_EQ(size2, 2);
}

int main() {
    std::cout << "=== Running ReadyQueue Unit Tests ===\n";
    
    try {
        RUN_TEST(test_basic_enqueue_dequeue);
        RUN_TEST(test_peek_functionality);
        RUN_TEST(test_fcfs_ordering);
        RUN_TEST(test_get_all_processes);
        RUN_TEST(test_contains_method);
        RUN_TEST(test_contains_by_pid);
        RUN_TEST(test_clear_queue);
        RUN_TEST(test_empty_queue_operations);
        RUN_TEST(test_process_state_on_enqueue);
        RUN_TEST(test_round_robin_time_quantum);
        RUN_TEST(test_round_robin_enqueue_dequeue);
        RUN_TEST(test_round_robin_requeue);
        RUN_TEST(test_round_robin_no_requeue_finished);
        RUN_TEST(test_multiple_cycles);
        RUN_TEST(test_large_queue);
        RUN_TEST(test_destructor_clears_queue);
        RUN_TEST(test_fcfs_single_process);
        RUN_TEST(test_concurrent_operations_simulation);
        
        std::cout << "\n=== All ReadyQueue tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test failed: " << e.what() << " ===\n";
        return 1;
    }
}
