##
## Made by texane <texane@gmail.com>
## 
## Started on Sun Oct 26 16:06:14 2008 texane
## Updated on Sun Oct 26 22:57:16 2008 texane
##



CC	:=	gcc
LD	:=	gcc
CFLAGS	:=	-Wall -O2 -Werror -D_DEBUG=1 -I../src
LFLAGS	:=
RM	:=	rm -f


BIN	:=	eyed

SRCS	:=	../src/main.c		\
		../src/cam.c		\
		../src/bmp.c		\
		../src/transform.c

OBJS	:=	$(SRCS:.c=.o)


all	:	$(BIN)
$(BIN)	:	$(OBJS)
		$(LD) $(LFLAGS) -o $(BIN) $(OBJS)
		sudo chown root $(BIN)
		sudo chmod 4755 $(BIN)
clean	:
		$(RM) $(OBJS)
fclean	:	clean
		$(RM) $(BIN)
re	:	fclean all
%.o	:	%.c
		$(CC) $(CFLAGS) -o $@ -c $<
