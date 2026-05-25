CC = gcc
CFLAGS = -Wall -Wextra -Isrc
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj

# Lista de .c do projeto
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/url.c  \
       $(SRC_DIR)/net.c  \
       $(SRC_DIR)/ftp.c

# Converter .c → .o
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Nome do executável final
TARGET = ftp_client

# ============================================================

# Regra principal
all: $(TARGET)

# Como construir o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Regra para compilar cada .c para .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# ============================================================
# Utilidade
# ============================================================
# Comandos úteis:
# make        → compila tudo
# make clean  → apaga obj/ e binário
# ============================================================