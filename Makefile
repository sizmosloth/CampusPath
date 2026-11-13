.PHONY: all cli server test clean run-cli run-server help

# Default target: compile both CLI and HTTP server
all: cli server

cli:
	$(MAKE) -C backend all

server:
	$(MAKE) -C backend server

test:
	$(MAKE) -C backend test

clean:
	$(MAKE) -C backend clean

run-cli: cli
	./backend/bin/campuspath

run-server: server
	./backend/bin/campuspath_server

help:
	@echo "=================================================="
	@echo "  CampusPath - Build & Execution Targets"
	@echo "=================================================="
	@echo "  make            - Build CLI and HTTP API server"
	@echo "  make cli        - Build terminal CLI application"
	@echo "  make server     - Build HTTP API & Web server"
	@echo "  make test       - Run all 3 ADSA test suites (88/88 tests)"
	@echo "  make run-cli    - Launch interactive terminal navigation"
	@echo "  make run-server - Launch C++ server on http://127.0.0.1:8080"
	@echo "  make clean      - Clean all build artifacts"
