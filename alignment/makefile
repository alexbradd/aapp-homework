all: alignment max_cost

alignment: alignment.c
	gcc -o $@ -g -DMAIN $^

max_cost: alignment.c max_cost.c
	gcc -o $@ -g $^

clean:
	rm alignment max_cost
