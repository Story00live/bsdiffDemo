cc = gcc
prom = bsdiffDemo
deps = $(shell find ./ -name "*.h")
src = $(shell find ./ -name "*.c")
obj = $(src:%.c=%.o)

$(prom): $(obj)
	$(cc) -lbz2 -std=c99 -o $(prom) $(obj)

%.o: %.c $(deps)
	$(cc) -lbz2 -std=c99 -c $< -o $@

clean:
	rm -rf $(obj)


# CFLAGS              +=   -O3 -lbz2

# PREFIX              ?=   /usr/local
# INSTALL_PROGRAM     ?=   ${INSTALL} -c -s -m 555
# INSTALL_MAN         ?=   ${INSTALL} -c -m 444

# all:      bsdiff bspatch
# bsdiff:   bsdiff.c
# bspatch:  bspatch.c

# install:
# 		${INSTALL_PROGRAM} bsdiff bspatch ${PREFIX}/bin
# 		.ifndef WITHOUT_MAN
# 		${INSTALL_MAN} bsdiff.1 bspatch.1 ${PREFIX}/man/man1
# 		.endif
