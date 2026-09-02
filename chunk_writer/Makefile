NAME		:= chunk_writer

SRC_DIR		:= src
INC_DIR		:= inc
OBJ_DIR		:= obj

vpath %.c $(SRC_DIR)
SRC			:= 
SRC			+= chunk_writer.c

OBJ			:= $(SRC:%.c=%.o)
OBJ			:= $(addprefix $(OBJ_DIR)/, $(OBJ))

DEP			:= $(OBJ:%.o=%.d)

CC			:= cc
CFLAGS		:=
CFLAGS		+= -std=c23

CPPFLAGS	:=
CPPFLAGS	+= -MMD
CPPFLAGS	+= -MP
CPPFLAGS	+= $(addprefix -I, $(INC_DIR))

LDFLAGS		:=

RM			:= rm -f
RMDIR		:= rm -rf

DIR_DUP		= mkdir -p $(@D)

BUILD		?= dev

# options

ifeq ($(BUILD), prod)
	CFLAGS		+= -Wall
	CFLAGS		+= -Wextra
	CFLAGS		+= -Werror
	CFLAGS		+= -O2
else ifeq ($(BUILD), dev)
	ASAN		:= 1
	UBSAN		:= 1
	DEBUG		:= 1
	CFLAGS	+= -Wall
	CFLAGS	+= -Wextra
	CFLAGS	+= -Werror
	CFLAGS	+= -Wpedantic
	CFLAGS	+= -Wconversion
	CFLAGS	+= -Wshadow
	CFLAGS	+= -Wno-unused-parameter
	CFLAGS	+= -Wno-unused-function
else
$(error Invalid BUILD='$(BUILD)' (expected 'dev' or 'prod'))
endif

ifeq ($(DEBUG), 1)
	CFLAGS		+= -O0
	CFLAGS		+= -ggdb3
endif

ifeq ($(ASAN), 1)
	CFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

ifeq ($(UBSAN), 1)
	CFLAGS += -fsanitize=undefined
	LDFLAGS += -fsanitize=undefined
endif

# rules

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: %.c
	$(DIR_DUP)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

-include $(DEP)

clean:
	$(RM) $(DEP) $(OBJ)
	$(RMDIR) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
