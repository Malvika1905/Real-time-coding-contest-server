CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -O2
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lws2_32
    MKDIR = if not exist $(BIN_DIR) mkdir $(BIN_DIR) & if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    BIN_EXT = .exe
else
    LDFLAGS = -pthread
    UNAME_S := $(shell uname -s 2>/dev/null || echo Linux)
    ifeq ($(UNAME_S),Linux)
        LDFLAGS += -lrt
    endif
    MKDIR = mkdir -p $(BIN_DIR) $(BUILD_DIR)
    BIN_EXT =
endif

BIN_DIR = bin
BUILD_DIR = build
SRC_DIR = src

# Object files common to server and utilities
COMMON_OBJS = $(BUILD_DIR)/protocol.o \
              $(BUILD_DIR)/contest.o \
              $(BUILD_DIR)/sync_manager.o \
              $(BUILD_DIR)/leaderboard.o \
              $(BUILD_DIR)/thread_pool.o \
              $(BUILD_DIR)/judge_sandbox.o \
              $(BUILD_DIR)/ipc_shared_memory.o \
              $(BUILD_DIR)/bankers_algo.o \
              $(BUILD_DIR)/memory_sim.o \
              $(BUILD_DIR)/disk_sim.o \
              $(BUILD_DIR)/monitor_dashboard.o \
              $(BUILD_DIR)/submission_manager.o \
              $(BUILD_DIR)/http_server.o

SERVER_TARGET  = $(BIN_DIR)/contest_server
CLIENT_TARGET  = $(BIN_DIR)/contest_client
MONITOR_TARGET = $(BIN_DIR)/monitor

.PHONY: all dirs clean demo_race demo_bankers demo_memory demo_disk

all: dirs $(SERVER_TARGET) $(CLIENT_TARGET) $(MONITOR_TARGET)
	@echo ==========================================================
	@echo Build complete! Binaries located in ./bin:
	@echo   - $(SERVER_TARGET)   (Contest Server)
	@echo   - $(CLIENT_TARGET)   (Interactive Participant CLI)
	@echo   - $(MONITOR_TARGET)  (Real-Time OS Dashboard)
	@echo ==========================================================

dirs:
	@$(MKDIR)

$(SERVER_TARGET): $(COMMON_OBJS) $(BUILD_DIR)/server_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(CLIENT_TARGET): $(BUILD_DIR)/protocol.o $(BUILD_DIR)/client_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(MONITOR_TARGET): $(BUILD_DIR)/ipc_shared_memory.o $(BUILD_DIR)/monitor_dashboard.o $(BUILD_DIR)/monitor_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/*.out $(BUILD_DIR)/*.cpp $(BIN_DIR)/*
	@echo "Clean completed."
