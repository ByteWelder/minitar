OUTPUT ?= .
LIBNAME ?= libmtar
OBJ ?= obj
SRC := src

CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := ${CFLAGS} -I. -D_IN_MINITAR
DESTDIR ?= /usr/local

OBJS := $(OBJ)/tar.o \
		$(OBJ)/util.o

build: $(OBJS)
	@echo -- Creating $(LIBNAME).a
	@mkdir -p $(OUTPUT)
	$(AR) rcs $(OUTPUT)/$(LIBNAME).a $(OBJS)

$(OBJ)/%.o: $(SRC)/%.c
	@echo -- Compiling $^
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $^

install:
	@echo -- Installing $(LIBNAME).a
	@mkdir -p $(DESTDIR)
	cp $(OUTPUT)/$(LIBNAME).a $(DESTDIR)/lib
	cp ./minitar.h $(DESTDIR)/include

clean:
	rm -f $(OBJ)/*.o
	rm -f $(OUTPUT)/$(LIBNAME).a

uninstall:
	@echo -- Removing $(LIBNAME).a
	rm -f $(DESTDIR)/lib/$(LIBNAME).a
	rm -f $(DESTDIR)/include/minitar.h