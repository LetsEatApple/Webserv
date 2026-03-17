# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: lhagemos <lhagemos@student.42berlin.de>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/08/25 12:32:34 by lea               #+#    #+#              #
#    Updated: 2026/02/14 19:38:03 by lhagemos         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

MAKEFLAGS += --no-print-directory

NAME = webserv

SRC = main.cpp \
Reactor/AEventHandler.cpp \
Reactor/handler.cpp \
Reactor/reactor.cpp \
Reactor/server.cpp \
Reactor/Logger.cpp \
Reactor/routing.cpp \
Reactor/Cgi.cpp \
Config/src/Config.cpp \
Config/src/Lexer.cpp \
Config/src/Parser.cpp \
Config/src/Utils.cpp \
Config/src/BlockValidators.cpp \
Config/src/DirectiveValidators.cpp \
Config/src/ValidationHelpers.cpp \
Config/src/ListenParser.cpp \
Config/src/ConfigPrinter.cpp \
Config/src/DirectiveApplier.cpp \
Http/request/Headers.cpp \
Http/request/Request.cpp \
Http/response/Response.cpp \
Http/response/ResponseDispatcher.cpp \
Http/response/MimeTypes.cpp \

CPP = c++
CPPFLAGS = -std=c++98 -Wall -Werror -Wextra
INCLUDES = \
-I./Config/includes \
-I./Reactor \
-I./Http \
-I./Http/request \
-I./Http/response
DEBUG ?= 0

ifeq ($(DEBUG),1)
	CPPFLAGS += -DDEBUG
endif

OBJS = $(SRC:.cpp=.o)

GREEN = \033[0;32m
YELLOW = \033[0;33m
RESET = \033[0m

all: $(NAME)

debug:
	@$(MAKE) DEBUG=1 all


%.o: %.cpp
	@$(CPP) $(CPPFLAGS) $(INCLUDES) -c $< -o $@
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

.PHONY: all debug clean fclean re