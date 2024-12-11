CC = gcc
SRCS = $(wildcard *.c)
BINS = main
OBJS = $(SRCS:.c=.o)
FLAGS = -Wall -g3

all: ${BINS}

${BINS}: $(OBJS) 
	${CC} ${FLAGS}  -o ${BINS} ${OBJS}  

%.o: %.cpp %.h
	${CC} ${FLAGS} -c $< -o $@

clean:
	rm -rvf *.o ${BINS}
