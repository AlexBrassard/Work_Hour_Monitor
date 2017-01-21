###############################
#
# Work Hour Monitor's Makefile.
#
###############################


# GCC compile flags

GNUCC = gcc
GNUCFLAGS = -g3 -ggdb -O2 -std=c99 -D_POSIX_C_SOURCE=200809L -Wno-format -Wall \
            -Wextra -Wstrict-prototypes -Wno-unused-variable -Wno-unused-parameter \
            -pedantic -fsanitize=signed-integer-overflow



# Other compiler goes here.

${CC} = ${GNUCC}
CFLAGS = ${GNUCFLAGS}


OBJECTS = whm_config.o whm_gen_utils.o whm_sheet.o whm_mem_utils.o whm_main.o
HEADERS = whm.h whm_error.h whm_sysdep.h

PROGNAME = whm


${PROGNAME} : ${OBJECTS} ${HEADERS}
	${CC} ${CFLAGS} ${OBJECTS} -o ${PROGNAME}

whm_config.o : whm_config.c ${HEADERS}
	${CC} ${CFLAGS} -c whm_config.c 

whm_gen_utils.o : whm_gen_utils.c ${HEADERS}
	${CC} ${CFLAGS} -c whm_gen_utils.c

whm_sheet.o : whm_sheet.c ${HEADERS}
	${CC} ${CFLAGS} -c whm_sheet.c 

whm_mem_utils.o : whm_mem_utils.c ${HEADERS}
	${CC} ${CFLAGS} -c whm_mem_utils.c


.PHONY : clean
clean :
	rm -f *.o PROGNAME
