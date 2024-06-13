# Makefile

# 编译器
CXX = g++

# 编译器选项
CXXFLAGS = -std=c++11 -Iinclude -g -Wall -pthread

# 目标可执行文件
TARGET = build/MyServer

# 源文件目录
SRC_DIR = src

# 构建目录
BUILD_DIR = build

# 源文件
SRCS = $(wildcard $(SRC_DIR)/*.cpp) main.cpp

# 对应的目标文件
OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))

# 链接器选项
LDFLAGS = -pthread

# 默认目标
all: $(BUILD_DIR) $(TARGET)

# 构建目标
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 编译源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# 构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 清理目标
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# 伪目标
.PHONY: all clean
