CXX=gcc
all: sn
sn:
	$(CXX) send.c email_notifier.c -lcurl -o sn -Wall -Wextra -Wpedantic -O3

clean:
	rm -f sn
