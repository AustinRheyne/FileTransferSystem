server: server.c client.c
	gcc server.c -o server
	gcc client.c -o client
	
clean:
	rm -f *.o server
	rm -f *.o client
