include ../make_base

BUILD_DIR = build
DEP_FILE = $(BUILD_DIR)/Makefile.dep
TARGET = $(BUILD_DIR)/output

SRCS := $(wildcard *.f90)
OBJS := $(SRCS:.f90=.o)
OBJS := $(addprefix $(BUILD_DIR)/, $(OBJS))
OBJS_JSON := $(SRCS:.f90=.json)
OBJS_JSON := $(addprefix $(BUILD_DIR)/, $(OBJS_JSON))

all: $(TARGET) $(DEP_FILE)

$(TARGET): $(OBJS)
	$(LD) $(TARGET).json $(OBJS_JSON)

$(BUILD_DIR)/%.o: %.f90 | $(BUILD_DIR)
	$(FC) -module-dir $(BUILD_DIR) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: depend
depend: $(DEP_FILE)

$(DEP_FILE): $(SRCS) | $(BUILD_DIR)
	@echo "Making dependencies!"
	$(FORTDEPEND_BIN) --build $(BUILD_DIR) -w -o $(DEP_FILE) -f $(SRCS)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

include $(DEP_FILE)
