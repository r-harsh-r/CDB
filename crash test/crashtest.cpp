#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fstream>
#include <cstdlib>
#include "../ds/ds.h" // Replace with your actual header

const int TARGET_INSERTS = 10000;
const char* DB_FILE = "crash_test.db";
const char* LOG_FILE = "last_acked.txt";

// Helper to log the last successfully inserted key to a plain text file
void recordAck(int i) {
    std::ofstream log(LOG_FILE, std::ios::trunc);
    log << i;
    log.flush(); // Ensure the OS writes this log immediately
}

int getAck() {
    std::ifstream log(LOG_FILE);
    int i = -1;
    log >> i;
    return i;
}

void runWriterProcess() {
    DiskFile db(DB_FILE);
    
    // Pick a random time to crash (between 1000 and 5000 inserts)
    srand(getpid());
    int crash_point = 1000 + (rand() % 4000);
    
    std::cout << "[Child] Starting inserts. Planned crash at insert #" << crash_point << "...\n";

    for (int i = 0; i < TARGET_INSERTS; i++) {
        std::string key = "key_" + std::to_string(i);
        std::string val = "val_" + std::to_string(i);

        db.set(key, val); // Your insert function that calls fsync()
        
        // If we reach here, the DB claims the data is safe on disk.
        recordAck(i);

        // SUDDEN DEATH
        if (i == crash_point) {
            std::cout << "[Child] CRASHING NOW (SIGKILL)!\n";
            raise(SIGKILL); // Instant death. OS reclaims memory, buffers are dropped.
        }
    }
}

void runVerifierProcess() {
    int last_acked = getAck();
    std::cout << "[Parent] Child died. Last acknowledged insert was #" << last_acked << "\n";
    std::cout << "[Parent] Verifying database integrity...\n";

    DiskFile db(DB_FILE);
    int missing_keys = 0;
    int corrupted_reads = 0;

    for (int i = 0; i <= last_acked; i++) {
        std::string key = "key_" + std::to_string(i);
        std::string expected_val = "val_" + std::to_string(i);
        std::string retrieved_val;

        try {
            bool found = db.get(key, retrieved_val);
            if (!found) {
                missing_keys++;
                std::cout << "  -> MISSING: " << key << "\n";
            } else if (retrieved_val != expected_val) {
                corrupted_reads++;
                std::cout << "  -> CORRUPT: " << key << " (Got: " << retrieved_val << ")\n";
            }
        } catch (...) {
            std::cout << "[Parent] CRITICAL: Database structure is broken. Segfault/Exception on read!\n";
            exit(1);
        }
    }

    if (missing_keys == 0 && corrupted_reads == 0) {
        std::cout << "\n✅ CRASH TEST PASSED!\n";
        std::cout << "Atomicity: No torn pages or structural corruption detected.\n";
        std::cout << "Durability: All " << (last_acked + 1) << " acknowledged keys survived the SIGKILL.\n";
    } else {
        std::cout << "\n❌ CRASH TEST FAILED!\n";
        std::cout << "Missing keys: " << missing_keys << "\n";
        std::cout << "Corrupted reads: " << corrupted_reads << "\n";
    }
}

int main() {
    // Clean up old test files
    unlink(DB_FILE);
    unlink(LOG_FILE);

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process writes data and kills itself
        runWriterProcess();
        exit(0);
    } else {
        // Parent process waits for child to die, then verifies
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) {
            runVerifierProcess();
        } else {
            std::cout << "[Parent] Child did not crash as expected.\n";
        }
    }
    return 0;
}