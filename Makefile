# UFCA Runner
#
#   make          compila para bin/ufca-runner
#   make run      compila e executa (a partir da raiz do projeto, que é onde
#                 os caminhos de assets/ são procurados)
#   make clean    apaga os arquivos gerados
#
# Opções:
#   make NO_AUDIO=1   compila sem áudio (para máquina sem OpenAL instalado)
#   make DEBUG=1      compila com símbolos de depuração, sem otimização

TARGET  := ufca-runner
BIN_DIR := bin
SRC     := src/main.cpp
BIN     := $(BIN_DIR)/$(TARGET)

CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Iinclude

ifdef DEBUG
    CXXFLAGS += -g -O0
else
    CXXFLAGS += -O2
endif

# Detecta a plataforma para escolher as bibliotecas certas.
ifeq ($(OS),Windows_NT)
    # Windows (MinGW + freeglut): áudio pelo MMSystem (-lwinmm)
    PLATAFORMA := Windows
    LDLIBS     := -lfreeglut -lopengl32 -lglu32 -lwinmm
    BIN        := $(BIN_DIR)/$(TARGET).exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PLATAFORMA := macOS
        LDLIBS     := -framework GLUT -framework OpenGL -framework OpenAL
    else
        PLATAFORMA := Linux
        LDLIBS     := -lglut -lGLU -lGL -lopenal -lm
    endif
endif

ifdef NO_AUDIO
    CXXFLAGS += -DAUDIO_DISABLED
    # Remove a biblioteca de áudio da linkagem quando o som está desligado
    LDLIBS := $(filter-out -lopenal -lwinmm,$(LDLIBS))
    LDLIBS := $(filter-out -framework OpenAL,$(LDLIBS))
endif

HEADERS := include/obj_loader.h include/texture_loader.h include/audio.h

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC) $(HEADERS) | $(BIN_DIR)
	@echo "Compilando para $(PLATAFORMA)..."
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDLIBS)
	@echo "Pronto: $@"

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Executa a partir da raiz do projeto: os assets são carregados por caminho
# relativo ("assets/..."), então rodar de outra pasta não acha as texturas.
run: $(BIN)
	./$(BIN)

clean:
	@rm -rf $(BIN_DIR)
	@echo "Arquivos de compilacao removidos."
