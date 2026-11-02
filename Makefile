CXX      = x86_64-w64-mingw32-g++
CXXFLAGS = -std=c++17 -Wall -O2 -municode -DUNICODE -D_UNICODE
LDFLAGS  = -static -municode
LIBS     = -ladvapi32

SRC_DIR   = src
BUILD_DIR = build
TARGET    = BackupService.exe

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
