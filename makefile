netpro: *.cpp *.h
	g++ *.cpp -g -std=c++11 -lpthread -o netpro

clean: 
	rm -rf netpro
