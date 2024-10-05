OBJ = main.o server.o client.o udp_socket.o multicast_app.o exceptions/socket_exceptions.o
CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -O2
INCLUDE_DIR = ./include
OUT_DIR = ./tmp

all: $(OBJ)
	cd $(OUT_DIR) && \
	$(CXX) $(CXXFLAGS) $^ -o ../app

$(OBJ): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) -c $^ -o $(OUT_DIR)/$@

clean:
	rm -rf $(OUT_DIR)

tmp:
	mkdir tmp && mkdir tmp/exceptions