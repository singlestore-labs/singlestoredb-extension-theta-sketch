#ifdef NOTWASM
#  include "extension-nowasm.h"
#else
#  include "extension.h"
#endif
#include "log.h"
#include "theta_a_not_b.hpp"
#include "theta_intersection.hpp"
#include "theta_sketch.hpp"
#include "theta_union.hpp"
#include "theta_constants.hpp"
#include "MurmurHash3.h"
#include <unistd.h>

using namespace datasketches;

#define NO_HANDLE ((extension_state_t) -1)
#define TO_HND(x_) (reinterpret_cast<extension_state_t>(x_))
#define TO_PTR(x_) (reinterpret_cast<void*>(x_))
#define VALID_HANDLE(s_) (TO_HND(s_) != NO_HANDLE)
#define BAIL(...) { fprintf(stderr, __VA_ARGS__); abort(); }
#define BAIL_IF(cond_, ...) { if (cond_) { BAIL(__VA_ARGS__); } }

using s2_theta_sketch = theta_sketch;
using s2_update_theta_sketch = update_theta_sketch;
using s2_compact_theta_sketch = compact_theta_sketch;
using s2_wrapped_compact_theta_sketch = wrapped_compact_theta_sketch;
using s2_theta_union = theta_union;
using s2_theta_intersection = theta_intersection;

#define theta_sketch                 s2_theta_sketch
#define update_theta_sketch          s2_update_theta_sketch
#define compact_theta_sketch         s2_compact_theta_sketch
#define wrapped_compact_theta_sketch s2_wrapped_compact_theta_sketch
#define theta_union                  s2_theta_union
#define theta_intersection           s2_theta_intersection

static void* theta_sketch_new_default()
{
    return new update_theta_sketch(update_theta_sketch::builder().build());
}

static void* theta_union_new_default()
{
    return new theta_union(theta_union::builder().build());
}

static void* theta_intersection_new_default()
{
    return new theta_intersection;
}

static void update_theta_sketch_delete(void* sketchptr)
{
    if (sketchptr)
    {
        delete static_cast<update_theta_sketch*>(sketchptr);
    }
}

static void theta_sketch_delete(void* sketchptr)
{
    if (sketchptr)
    {
        delete static_cast<theta_sketch*>(sketchptr);
    }
}

static void theta_union_delete(void* unionptr)
{
    if (unionptr)
    {
        delete static_cast<theta_union*>(unionptr);
    }
}

static void theta_intersection_delete(void* interptr)
{
    if (interptr)
    {
        delete static_cast<theta_intersection*>(interptr);
    }
}

static void theta_sketch_update(void* sketchptr, const void* data, unsigned length)
{
    static_cast<update_theta_sketch*>(sketchptr)->update(data, length);
}

static void theta_sketch_update_by_hash(void* sketchptr, const uint64_t hash)
{
    static_cast<update_theta_sketch*>(sketchptr)->update_by_hash(hash);
}

static void theta_data_set_type(void* dataptr, agg_state_type t)
{
    static_cast<theta_data*>(dataptr)->set_type(t);
}

static agg_state_type theta_data_get_type(void* dataptr)
{
    return static_cast<theta_data*>(dataptr)->get_type();
}

static void* theta_union_get_result(void* unionptr)
{
    auto u = static_cast<theta_union*>(unionptr);
    auto sketchptr = new compact_theta_sketch(u->get_result());
    theta_union_delete(u);
    return sketchptr;
}

static void* theta_intersection_get_result(void* interptr)
{
    auto i = static_cast<theta_intersection*>(interptr);
    auto sketchptr = new compact_theta_sketch(i->get_result());
    theta_intersection_delete(i);
    return sketchptr;
}

static void* theta_data_get_result(void* dataptr)
{
    switch (theta_data_get_type(dataptr))
    {
        case UNION:
            return theta_union_get_result(dataptr);

        case INTERSECTION:
            return theta_intersection_get_result(dataptr);

        default:
            return dataptr;
    }
}

