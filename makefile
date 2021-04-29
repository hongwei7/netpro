netpro: *.cpp *.h
	g++ *.cpp -g  -std=c++11 -lpthread -lmysqlclient -o netpro

clean: 
	rm -rf netpro
