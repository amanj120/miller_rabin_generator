prime: prime.c
	clang-format -i --style="{IndentWidth: 4, TabWidth: 4, UseTab: "Always"}" prime.c
	gcc -o prime -O3 prime.c

clean: 
	yes | rm prime
