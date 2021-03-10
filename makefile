netpro: *.cpp *.h
	g++ *.cpp -std=c++11 -lpthread -o netpro

clean: 
	rm -rf netpro
