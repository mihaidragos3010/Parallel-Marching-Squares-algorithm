build: tema1_par.cpp
	g++ tema1_par.cpp helpers.cpp marchingSquares.cpp -o tema1_par -lm -lpthread -Wall -Wextra -g
clean:
	rm -rf tema1 tema1_par