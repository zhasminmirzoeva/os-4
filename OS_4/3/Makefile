all: Generator Server Worker Observer

Generator: Generator.c
	gcc -o Generator Generator.c

Server: Server.c DieWithError.c List.h List.c Book.h Book.c Task.h Task.c IO.h IO.c
	gcc -o Server Server.c DieWithError.c List.c Book.c Task.c IO.c

Worker: Worker.c DieWithError.c Book.h Book.c Task.h Task.c IO.h IO.c
	gcc -o Worker Worker.c DieWithError.c Book.c Task.c IO.c

Observer:  Observer.c DieWithError.c IO.h IO.c
	gcc -o Observer Observer.c DieWithError.c IO.c
