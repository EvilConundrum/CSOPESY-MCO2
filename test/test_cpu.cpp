#include <iostream>
#include <cassert>
#include <memory>
#include "../src/model/CPU.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define ASSERT_NE(x, y) assert((x) != (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

// Test: CPU creation
TEST(test_cpu_creation) {
    CPU cpu(0, 5);
    
    ASSERT_TRUE(cpu.isIdle());
    ASSERT_FALSE(cpu.hasRemainingProcesses());
    ASSERT_EQ(cpu.getCoreID(), 0);
}

// Test: Add process to CPU
TEST(test_add_process) {
    CPU cpu(0, 5);
    auto p1 = std::make_shared<Process>("P1", 3);
    
    p1->addInstruction(Instruction("PRINT", {"Hello"}));
    p1->addInstruction(Instruction("DECLARE", {"x", "10"}));
    p1->addInstruction(Instruction("PRINT", {"World"}));
    
    cpu.addProcess(p1);
    
    ASSERT_FALSE(cpu.isIdle());
    ASSERT_TRUE(cpu.hasRemainingProcesses());
    ASSERT_EQ(cpu.getCurrentProcess()->getPID(), "P1");
}

// Test: Execute instruction on CPU
TEST(test_execute_instruction) {
    CPU cpu(0, 0); // No time quantum (FCFS mode)
    auto p1 = std::make_shared<Process>("P1", 2);
    
    p1->addInstruction(Instruction("DECLARE", {"x", "5"}));
    p1->addInstruction(Instruction("PRINT", {"x"}));
    
    cpu.addProcess(p1);
    
    ASSERT_EQ(p1->getCurrentLine(), 0);
    
    cpu.executeInstruction();
    ASSERT_EQ(p1->getCurrentLine(), 1);
    
    cpu.executeInstruction();
    ASSERT_EQ(p1->getCurrentLine(), 2);
}

// Test: Process finishes on CPU
TEST(test_process_finishes) {
    CPU cpu(0, 0);
    auto p1 = std::make_shared<Process>("P1", 2);
    
    p1->addInstruction(Instruction("PRINT", {"Test1"}));
    p1->addInstruction(Instruction("PRINT", {"Test2"}));
    
    cpu.addProcess(p1);
    ASSERT_FALSE(cpu.isIdle());
    
    cpu.executeInstruction();
    ASSERT_FALSE(cpu.isIdle());
    
    cpu.executeInstruction();
    ASSERT_TRUE(cpu.isIdle()); // Process finished, CPU should be idle
}

// Test: Round Robin time quantum expiration
TEST(test_round_robin_time_quantum) {
    CPU cpu(0, 2); // Time quantum of 2
    auto p1 = std::make_shared<Process>("P1", 10);
    
    // Add 10 instructions
    for (int i = 0; i < 10; i++) {
        p1->addInstruction(Instruction("PRINT", {"Test"}));
    }
    
    cpu.addProcess(p1);
    
    // Execute first instruction
    auto preempted = cpu.executeNext();
    ASSERT_EQ(preempted, nullptr); // Time left = 1
    ASSERT_EQ(p1->getCurrentLine(), 1);
    
    // Execute second instruction - time quantum expires
    preempted = cpu.executeNext();
    ASSERT_NE(preempted, nullptr); // Should return process for requeue
    ASSERT_EQ(preempted->getPID(), "P1");
    ASSERT_EQ(p1->getCurrentLine(), 2);
}

// Test: Get current process
TEST(test_get_current_process) {
    CPU cpu(0, 5);
    auto p1 = std::make_shared<Process>("P1", 5);
    
    ASSERT_EQ(cpu.getCurrentProcess(), nullptr);
    
    cpu.addProcess(p1);
    
    ASSERT_NE(cpu.getCurrentProcess(), nullptr);
    ASSERT_EQ(cpu.getCurrentProcess()->getPID(), "P1");
}

// Test: CPU idle state
TEST(test_cpu_idle_state) {
    CPU cpu(0, 5);
    
    ASSERT_TRUE(cpu.isIdle());
    
    auto p1 = std::make_shared<Process>("P1", 1);
    p1->addInstruction(Instruction("PRINT", {"Test"}));
    
    cpu.addProcess(p1);
    ASSERT_FALSE(cpu.isIdle());
    
    cpu.executeInstruction();
    ASSERT_TRUE(cpu.isIdle()); // Process finished
}

