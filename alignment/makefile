BUILD := release

BIN_DIR.debug   := bin_debug
BIN_DIR.release := bin
BIN_DIR := $(BIN_DIR.$(BUILD))

FLAGS.debug   := -g -Wall -pedantic -DDEBUG
FLAGS.release := -O2 -g -Wall -pedantic
FLAGS := $(FLAGS.$(BUILD))

all: $(BIN_DIR)/alignment $(BIN_DIR)/max_cost

$(BIN_DIR)/alignment: alignment.c | $(BIN_DIR)
	$(CC) $(FLAGS) -DMAIN -o $@ $^

$(BIN_DIR)/max_cost: alignment.c max_cost.c | $(BIN_DIR)
	$(CC) $(FLAGS) -o $@ $^

$(BIN_DIR):
	mkdir $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all
