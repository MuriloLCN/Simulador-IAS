COMPILADOR=gcc
FLAGS= -g -lm
OBJETOS= main.o ias.o

# Como se trata de um projeto pequeno, criei uma regra para verificar as modificações individuais em cada arquivo,
# mas uma versão um pouco mais "eficiente" é aprensentada no fim desse arquivo

ias: main.c ias.c ias.h
	$(COMPILADOR) $^ -o $@ $(FLAGS)

#%.o: %.c ias.h # todos os arquivos dependem do ias.h
#	$(COMPILADOR) -c $< -o $@ $(FLAGS) 
#
#ias: $(OBJETOS)
#	$(COMPILADOR) $^ -o $@ $(FLAGS)