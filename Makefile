CC = gcc
CXXFLAGS = -std=c11 -Wall
LDFLAGS = -lsqlite3 -lcrypto -lssl
CFLAGS=-g -Wall -Werror -UDEBUG

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
clean: clean-server clean-client clean-db
	rm $(wildcard $(OBJDIR)/*.o) $(wildcard *.d)
clean-server:
	rm server
clean-client:
	rm client
clean-db:
	rm secure-chat.db

pems: server-key.pem server-self-cert.pem

server-key.pem:
	openssl genrsa -out server-key.pem

server-self-cert.pem:
	openssl req -x509 -key server-key.pem -out server-self-cert.pem -nodes -subj '/CN=server\.example\.com/'
