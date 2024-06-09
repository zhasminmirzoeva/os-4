all: Generator Server Client

Generator: Generator.c
	gcc -o Generator Generator.c

Server: Server.c DieWithError.c List.h List.c Book.h Book.c Task.h Task.c IO.h IO.c
	gcc -o Server Server.c DieWithError.c List.c Book.c Task.c IO.c

Client: Client.c DieWithError.c Book.h Book.c Task.h Task.c IO.h IO.c
	gcc -o Client Client.c DieWithError.c Book.c Task.c IO.c
