CC = gcc
CFLAGS += -I../../include
CFLAGS += -L../../build
CFLAGS += -lsyscall_intercept
CFLAGS += -fpic -shared

BIN_DIR = bin
BUILD_DIR = build
SRC_DIR = src

LD_LIBRARY_PATH = ./build:../../build
LD_PRELOAD = ./build

TESTS += clone
TESTS += fcntl
TESTS += mkdirat
TESTS += openat
TESTS += read
TESTS += statx
TESTS += write

EXECUTABLES += $(BIN_DIR)/clone_test
EXECUTABLES += $(BIN_DIR)/fcntl_test
EXECUTABLES += $(BIN_DIR)/mkdirat_test
EXECUTABLES += $(BIN_DIR)/openat_test
EXECUTABLES += $(BIN_DIR)/read_test
EXECUTABLES += $(BIN_DIR)/statx_test
EXECUTABLES += $(BIN_DIR)/write_test

INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_clone.so
INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_fcntl.so
INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_mkdirat.so
INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_openat.so
INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_read.so
INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_statx.so
INTERCEPT_LIBS += $(BUILD_DIR)/intercept_sys_write.so

all: $(BIN_DIR) $(BUILD_DIR) $(EXECUTABLES) $(INTERCEPT_LIBS)

$(BUILD_DIR)/%.so: $(SRC_DIR)/%.c
	$(CC) -o $@ $< $(CFLAGS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CC) $< -o $@

test: $(TESTS)

$(TESTS):
	@echo "Running test: $@"
	@LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) LD_PRELOAD=$(LD_PRELOAD)/intercept_sys_$@.so $(BIN_DIR)/$@_test \
		&& echo "Test $@ - \033[32mPASSED\033[0m" \
		|| echo "Test $@ - \033[31mFAILED\033[0m"
	@echo

clean:
	rm -f $(BIN_DIR)/*
	rm -f $(BUILD_DIR)/*
	rm -f *.txt
	rm -rf wrongdir/

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
