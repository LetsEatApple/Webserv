CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98
NAME := webserv
SRC := main.cpp
SRC += Config/Lexer.cpp
SRC += Config/Parser.cpp
SRC += Config/Validator.cpp
SRC += Config/Utils.cpp
SRC += Config/Config.cpp
OBJ := $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	@echo "Linking $@"
	@$(CXX) $(OBJ) -o $@

%.o: %.cpp
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -IConfig -c $< -o $@

clean:
	@echo "Removing objects"
	@rm -f $(OBJ)

fclean: clean
	@echo "Removing binary"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

