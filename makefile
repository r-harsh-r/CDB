CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./ds

TARGET = CDB
OBJS = main.o ds.o

TEST_TARGET = test_ds
TEST_OBJS = test_ds.o ds.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) -lgtest -lgtest_main -lpthread

main.o: ./src/main.cpp ./ds/ds.h
	$(CXX) $(CXXFLAGS) -c ./src/main.cpp -o main.o

ds.o: ./ds/ds.cpp ./ds/ds.h
	$(CXX) $(CXXFLAGS) -c ./ds/ds.cpp -o ds.o

test_ds.o: ./tests/test_ds.cpp ./ds/ds.h
	$(CXX) $(CXXFLAGS) -c ./tests/test_ds.cpp -o test_ds.o

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET) $(TEST_OBJS)