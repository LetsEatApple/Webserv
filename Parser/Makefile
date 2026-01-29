CC := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -g
NAME := config
SRC := main.cpp Lexer.cpp Parser.cpp Validator.cpp Utils.cpp Config.cpp
OBJ := $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	@echo "Linking $@"
	@$(CC) $(OBJ) -o $@

%.o: %.cpp
	@echo "Compiling $<"
	@$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Removing objects"
	@rm -f $(OBJ)

fclean: clean
	@echo "Removing binary"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

