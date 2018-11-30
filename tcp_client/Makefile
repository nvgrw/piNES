.PHONY: all controller

all: bin/controller

bin/controller: src/controller.c
	mkdir -p bin && \
	gcc -lpigpio -lpthread -o bin/controller src/controller.c
