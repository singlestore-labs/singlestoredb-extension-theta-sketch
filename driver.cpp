#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <extension-nowasm.h>
#include <unistd.h>
#include <vector>

double __wasm_export_extension_sketch_get_estimate(ADDR arg, SIZE arg0);
ADDR __wasm_export_extension_sketch_union(ADDR arg, SIZE arg0, ADDR arg1, SIZE arg2);
HANDLE __wasm_export_extension_sketch_handle_init(void);
HANDLE __wasm_export_extension_sketch_handle_clone(HANDLE arg);
int32_t __wasm_export_extension_sketch_handle_destroy(HANDLE arg);
HANDLE __wasm_export_extension_sketch_handle_build_accum(HANDLE arg, ADDR arg0, SIZE arg1);
HANDLE __wasm_export_extension_sketch_handle_union_accum(HANDLE arg, ADDR arg0, SIZE arg1);
HANDLE __wasm_export_extension_sketch_handle_union_merge(HANDLE arg, HANDLE arg0);
HANDLE __wasm_export_extension_sketch_handle_union_copymerge(HANDLE arg, HANDLE arg0);
ADDR __wasm_export_extension_sketch_handle_serialize(HANDLE arg);
HANDLE __wasm_export_extension_sketch_handle_deserialize(ADDR arg, SIZE arg0);
ADDR __wasm_export_extension_sketch_to_string(ADDR arg, SIZE arg0);

HANDLE make_sketch()
{
    HANDLE s = __wasm_export_extension_sketch_handle_init();
    for (int i = 0; i < 1000000; ++i)
    {
        int* ix = (int*) malloc(sizeof(int));
        *ix = i;
        s = __wasm_export_extension_sketch_handle_build_accum(s, (int64_t) ix, sizeof(int));
    }
    return s;
}

int main(int argc, char* argv[])
{
    printf("Accumulating s1...\n");
    HANDLE s1 = make_sketch();

    printf("Accumulating s2...\n");
    HANDLE s2 = make_sketch();

    printf("Accumulating s3...\n");
    HANDLE s3 = make_sketch();

    printf("Accumulating s4...\n");
    HANDLE s4 = make_sketch();

    printf("Accumulating s5...\n");
    HANDLE s5 = make_sketch();

    printf("Accumulating s6...\n");
    HANDLE s6 = make_sketch();

    printf("Merging s1 and s2 -> u1...\n");
    HANDLE u1 = __wasm_export_extension_sketch_handle_union_merge(s1, s2);

    printf("Merging s3 and s4 -> u2...\n");
    HANDLE u2 = __wasm_export_extension_sketch_handle_union_merge(s3, s4);

    printf("Merging s5 and s6 -> u3...\n");
    HANDLE u3 = __wasm_export_extension_sketch_handle_union_merge(s5, s6);

    printf("Serializing u2...\n");
    ADDR u2_a = __wasm_export_extension_sketch_handle_serialize(u2);
    ADDR u2_p = *(ADDR*) ((char*) u2_a);
    SIZE u2_z = *(SIZE*) ((char*) u2_a + 8);

    printf("Accumulating u2 into u1...\n");
    u1 = __wasm_export_extension_sketch_handle_union_accum(u1, u2_p, u2_z);

    printf("Merging u1 and u3 -> u4...\n");
    HANDLE u4 = __wasm_export_extension_sketch_handle_union_merge(u1, u3);

    printf("Serializing u4...\n");
    ADDR u4_a = __wasm_export_extension_sketch_handle_serialize(u4);
    ADDR u4_p = *(ADDR*) ((char*) u4_a);
    SIZE u4_z = *(SIZE*) ((char*) u4_a + 8);

    printf("Deserializing to u5...\n");
    HANDLE u5 = __wasm_export_extension_sketch_handle_deserialize(u4_p, u4_z);

    printf("Accumulating s7...\n");
    HANDLE s7 = make_sketch();

    printf("Cloning s7 -> s8...\n");
    HANDLE s8 = __wasm_export_extension_sketch_handle_clone(s7);

    printf("Cloning u5 -> u6...\n");
    HANDLE u6 = __wasm_export_extension_sketch_handle_clone(u5);

    printf("Copymerge u5 and u6 -> u7...\n");
    HANDLE u7 = __wasm_export_extension_sketch_handle_union_copymerge(u5, u6);

    printf("Accumulating s9...\n");
    HANDLE s9 = make_sketch();

    printf("Copymerge u7 and s9 -> u8...\n");
    HANDLE u8 = __wasm_export_extension_sketch_handle_union_copymerge(u6, s9);

    __wasm_export_extension_sketch_handle_destroy(u5);
    __wasm_export_extension_sketch_handle_destroy(s7);
    __wasm_export_extension_sketch_handle_destroy(s8);
    __wasm_export_extension_sketch_handle_destroy(u6);
    __wasm_export_extension_sketch_handle_destroy(u7);
    __wasm_export_extension_sketch_handle_destroy(s9);
    __wasm_export_extension_sketch_handle_destroy(u8);

    return 0;
}

int main1(int argc, char* argv[])
{
    HANDLE s1 = __wasm_export_extension_sketch_handle_init();
    printf("S1=%ld\n", s1);

#define THING "\x01\x03\x03\x00\x00\x3A\xCC\x93\x00\x01\xBF\xAB\xF4\x6A\x96\x0D"
    size_t size = sizeof(THING);
    char* ptr = (char*) malloc(size);
    memcpy(ptr, THING, size);

    HANDLE u = __wasm_export_extension_sketch_handle_build_accum(s1, (int64_t) ptr, size);
    printf("U=%ld\n", u);

    ADDR a = __wasm_export_extension_sketch_handle_serialize(u);
    ADDR p = *(ADDR*) ((char*) a);
    ADDR z = *(SIZE*) ((char*) a + 8);
    printf("SERIALIZED PTR=%ld, LENGTH=%ld\n", p, z);
    for (int i = 0; i < z; ++i)
    {
        printf("%02X", ((char*) p)[i] & 0xFF);
    }
    printf("\n");

    extension_string_t str;
    a = __wasm_export_extension_sketch_to_string(p, z);
    ADDR sp = *(ADDR*) ((char*) a); 
    ADDR sz = *(ADDR*) ((char*) a + 8); 
    printf("tostring=%.*s", (int) sz, (char*) sp);

    free((void*) p);
    free((void*) sp);

    return 0;
}