static void theta_union_update_with_sketch(void* unionptr, const void* sketchptr)
{
    static_cast<theta_union*>(unionptr)->update(*static_cast<const theta_sketch*>(sketchptr));
}

static void theta_union_update_with_bytes(void* unionptr, extension_list_u8_t* bytes)
{
    static_cast<theta_union*>(unionptr)->update(
        wrapped_compact_theta_sketch::wrap(bytes->ptr, bytes->len));
}

static void theta_intersection_update_with_sketch(void* interptr, const void* sketchptr)
{
    static_cast<theta_intersection*>(interptr)->update(*static_cast<const theta_sketch*>(sketchptr));
}

static void theta_intersection_update_with_bytes(void* interptr, extension_list_u8_t* bytes)
{
    static_cast<theta_intersection*>(interptr)->update(
        wrapped_compact_theta_sketch::wrap(bytes->ptr, bytes->len));
}

static void* theta_sketch_compact(void* sketchptr)
{
    auto s = static_cast<update_theta_sketch*>(sketchptr);
    auto res = new compact_theta_sketch(s->compact());
    update_theta_sketch_delete(s);
    return res;
}

static void theta_sketch_serialize(const void* sketchptr, extension_list_u8_t* output)
{
    unsigned char* bytes;
    size_t size;
    auto s = static_cast<const compact_theta_sketch*>(sketchptr);
    if (!s->serialize2(&bytes, &size))
    {
        BAIL("Failed to serialize theta sketch");
    }
    output->ptr = bytes;
    output->len = size;
}

static void* theta_sketch_deserialize(extension_list_u8_t* bytes)
{
    return new compact_theta_sketch(
        compact_theta_sketch::deserialize(bytes->ptr, bytes->len));
}

static double theta_sketch_get_estimate(const void* sketchptr)
{
    return static_cast<const theta_sketch*>(sketchptr)->get_estimate();
}

static void theta_sketch_to_string(void* sketchptr, extension_string_t *output)
{
    auto str = static_cast<const theta_sketch*>(sketchptr)->to_string();
    output->len = str.length() + 1;
    char* buffer = (char*) malloc(output->len);
    strncpy(buffer, str.c_str(), output->len);
    output->ptr = buffer;
}

static void*
theta_anotb(
    const extension_list_u8_t* buf1,
    const extension_list_u8_t* buf2)
{
    theta_a_not_b a_not_b;
    return new compact_theta_sketch(
        a_not_b.compute(
            wrapped_compact_theta_sketch::wrap(buf1->ptr, buf1->len),
            wrapped_compact_theta_sketch::wrap(buf2->ptr, buf2->len)));
}


extension_state_t extension_sketch_handle_init()
{
    return NO_HANDLE;
}

