default: build

build:
	@g++ main.cpp Player.cpp Ball.cpp -lncurses -o pong || (echo "\n==> $(tput setaf 1)Make failed.\n$(tput sgr0)"; exit 1)
	@echo "$(tput setaf 2)Make successful.  Start the game using $(tput sgr0)make run$(tput setaf 2) or $(tput sgr0)./pong $(tput sgr0)"

run:
	./pong || true

clean:
	rm pong