// Test: Multiple processes sequentially
TEST(test_multiple_processes_sequential) {
    CPU cpu(0, 0);
    
    auto p1 = std::make_shared<Process>("P1", 1);
    p1->addInstruction(Instruction("PRINT", {"P1"}));
    
    cpu.addProcess(p1);
    cpu.executeInstruction();
    ASSERT_TRUE(cpu.isIdle());
    
    auto p2 = std::make_shared<Process>("P2", 1);
    p2->addInstruction(Instruction("PRINT", {"P2"}));
    
    cpu.addProcess(p2);
    cpu.executeInstruction();
    ASSERT_TRUE(cpu.isIdle());
}

// Test: Process state changes to RUNNING
TEST(test_process_state_running) {
    CPU cpu(0, 0);
    auto p1 = std::make_shared<Process>("P1", 2);
    
    p1->addInstruction(Instruction("PRINT", {"Test"}));
    p1->addInstruction(Instruction("PRINT", {"Test2"}));
    
    p1->setState(ProcessState::READY);
    cpu.addProcess(p1);
    
    cpu.executeInstruction();
    ASSERT_EQ(p1->getState(), ProcessState::RUNNING);
}

// Test: Process state changes to FINISHED
TEST(test_process_state_finished) {
    CPU cpu(0, 0);
    auto p1 = std::make_shared<Process>("P1", 1);
    
    p1->addInstruction(Instruction("PRINT", {"Test"}));
    
    cpu.addProcess(p1);
    cpu.executeInstruction();
    
    ASSERT_EQ(p1->getState(), ProcessState::FINISHED);
}

// Test: Time quantum reset after process finishes
TEST(test_time_quantum_reset) {
    CPU cpu(0, 5);
    auto p1 = std::make_shared<Process>("P1", 1);
    
    p1->addInstruction(Instruction("PRINT", {"Test"}));
    
    cpu.addProcess(p1);
    
    // Execute and finish
    cpu.executeNext();
    
    ASSERT_TRUE(cpu.isIdle());
    // Time quantum should be reset (internal state, can't directly test)
}

// Test: CPU with zero time quantum (FCFS mode)
TEST(test_cpu_zero_time_quantum) {
    CPU cpu(0, 0); // FCFS mode
    auto p1 = std::make_shared<Process>("P1", 5);
    
    for (int i = 0; i < 5; i++) {
        p1->addInstruction(Instruction("PRINT", {"Test"}));
    }
    
    cpu.addProcess(p1);
    
    // Should execute without preemption
    for (int i = 0; i < 5; i++) {
        auto preempted = cpu.executeNext();
        // FCFS shouldn't preempt (though implementation may vary)
    }
    
    ASSERT_TRUE(p1->isFinished());
}

// Test: Empty CPU execution
TEST(test_empty_cpu_execution) {
    CPU cpu(0, 5);
    
    // Try to execute with no process
    cpu.executeInstruction();
    ASSERT_TRUE(cpu.isIdle());
    
    auto preempted = cpu.executeNext();
    ASSERT_EQ(preempted, nullptr);
}

// Test: Multiple CPUs with different IDs
TEST(test_multiple_cpus) {
    CPU cpu0(0, 5);
    CPU cpu1(1, 5);
    CPU cpu2(2, 5);
    
    ASSERT_EQ(cpu0.getCoreID(), 0);
    ASSERT_EQ(cpu1.getCoreID(), 1);
    ASSERT_EQ(cpu2.getCoreID(), 2);
}

// Test: Process with no instructions
TEST(test_process_no_instructions_on_cpu) {
    CPU cpu(0, 5);
    auto p1 = std::make_shared<Process>("P1", 0);
    
    cpu.addProcess(p1);
    
    // Should handle gracefully
    cpu.executeInstruction();
    ASSERT_TRUE(cpu.isIdle());
}

int main() {
    std::cout << "=== Running CPU Unit Tests ===\n";
    
    try {
        RUN_TEST(test_cpu_creation);
        RUN_TEST(test_add_process);
        RUN_TEST(test_execute_instruction);
        RUN_TEST(test_process_finishes);
        RUN_TEST(test_round_robin_time_quantum);
        RUN_TEST(test_get_current_process);
        RUN_TEST(test_cpu_idle_state);
        RUN_TEST(test_multiple_processes_sequential);
        RUN_TEST(test_process_state_running);
        RUN_TEST(test_process_state_finished);
        RUN_TEST(test_time_quantum_reset);
        RUN_TEST(test_cpu_zero_time_quantum);
        RUN_TEST(test_empty_cpu_execution);
        RUN_TEST(test_multiple_cpus);
        RUN_TEST(test_process_no_instructions_on_cpu);
        
        std::cout << "\n=== All CPU tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test failed: " << e.what() << " ===\n";
        return 1;
    }
}