extension_state_t
extension_sketch_handle_build_accum(
    extension_state_t handle, 
    extension_list_u8_t *input)
{
    if (!input)
    {
        return handle;
    }

    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    theta_sketch_update(state, input->ptr, input->len);
    extension_list_u8_free(input);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_build_accum_emptyisnull(
    extension_state_t handle, 
    extension_list_u8_t *input)
{
    // If input is empty string, no update needed.
    //
    if (!input->len)
    {
        extension_list_u8_free(input);
        input = nullptr;
    }
    return extension_sketch_handle_build_accum(handle, input);
}

extension_state_t
extension_sketch_handle_build_accum_by_hash(
    extension_state_t handle,
    uint64_t input)
{
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    theta_sketch_update_by_hash(state, input);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_build_accum_by_hash_emptyisnull(
    extension_state_t handle,
    uint64_t input)
{
    if (!input)
    {
        return handle;
    }
    return extension_sketch_handle_build_accum_by_hash(handle, input);
}

extension_state_t
extension_sketch_handle_union_accum(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    if (!input)
    {
        return handle;
    }

    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_union_new_default();
        theta_data_set_type(state, UNION);
    }
    theta_union_update_with_bytes(state, input);
    extension_list_u8_free(input);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_union_accum_emptyisnull(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    // If input is empty string, no update needed.
    //
    if (!input->len)
    {
        extension_list_u8_free(input);
        input = nullptr;
    }
    return extension_sketch_handle_union_accum(handle, input);
}

extension_state_t
extension_sketch_handle_intersection_accum(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    if (!input)
    {
        return handle;
    }

    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_intersection_new_default();
        theta_data_set_type(state, INTERSECTION);
    }
    theta_intersection_update_with_bytes(state, input);
    extension_list_u8_free(input);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_intersection_accum_emptyisnull(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    // If input is empty string, no update needed.
    //
    if (!input->len)
    {
        extension_list_u8_free(input);
        input = nullptr;
    }
    return extension_sketch_handle_intersection_accum(handle, input);
}

extension_state_t
extension_sketch_handle_union_merge(
    extension_state_t left,
    extension_state_t right)
{
    if (!VALID_HANDLE(left) && !VALID_HANDLE(right))
    {
        return NO_HANDLE;
    }

    void* u = theta_union_new_default();
    theta_data_set_type(u, UNION);
    if (VALID_HANDLE(left))
    {
        auto lptr = theta_data_get_result(TO_PTR(left));
        theta_union_update_with_sketch(u, lptr);
        theta_sketch_delete(lptr);
    }
    if (VALID_HANDLE(right))
    {
        auto rptr = theta_data_get_result(TO_PTR(right));
        theta_union_update_with_sketch(u, rptr);
        theta_sketch_delete(rptr);
    }

    return TO_HND(u);
}

extension_state_t
extension_sketch_handle_intersection_merge(
    extension_state_t left,
    extension_state_t right)
{
    if (!VALID_HANDLE(left) && !VALID_HANDLE(right))
    {
        return NO_HANDLE;
    }

    void* x = theta_intersection_new_default();
    theta_data_set_type(x, INTERSECTION);
    if (VALID_HANDLE(left))
    {
        auto lptr = theta_data_get_result(TO_PTR(left));
        theta_intersection_update_with_sketch(x, lptr);
        theta_sketch_delete(lptr);
    }
    if (VALID_HANDLE(right))
    {
        auto rptr = theta_data_get_result(TO_PTR(right));
        theta_intersection_update_with_sketch(x, rptr);
        theta_sketch_delete(rptr);
    }

    return TO_HND(x);
}

void
extension_sketch_handle_serialize(
    extension_state_t handle,
    extension_list_u8_t *output)
{
    if (!VALID_HANDLE(handle))
    {
        output->ptr = nullptr;
        output->len = 0;
        return;
    }
    auto state = TO_PTR(handle);
    switch (theta_data_get_type(state))
    {
        case MUTABLE_SKETCH:
            state = theta_sketch_compact(state);
            break;
        default:
            state = theta_data_get_result(state);
            break;
    }
    theta_sketch_serialize(state, output);
    theta_sketch_delete(state);
}

extension_state_t
extension_sketch_handle_deserialize(
    extension_list_u8_t *data)
{
    extension_state_t res;
    if (data->len == 0)
    {
        res = NO_HANDLE;
    }
    else
    {
        auto stateptr = theta_sketch_deserialize(data);
        theta_data_set_type(stateptr, IMMUTABLE_SKETCH);
        res = TO_HND(stateptr);
    }
    extension_list_u8_free(data);
    return res;
}

double extension_sketch_get_estimate(extension_list_u8_t* data)
{
    if (!data)
    {
        extension_list_u8_free(data);
        return 0.0;
    }
    BAIL_IF(data->len < 8, "at least 8 bytes expected, actual %zu", data->len);

    auto sketchptr = theta_sketch_deserialize(data);
    auto estimate = theta_sketch_get_estimate(sketchptr);
    theta_sketch_delete(sketchptr);
    extension_list_u8_free(data);
    return estimate;
}

double extension_sketch_get_estimate_emptyisnull(extension_list_u8_t* data)
{
    if (!data->len)
    {
        extension_list_u8_free(data);
        data = nullptr;
    }
    return extension_sketch_get_estimate(data);
}

void
extension_sketch_union(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* unionptr = theta_union_new_default();
    if (left)
    {
        BAIL_IF(left->len < 8, "at least 8 bytes expected, actual %zu", left->len);
        theta_union_update_with_bytes(unionptr, left);
    }
    if (right)
    {
        BAIL_IF(right->len < 8, "at least 8 bytes expected, actual %zu", right->len);
        theta_union_update_with_bytes(unionptr, right);
    }

    void* sketchptr = theta_union_get_result(unionptr);
    theta_sketch_serialize(sketchptr, output);

    if (left)  extension_list_u8_free(left);
    if (right) extension_list_u8_free(right);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_union_emptyisnull(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    if (!left->len)
    {
        extension_list_u8_free(left);
        left = nullptr;
    }
    if (!right->len)
    {
        extension_list_u8_free(right);
        right = nullptr;
    }
    extension_sketch_union(left, right, output);
}

void
extension_sketch_intersection(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* interptr = theta_intersection_new_default();
    if (left)
    {
        BAIL_IF(left->len < 8, "at least 8 bytes expected, actual %zu", left->len);
        theta_intersection_update_with_bytes(interptr, left);
    }
    if (right)
    {
        BAIL_IF(right->len < 8, "at least 8 bytes expected, actual %zu", right->len);
        theta_intersection_update_with_bytes(interptr, right);
    }

    void* sketchptr = theta_intersection_get_result(interptr);
    theta_sketch_serialize(sketchptr, output);

    if (left)  extension_list_u8_free(left);
    if (right) extension_list_u8_free(right);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_intersection_emptyisnull(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    if (!left->len)
    {
        extension_list_u8_free(left);
        left = nullptr;
    }
    if (!right->len)
    {
        extension_list_u8_free(right);
        right = nullptr;
    }
    extension_sketch_intersection(left, right, output);
}

void
extension_sketch_a_not_b(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    BAIL_IF(!left || !right, "a_not_b must have two valid sketches");
    BAIL_IF(left->len < 8, "at least 8 bytes expected, actual %zu", left->len);
    BAIL_IF(right->len < 8, "at least 8 bytes expected, actual %zu", right->len);

    void* sketchptr = theta_anotb(left, right);
    theta_sketch_serialize(sketchptr, output);

    if (left) extension_list_u8_free(left);
    if (right) extension_list_u8_free(right);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_a_not_b_emptyisnull(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    if (!left->len)
    {
        extension_list_u8_free(left);
        left = nullptr;
    }
    if (!right->len)
    {
        extension_list_u8_free(right);
        right = nullptr;
    }
    extension_sketch_a_not_b(left, right, output);
}

void 
extension_sketch_to_string(
    extension_list_u8_t *data,
    extension_string_t *ret)
{
    if (!data)
    {
        ret->ptr = nullptr;
        ret->len = 0;
        return;
    }

    auto sketchptr = theta_sketch_deserialize(data);
    theta_sketch_to_string(sketchptr, ret);
    theta_sketch_delete(sketchptr);
    extension_list_u8_free(data);
}

void 
extension_sketch_to_string_emptyisnull(
    extension_list_u8_t *data,
    extension_string_t *ret)
{
    if (!data->len)
    {
        extension_list_u8_free(data);
        data = nullptr;
    }
    extension_sketch_to_string(data, ret);
}

uint64_t extension_sketch_hash(extension_list_u8_t *data)
{
    if (!data)
    {
        return 0;
    }
    HashState hashes;
    MurmurHash3_x64_128(data->ptr, data->len, DEFAULT_SEED, hashes);
    extension_list_u8_free(data);
    return (hashes.h1 >> 1);
}

uint64_t extension_sketch_hash_emptyisnull(extension_list_u8_t *data)
{
    if (!data->len)
    {
        extension_list_u8_free(data);
        data = nullptr;
    }
    return extension_sketch_hash(data);
}

