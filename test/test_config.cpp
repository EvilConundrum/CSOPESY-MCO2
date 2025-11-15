#include <iostream>
#include <cassert>
#include <fstream>
#include "../src/model/Config.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

// Helper function to create a test config file
void createTestConfigFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

// Test: Load basic config
TEST(test_load_basic_config) {
    std::string configContent = 
        "num-cpu 4\n"
        "scheduler \"fcfs\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 3\n"
        "min-ins 100\n"
        "max-ins 200\n"
        "delay-per-exec 10\n";
    
    createTestConfigFile("test_config_basic.txt", configContent);
    
    Config config("test_config_basic.txt");
    
    ASSERT_EQ(config.getNumCpus(), 4);
    ASSERT_EQ(config.getSchedulerAlgorithm(), "fcfs");
    ASSERT_EQ(config.getQuantumCycles(), 5);
    ASSERT_EQ(config.getBatchProcessFreq(), 3);
    ASSERT_EQ(config.getMinInstructions(), 100);
    ASSERT_EQ(config.getMaxInstructions(), 200);
    ASSERT_EQ(config.getDelayPerExec(), 10);
}

// Test: Load config with Round Robin
TEST(test_load_rr_config) {
    std::string configContent = 
        "num-cpu 8\n"
        "scheduler \"rr\"\n"
        "quantum-cycles 10\n"
        "batch-process-freq 5\n"
        "min-ins 50\n"
        "max-ins 150\n"
        "delay-per-exec 5\n";
    
    createTestConfigFile("test_config_rr.txt", configContent);
    
    Config config("test_config_rr.txt");
    
    ASSERT_EQ(config.getNumCpus(), 8);
    ASSERT_EQ(config.getSchedulerAlgorithm(), "rr");
    ASSERT_EQ(config.getQuantumCycles(), 10);
}

// Test: Scheduler without quotes
TEST(test_scheduler_no_quotes) {
    std::string configContent = 
        "num-cpu 2\n"
        "scheduler fcfs\n"
        "quantum-cycles 1\n"
        "batch-process-freq 1\n"
        "min-ins 10\n"
        "max-ins 20\n"
        "delay-per-exec 1\n";
    
    createTestConfigFile("test_config_noquotes.txt", configContent);
    
    Config config("test_config_noquotes.txt");
    
    ASSERT_EQ(config.getSchedulerAlgorithm(), "fcfs");
}

// Test: Default time_between_instructions
TEST(test_default_time_between_instructions) {
    std::string configContent = 
        "num-cpu 4\n"
        "scheduler \"fcfs\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 3\n"
        "min-ins 100\n"
        "max-ins 200\n"
        "delay-per-exec 10\n";
    
    createTestConfigFile("test_config_default_time.txt", configContent);
    
    Config config("test_config_default_time.txt");
    
    ASSERT_EQ(config.getTimeBetweenInstructions(), 100); // Default value
}

// Test: Custom time_between_instructions
TEST(test_custom_time_between_instructions) {
    std::string configContent = 
        "num-cpu 4\n"
        "scheduler \"fcfs\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 3\n"
        "min-ins 100\n"
        "max-ins 200\n"
        "delay-per-exec 10\n"
        "time-between-instructions 50\n";
    
    createTestConfigFile("test_config_custom_time.txt", configContent);
    
    Config config("test_config_custom_time.txt");
    
    ASSERT_EQ(config.getTimeBetweenInstructions(), 50);
}

// Test: MO2 memory configurations
TEST(test_mo2_memory_config) {
    std::string configContent = 
        "num-cpu 4\n"
        "scheduler \"fcfs\"\n"
        "quantum-cycles 5\n"
        "batch-process-freq 3\n"
        "min-ins 100\n"
        "max-ins 200\n"
        "delay-per-exec 10\n"
        "max-overall-mem 1024\n"
        "mem-per-frame 64\n"
        "min-mem-per-proc 128\n"
        "max-mem-per-proc 512\n";
    
    createTestConfigFile("test_config_mo2.txt", configContent);
    
    Config config("test_config_mo2.txt");
    
    ASSERT_EQ(config.getMaxOverallMem(), 1024);
    ASSERT_EQ(config.getMemPerFrame(), 64);
    ASSERT_EQ(config.getMinMemPerProc(), 128);
    ASSERT_EQ(config.getMaxMemPerProc(), 512);
}

