COMPILADOR=gcc
FLAGS= -g -lm
OBJETOS= main.o parse_entrada.o

# Como se trata de um projeto pequeno, criei uma regra para verificar as modificações individuais em cada arquivo,
# mas uma versão um pouco mais "eficiente" é aprensentada no fim desse arquivo

parse_entrada: main.c parse_entrada.c parse_entrada.h
	$(COMPILADOR) $^ -o $@ $(FLAGS)

#%.o: %.c parse_entrada.h # todos os arquivos dependem do parse_entrada.h
#	$(COMPILADOR) -c $< -o $@ $(FLAGS) 
#
#parse_entrada: $(OBJETOS)
#	$(COMPILADOR) $^ -o $@ $(FLAGS)