%.o:%.c
	gcc -pthread $< -o $@

clean:
	rm *.o 
