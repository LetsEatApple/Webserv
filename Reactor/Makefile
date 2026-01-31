# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/08/25 12:32:34 by lea               #+#    #+#              #
#    Updated: 2026/01/31 15:57:52 by lhagemos         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

SRC = ../main.cpp \
AEventHandler.cpp \
handler.cpp \
reactor.cpp \
server.cpp \
../Config/Config.cpp \
../Config/Lexer.cpp \
../Config/Parser.cpp \
../Config/Utils.cpp \
../Config/Validator.cpp \
../Http/request/Headers.cpp \
../Http/request/Request.cpp \
../Http/response/Response.cpp \
../Http/router/Router.cpp \
../Http/routes/routes.cpp \

CPP = c++
CPPFLAGS = -std=c++98 -Wall -Werror -Wextra

OBJS = $(SRC:.cpp=.o)

GREEN = \033[0;32m
YELLOW = \033[0;33m
RESET = \033[0m

all: $(NAME)


%.o: %.cpp
	@$(CPP) $(CPPFLAGS) -c $< -o $@
	@echo "$(YELLOW) Compiling ... $(RESET)"

$(NAME): $(OBJS)
	@$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN) Executable $(NAME) created.$(RESET)"

clean:
	@echo "$(YELLOW)Cleaning object files...$(RESET)"
	@rm -rf $(OBJS)

fclean: clean
	@echo "$(YELLOW)Removing executable ... $(RESET)"
	@rm -rf $(NAME)
	@echo "$(GREEN)All clean. $(RESET)"

re: fclean all

.PHONY: all clean fclean re