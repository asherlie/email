CXX=gcc
all: sn
sn:
	$(CXX) send.c email_notifier.c -lcurl -o sn -Wall -Wextra -pedantic

clean:
	rm -f sn
