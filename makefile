CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./ds

TARGET = CDB
OBJS = main.o ds.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

main.o: ./src/main.cpp ./ds/ds.h
	$(CXX) $(CXXFLAGS) -c ./src/main.cpp -o main.o

ds.o: ./ds/ds.cpp ./ds/ds.h
	$(CXX) $(CXXFLAGS) -c ./ds/ds.cpp -o ds.o

clean:
	rm -f $(OBJS) $(TARGET)