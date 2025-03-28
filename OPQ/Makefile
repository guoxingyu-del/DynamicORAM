SRC_DIR := src
OBJ_DIR := obj
TEST_DIR := test

CPPFLAGS := -Iinclude
CFLAGS := -std=c++17 -g -Werror -O3
CTESTFLAGS := -I/usr/include/catch2

MAIN = PORAMRun
TEST = PORAMTest

# 主程序构建
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# 测试程序构建
TEST_SRC := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJ += $(patsubst $(TEST_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(TEST_SRC))
MAIN_OBJ := $(OBJ_DIR)/main.o  # 明确指定要排除的main.o

.PHONY: all test clean

all: $(MAIN)

test: $(TEST)

$(MAIN): $(OBJ_FILES)
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ $^

$(TEST): $(filter-out $(MAIN_OBJ), $(OBJ_FILES)) $(TEST_OBJ)
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(CTESTFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(MAIN) $(TEST)
