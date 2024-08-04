CC     = c89
CFLAGS = -Wall -Wextra -Werror -ansi -O3

wol: wol.c
	$(CC) $(CFLAGS) $< -o $@ 

