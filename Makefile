EXE=fetchmail
SOURCE=$(wildcard *.c)
$(EXE): $(SOURCE)
	cc -Wall -o $(EXE) $(SOURCE) -lssl -g -lcrypto

# Rust
# $(EXE): src/*.rs vendor
# 	cargo build --frozen --offline --release
# 	cp target/release/$(EXE) .

# vendor:
# 	if [ ! -d "vendor/" ]; then \
# 		cargo vendor --locked; \
# 	fi

format:
	clang-format -style=file -i *.c

clean:
	rm -f $(EXE)

all: $(EXE)