// Test: Empty config file
TEST(test_empty_config) {
    createTestConfigFile("test_config_empty.txt", "");
    
    // Should not crash
    Config config("test_config_empty.txt");
    
    // Default value should be set
    ASSERT_EQ(config.getTimeBetweenInstructions(), 100);
}

// Test: Nonexistent config file
TEST(test_nonexistent_config) {
    // Should handle gracefully
    Config config("nonexistent_file.txt");
    
    ASSERT_EQ(config.getTimeBetweenInstructions(), 100); // Default
}

// Test: Config with extra whitespace
TEST(test_config_with_whitespace) {
    std::string configContent = 
        "  num-cpu   4  \n"
        "scheduler  \"fcfs\"  \n"
        "  quantum-cycles  5\n";
    
    createTestConfigFile("test_config_whitespace.txt", configContent);
    
    Config config("test_config_whitespace.txt");
    
    // Should parse correctly despite extra whitespace
    ASSERT_EQ(config.getNumCpus(), 4);
    ASSERT_EQ(config.getSchedulerAlgorithm(), "fcfs");
    ASSERT_EQ(config.getQuantumCycles(), 5);
}

// Test: Large values
TEST(test_large_values) {
    std::string configContent = 
        "num-cpu 128\n"
        "scheduler \"rr\"\n"
        "quantum-cycles 1000\n"
        "batch-process-freq 100\n"
        "min-ins 10000\n"
        "max-ins 50000\n"
        "delay-per-exec 500\n";
    
    createTestConfigFile("test_config_large.txt", configContent);
    
    Config config("test_config_large.txt");
    
    ASSERT_EQ(config.getNumCpus(), 128);
    ASSERT_EQ(config.getQuantumCycles(), 1000);
    ASSERT_EQ(config.getMinInstructions(), 10000);
    ASSERT_EQ(config.getMaxInstructions(), 50000);
}

// Test: Minimum values
TEST(test_minimum_values) {
    std::string configContent = 
        "num-cpu 1\n"
        "scheduler \"fcfs\"\n"
        "quantum-cycles 1\n"
        "batch-process-freq 1\n"
        "min-ins 1\n"
        "max-ins 1\n"
        "delay-per-exec 0\n";
    
    createTestConfigFile("test_config_min.txt", configContent);
    
    Config config("test_config_min.txt");
    
    ASSERT_EQ(config.getNumCpus(), 1);
    ASSERT_EQ(config.getQuantumCycles(), 1);
    ASSERT_EQ(config.getDelayPerExec(), 0);
}

int main() {
    std::cout << "=== Running Config Unit Tests ===\n";
    
    try {
        RUN_TEST(test_load_basic_config);
        RUN_TEST(test_load_rr_config);
        RUN_TEST(test_scheduler_no_quotes);
        RUN_TEST(test_default_time_between_instructions);
        RUN_TEST(test_custom_time_between_instructions);
        RUN_TEST(test_mo2_memory_config);
        RUN_TEST(test_empty_config);
        RUN_TEST(test_nonexistent_config);
        RUN_TEST(test_config_with_whitespace);
        RUN_TEST(test_large_values);
        RUN_TEST(test_minimum_values);
        
        std::cout << "\n=== All Config tests passed! ===\n";
        
        // Cleanup test files
        std::remove("test_config_basic.txt");
        std::remove("test_config_rr.txt");
        std::remove("test_config_noquotes.txt");
        std::remove("test_config_default_time.txt");
        std::remove("test_config_custom_time.txt");
        std::remove("test_config_mo2.txt");
        std::remove("test_config_empty.txt");
        std::remove("test_config_whitespace.txt");
        std::remove("test_config_large.txt");
        std::remove("test_config_min.txt");
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test failed: " << e.what() << " ===\n";
        return 1;
    }
}
