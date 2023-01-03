#include <string>
#include <vector>
#include "extension.h"

/// @brief Greets the invoker with a warm welcome
/// @param name Name of the welcomee
/// @param ret The welcome message
void extension_greet(extension_string_t *name, extension_string_t *ret) {
    const auto greeting = std::string("Hello ") + std::string(name->ptr, name->len);
    extension_string_dup(ret, greeting.c_str());
    extension_string_free(name);
}

/// @brief Provides the answer to life, the universe & everything
/// @return 42
int32_t extension_answer_to_life() {
    return 42;
}

/// @brief WASM state retained between function calls
struct WasmState {
    std::vector<uint8_t> _buffer;
    int32_t _idx;
};

static WasmState WASM_STATE = WasmState();

/// @brief Updates the WASM state
/// @param s State to be stored
int32_t extension_set_state(extension_state_t *s) {
    WASM_STATE._buffer = std::vector<uint8_t>(s->buffer.len);
    memcpy(WASM_STATE._buffer.data(), s->buffer.ptr, s->buffer.len);
    WASM_STATE._idx = s->idx;
    return WASM_STATE._idx;
}

/// @brief Returns the current WASM state
/// @param ret0 output buffer
void extension_get_state(extension_state_t *ret) {
    ret->buffer.ptr = WASM_STATE._buffer.data();
    ret->buffer.len = WASM_STATE._buffer.size();
    ret->idx = WASM_STATE._idx;
}