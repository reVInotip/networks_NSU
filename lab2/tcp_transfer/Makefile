OBJ = main.o tcp_server.o tcp_client.o
CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -O2
INCLUDE_DIR = ./include
OUT_DIR = ./tmp

all: $(OBJ)
	cd $(OUT_DIR) && \
	$(CXX) $(CXXFLAGS) $^ -lstdc++fs -o ../app

$(OBJ): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) -c $^ -o $(OUT_DIR)/$@

clean:
	rm -rf $(OUT_DIR)

tmp:
	mkdir tmp && mkdir tmp/exceptions