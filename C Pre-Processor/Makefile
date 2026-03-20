# ===========================================================================
# Makefile – mypreprocessor
# ===========================================================================
#
# Targets
# -------
#   make          – compile all modules and link the executable
#   make clean    – remove all object files and the executable
#   make rebuild  – clean then build from scratch
#
# Usage after building
# --------------------
#   ./mypreprocessor inputfile.c
#   -> produces inputfile.i in the current directory
#
# Compiler flags
# --------------
#   -Wall   : enable most warnings
#   -Wextra : enable extra warnings
#   -std=c11: use the C11 standard (for for-loop variable declarations)
#   -g      : include debug symbols (remove for release builds)
# ===========================================================================

# ----- Configurable variables ------------------------------------------------

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
TARGET  = mypreprocessor

# ----- Source and object lists -----------------------------------------------

SRCS = main.c \
       comment_removal.c \
       file_inclusion.c \
       macro_handler.c

# Derive object file names automatically from source names
OBJS = $(SRCS:.c=.o)

# ----- Default target ---------------------------------------------------------

all: $(TARGET)
	@echo "Build successful – run with: ./$(TARGET) <source.c>"

# ----- Link step --------------------------------------------------------------

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# ----- Compile step (pattern rule) -------------------------------------------
# Each .c file depends on the central header so that any header change
# triggers a recompile of every module.

%.o: %.c preprocessor.h
	$(CC) $(CFLAGS) -c -o $@ $<

# ----- Phony targets ----------------------------------------------------------

.PHONY: all clean rebuild

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned build artefacts."

rebuild: clean all
