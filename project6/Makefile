.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.cpp)
# OBJS:=$(SRCS:.cpp=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o
STATIC_LIB:=$(LIBS)libbpt.a

# Include more files if you write another source file.
SRCS_FOR_LIB:= \
	$(SRCDIR)bpt.cpp \
	$(SRCDIR)buffer.cpp \
	$(SRCDIR)file.cpp \
	$(SRCDIR)log.cpp \
	$(SRCDIR)trx.cpp \
	$(SRCDIR)lock.cpp \
	$(SRCDIR)db.cpp

OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CXXFLAGS+= -g -std=c++17 -pthread -fPIC -I $(INC)

TARGET=main

all: $(TARGET)

$(TARGET): $(TARGET_OBJ) $(STATIC_LIB)
	$(CC) $(CXXFLAGS) $< -o $@ -L $(LIBS) -lbpt

%o: %.cpp
	$(CC) $(CXXFLAGS) $^ -c -o $@

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

$(STATIC_LIB): $(OBJS_FOR_LIB)
	ar cr $@ $^
