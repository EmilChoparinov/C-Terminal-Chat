CC = gcc
CXXFLAGS = -std=c11 -Wall
LDFLAGS = 
CFLAGS=-g -Wall -Werror -UDEBUG
LDLIBS=-lsqlite3 -lcrypto -lssl

EXT = .c
SRCDIR = src
OBJDIR = obj

# entire source
SRC = $(wildcard $(SRCDIR)/*$(EXT))

all: server client

# build specific sources
SERVER_SRC = $(filter-out $(wildcard $(SRCDIR)/client*.c), $(SRC))
SERVER_OBJ =  $(SERVER_SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)

CLIENT_SRC = $(filter-out $(wildcard $(SRCDIR)/server*.c), $(SRC))
CLIENT_OBJ =  $(CLIENT_SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)

server: $(SERVER_OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

client: $(CLIENT_OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# dep rule
%.d: $(SRCDIR)/%$(EXT)
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$@

-include $(DEP)

# rule for .o files and .c to combine with .h
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean
clean: clean-server clean-client
	rm $(wildcard $(OBJDIR)/*.o) $(wildcard *.d)
clean-server:
	rm server
clean-client:
	rm client