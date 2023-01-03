all: clean release export

.PHONY: debug
debug: DBGFLAGS = -g
debug: extension.wasm

.PHONY: release
release: extension.wasm

extension.wasm: gen
	clang++ \
		${DBGFLAGS} \
		-fno-exceptions \
 		--target=wasm32-unknown-wasi \
 		-mexec-model=reactor \
 		-I. \
 		-Icommon \
 		-Itheta-sketch \
 		-o extension.wasm \
 		extension.cpp extension_impl.cpp

.PHONY: gen
gen:
	wit-bindgen c --export extension.wit
	# temporary: https://github.com/bytecodealliance/wit-bindgen/issues/290
	sed "s:canonical_abi_realloc(NULL, 0, 1, ret->len:\(char \*\)canonical_abi_realloc(NULL, 0, 1, ret->len:g" extension.c >extension.cpp
	rm extension.c

.PHONY: export
export:
	./create_loader.sh

.PHONY: clean
clean:
	@rm -f extension.wasm

