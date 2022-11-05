OUTPUT ?= .
LIBNAME ?= libmtar
OBJ ?= obj
SRC := src

CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := ${CFLAGS} -I.
DESTDIR ?= /usr/local/lib

OBJS := $(OBJ)/tar.o

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
	cp $(OUTPUT)/$(LIBNAME).a $(DESTDIR)

clean:
	rm -f $(OBJ)/*.o
	rm -f $(OUTPUT)/$(LIBNAME).a

uninstall:
	@echo -- Removing $(LIBNAME).a
	rm -f $(DESTDIR)/$(LIBNAME).a