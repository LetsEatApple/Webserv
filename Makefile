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

RM = rm -rf

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
Http/response/Multipart.cpp

CXX = c++
CXXFLAGS = -std=c++98 -Wall -Werror -Wextra -MMD -MP

INCLUDES = \
-I./Config/includes \
-I./Reactor \
-I./Http \
-I./Http/request \
-I./Http/response
DEBUG ?= 0

CPPFLAGS = $(INCLUDES)

ifeq ($(DEBUG),1)
	CPPFLAGS += -DDEBUG
endif

OBJ_DIR = obj
OBJS = $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

GREEN = \033[0;32m
YELLOW = \033[0;33m
RESET = \033[0m

all: $(NAME)

debug:
	@$(MAKE) DEBUG=1 all

${OBJ_DIR}/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@
	@echo "$(YELLOW) Compiling $<$(RESET)"

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "$(GREEN) Executable $(NAME) created.$(RESET)"

clean:
	@echo "$(YELLOW)Cleaning object files...$(RESET)"
	@${RM} $(OBJ_DIR)

fclean: clean
	@echo "$(YELLOW)Removing executable ... $(RESET)"
	@${RM} $(NAME)
	@echo "$(GREEN)All clean. $(RESET)"

re: fclean all

.PHONY: all debug clean fclean re

-include $(DEPS)