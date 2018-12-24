cc = gcc
prom = application
deps = bsdiff.h bspatch.h				#$(shell find ./ -name "*.h")
src = bsdiff.c bspatch.c main.c			#$(shell find ./ -name "*.c")
obj = $(src:%.c=%.o)

$(prom): $(obj)
	$(cc) -o $(prom) $(obj)

%.o: %.c $(deps)
	$(cc) -c $< -o $@

clean:
	rm -rf $(obj) $(prom)
