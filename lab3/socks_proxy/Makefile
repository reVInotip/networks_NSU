OBJ = main.o socks_server.o tunnel.o socket.o
CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -O2 -g -std=c++20
INCLUDE_DIR = ./include
OUT_DIR = ./tmp

all: $(OBJ)
	cd $(OUT_DIR) && \
	$(CXX) $(CXXFLAGS) $^ -lstdc++fs -lcares -o ../app

$(OBJ): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) -c $^ -o $(OUT_DIR)/$@

clean:
	rm -rf $(OUT_DIR)

tmp:
	mkdir tmp && mkdir tmp/exceptions