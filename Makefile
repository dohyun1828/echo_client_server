all : echo_server echo_client

echo_server: echo_server.o
	g++ -o echo_server echo_server.o -lpthread
	
echo_client: echo_client.o
	g++ -o echo_client echo_client.o -lpthread
	
clean:
	rm -f echo_client echo_server *.o
