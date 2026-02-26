#include <gtest/gtest.h>
#include "../ds/ds.h"
#include <fstream>
#include <cstdio>

// Test Fixture for DiskFile
class DiskFileTest : public ::testing::Test {
protected:
    std::string test_db_name = "test_db_gtest";

    void SetUp() override {
        // Clean up before test starts to ensure a fresh state
        remove(test_db_name.c_str());
    }

    void TearDown() override {
        // Clean up after test finishes
        remove(test_db_name.c_str());
    }
};

// Test basic insertion and retrieval
TEST_F(DiskFileTest, BasicInsertAndGet) {
    {
        DiskFile db(test_db_name.c_str());
        std::string key = "key1";
        std::string val = "value1";
        db.insert(key, val);

        std::string out_val;
        bool found = db.get(key, out_val);
        EXPECT_TRUE(found);
        EXPECT_EQ(out_val, val);
    }
}

// Test multiple insertions
TEST_F(DiskFileTest, MultipleInsertions) {
    DiskFile db(test_db_name.c_str());
    int n = 5000;
    for (int i = 0; i < n; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string val = "val" + std::to_string(i);
        db.insert(key, val);
    }

    for (int i = 0; i < n; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string val = "val" + std::to_string(i);
        std::string out_val;
        bool found = db.get(key, out_val);
        EXPECT_TRUE(found) << "Failed to find key: " << key;
        EXPECT_EQ(out_val, val) << "Value mismatch for key: " << key;
    }
}

// Test persistence (close and reopen)
TEST_F(DiskFileTest, Persistence) {
    {
        DiskFile db(test_db_name.c_str());
        std::string key = "persistent_key";
        std::string val = "persistent_val";
        db.insert(key, val);
    } // db goes out of scope and closes file

    {
        DiskFile db(test_db_name.c_str());
        std::string key = "persistent_key";
        std::string out_val;
        bool found = db.get(key, out_val);
        EXPECT_TRUE(found);
        EXPECT_EQ(out_val, "persistent_val");
    }
}

// Test non-existent key
TEST_F(DiskFileTest, NonExistentKey) {
    DiskFile db(test_db_name.c_str());
    std::string key = "non_existent";
    std::string val = "val";
    db.insert(key, val);

    std::string query = "missing";
    std::string out_val;
    bool found = db.get(query, out_val);
    EXPECT_FALSE(found);
}

// Test updates (overwriting value for same key)
TEST_F(DiskFileTest, UpdateKey) {
   {
        DiskFile db(test_db_name.c_str());
        std::string key = "update_key";
        std::string val1 = "initial_val";
        db.insert(key, val1);
        
        std::string out_val;
        db.get(key, out_val);
        EXPECT_EQ(out_val, val1);

        std::string val2 = "updated_val";
        db.insert(key, val2); 

        db.get(key, out_val);
        EXPECT_EQ(out_val, val2);
   }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
