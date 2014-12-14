include ../Make.defines

PROGS =	sp cbp

all:	${PROGS}

sp:		sp.o
		${CC} ${CFLAGS} -o $@ sp.o ${LIBS}

cbp:		cbp.o
		${CC} ${CFLAGS} -o $@ cbp.o ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}
