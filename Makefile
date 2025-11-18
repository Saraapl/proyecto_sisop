# ================================
# Makefile para Windows (MSYS2/MinGW)
# ================================

# Compilador y banderas
CXX      = g++
CXXFLAGS = -Wall -Wextra -O2 -I./include -D_WIN32_WINNT=0x0600
LDFLAGS  = -static

# Directorios
SRCDIR   = src
OBJDIR   = obj
LIBDIR   = lib
INCDIR   = include
BINDIR   = bin

# Archivos de salida
TARGET   = libProcesoPar.a
TEST_EXE = proceso_par_main.exe
CAT_EXE  = cat.exe

# Archivos fuente y objetos
SOURCES  = $(wildcard $(SRCDIR)/*.cpp)
SOURCES  := $(filter-out $(SRCDIR)/main.cpp $(SRCDIR)/cat.cpp, $(SOURCES))
TEST_SRC = $(SRCDIR)/main.cpp
CAT_SRC  = $(SRCDIR)/cat.cpp

OBJECTS  = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TEST_OBJ = $(OBJDIR)/main.o
CAT_OBJ  = $(OBJDIR)/cat.o

# ================================
# Reglas
# ================================

# Compilación completa
all: directories $(LIBDIR)/$(TARGET) $(BINDIR)/$(TEST_EXE) $(BINDIR)/$(CAT_EXE)

# Crear directorios necesarios
directories:
	@mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR) $(INCDIR)

# Crear la biblioteca estática
$(LIBDIR)/$(TARGET): $(OBJECTS)
	@echo "Creando biblioteca..."
	@ar rcs $@ $^

# Compilar ejecutable de prueba
$(BINDIR)/$(TEST_EXE): $(TEST_OBJ) $(LIBDIR)/$(TARGET)
	@echo "Compilando prueba..."
	@$(CXX) $(TEST_OBJ) -L$(LIBDIR) -lProcesoPar $(LDFLAGS) -o $@

# Compilar utilidad cat
$(BINDIR)/$(CAT_EXE): $(CAT_OBJ)
	@echo "Compilando utilidad cat..."
	@$(CXX) $(CAT_OBJ) $(LDFLAGS) -o $@

# Compilar archivos objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compilando $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	@echo "Limpiando archivos compilados..."
	@rm -rf $(OBJDIR)/*.o $(LIBDIR)/$(TARGET) $(BINDIR)/$(TEST_EXE) $(BINDIR)/$(CAT_EXE)

# Ejecutar el programa de prueba
run: $(BINDIR)/$(TEST_EXE)
	@./$(BINDIR)/$(TEST_EXE)

.PHONY: all clean run directories
