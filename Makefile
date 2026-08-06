listener: listener.c
		gcc listener.c -o listener -lcurl

clean:
		rm -f listener
