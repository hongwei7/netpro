netpro: *.cpp *.h
	g++ *.cpp -g  -std=c++11 -lpthread -Wl,--no-whole-archive -lmysqlclient -o netpro

clean: 
	rm -rf netpro
