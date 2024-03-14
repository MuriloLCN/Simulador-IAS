COMPILADOR=gcc
FLAGS= -g -lm
OBJETOS= main.o parse_entrada.o

parse_entrada: main.c parse_entrada.c parse_entrada.h
	$(COMPILADOR) $^ -o $@ $(FLAGS)
