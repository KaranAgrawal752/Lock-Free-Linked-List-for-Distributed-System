all:
	g++  -pthread lock_free_linked_list.cpp  main.cpp -o program -latomic
	./program


clean:
	rm -f  program  *.o
