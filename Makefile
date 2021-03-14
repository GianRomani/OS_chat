CC = gcc 
BINS = server client

all: server client

server: server.c linked_list.h
	$(CC) -o server server.c linked_list.h

client : client.c utils.h
	$(CC) -o client client.c utils.h

.phony: clean all

clean:
	rm -rf *.o *~ $(LIBS) $(BINS)