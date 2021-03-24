all:
	gcc -o jackdiff jackdiff.c rtqueue.c -ljack -lpthread -lm
