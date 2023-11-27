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

#define MAX_PAGE_SIZE 8192

struct page
{
    struct page* next;
    uint64_t data[MAX_PAGE_SIZE];
};

page* free_pages = nullptr;


template <class T> class s2alloc {
public:
  typedef T                 value_type;
  typedef value_type*       pointer;
  typedef const value_type* const_pointer;
  typedef value_type&       reference;
  typedef const value_type& const_reference;
  typedef std::size_t       size_type;
  typedef std::ptrdiff_t    difference_type;

  template <class U>
  struct rebind { typedef s2alloc<U> other; };

  s2alloc() = default;
  template <class U>
  s2alloc(const s2alloc<U>&) {}

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const {
    return &x;
  }

  pointer allocate(size_type n, const_pointer = 0) {
      /*
    if (n > MAX_PAGE_SIZE)
    {
        abort();
    }

    page* p = free_pages;
    if (!p)
    {
        p = (page*) malloc(sizeof(page));
    }
    else
    {
        free_pages = p->next;
        p->next = nullptr;
    }
    */

    void* p = malloc(n * sizeof(T));
    //printf("MALLOC %zu: %p\n", n, p);
    //return static_cast<pointer>((T*) &p->data[0]);
    return static_cast<pointer>(p);
  }

  void deallocate(pointer p, size_type) {
      //printf("FREE %p\n", p);
      if (p)
      {
          //page* pg = (page*) ((char*) p - sizeof(uintptr_t));
          free(p);
          //pg->next = free_pages;
          //free_pages = pg;
      }
  }

  size_type max_size() const {
    return static_cast<size_type>(-1) / sizeof(T);
  }

  template<typename... Args>
  void construct(pointer p, Args&&... args) {
    new(p) value_type(std::forward<Args>(args)...);
  }
  void destroy(pointer p) { p->~value_type(); }
};

//using s2_theta_sketch = theta_sketch_alloc<s2alloc<uint64_t>>;
//using s2_update_theta_sketch = update_theta_sketch_alloc<s2alloc<uint64_t>>;
//using s2_compact_theta_sketch = compact_theta_sketch_alloc<s2alloc<uint64_t>>;
//using s2_theta_union = theta_union_alloc<s2alloc<uint64_t>>;
//using s2_theta_intersection = theta_intersection_alloc<s2alloc<uint64_t>>;
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

static void theta_sketch_update_raw(void* sketchptr, const uint64_t hash)
{
    static_cast<update_theta_sketch*>(sketchptr)->update_raw(hash);
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
        abort();
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

static void theta_sketch_to_string(void* sketchptr, extension_string_t *ret0)
{
    auto str = static_cast<const theta_sketch*>(sketchptr)->to_string();
    ret0->len = str.length() + 1;
    char* buffer = (char*) malloc(ret0->len);
    strncpy(buffer, str.c_str(), ret0->len);
    ret0->ptr = buffer;
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
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    theta_sketch_update(state, input->ptr, input->len);
    free(input->ptr);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_build_accum_raw(
    extension_state_t handle,
    uint64_t input)
{
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    theta_sketch_update_raw(state, input);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_union_accum(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_union_new_default();
        theta_data_set_type(state, UNION);
    }
    theta_union_update_with_bytes(state, input);
    free(input->ptr);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_intersection_accum(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_intersection_new_default();
        theta_data_set_type(state, INTERSECTION);
    }
    theta_intersection_update_with_bytes(state, input);
    free(input->ptr);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_union_merge(
    extension_state_t left,
    extension_state_t right)
{
    if (!VALID_HANDLE(left) && VALID_HANDLE(right))
    {
        return right;
    }
    if (VALID_HANDLE(left) && !VALID_HANDLE(right))
    {
        return left;
    }

    void* u = nullptr;
    void* ops[2] = { 0 };
    if (theta_data_get_type(TO_PTR(left)) == UNION)
    {
        u = TO_PTR(left);
        ops[0] = TO_PTR(right);
    }
    else if (theta_data_get_type(TO_PTR(right)) == UNION)
    {
        u = TO_PTR(right);
        ops[0] = TO_PTR(left);
    }
    else
    {
        u = theta_union_new_default();
        theta_data_set_type(u, UNION);
        ops[0] = TO_PTR(left);
        ops[1] = TO_PTR(right);
    }

    for (int i = 0; i < 2; ++i)
    {
        if (!ops[i])
        {
            continue;
        }
        if (theta_data_get_type(ops[i]) == UNION)
        {
            ops[i] = theta_union_get_result(ops[i]);
        }
        else if (theta_data_get_type(ops[i]) == INTERSECTION)
        {
            ops[i] = theta_intersection_get_result(ops[i]);
        }
        theta_union_update_with_sketch(u, ops[i]);
        theta_sketch_delete(ops[i]);
    }

    return TO_HND(u);
}

extension_state_t
extension_sketch_handle_intersection_merge(
    extension_state_t left,
    extension_state_t right)
{
    if (!VALID_HANDLE(left) && VALID_HANDLE(right))
    {
        theta_sketch_delete(TO_PTR(right));
        return NO_HANDLE;
    }
    if (VALID_HANDLE(left) && !VALID_HANDLE(right))
    {
        theta_sketch_delete(TO_PTR(left));
        return NO_HANDLE;
    }

    void* x = nullptr;
    void* ops[2] = { 0 };
    if (theta_data_get_type(TO_PTR(left)) == INTERSECTION)
    {
        x = TO_PTR(left);
        ops[0] = TO_PTR(right);
    }
    else if (theta_data_get_type(TO_PTR(right)) == INTERSECTION)
    {
        x = TO_PTR(right);
        ops[0] = TO_PTR(left);
    }
    else
    {
        x = theta_intersection_new_default();
        theta_data_set_type(x, INTERSECTION);
        ops[0] = TO_PTR(left);
        ops[1] = TO_PTR(right);
    }

    for (int i = 0; i < 2; ++i)
    {
        if (!ops[i])
        {
            continue;
        }
        if (theta_data_get_type(ops[i]) == INTERSECTION)
        {
            ops[i] = theta_intersection_get_result(ops[i]);
        }
        else if (theta_data_get_type(ops[i]) == UNION)
        {
            ops[i] = theta_union_get_result(ops[i]);
        }
        theta_intersection_update_with_sketch(x, ops[i]);
        theta_sketch_delete(ops[i]);
    }

    return TO_HND(x);
}

void
extension_sketch_handle_serialize(
    extension_state_t handle,
    extension_list_u8_t *output)
{
    auto state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    switch (theta_data_get_type(state))
    {
        case MUTABLE_SKETCH:
            state = theta_sketch_compact(state);
            break;
        case UNION:
            state = theta_union_get_result(state);
            break;
        case INTERSECTION:
            state = theta_intersection_get_result(state);
            break;
        default:
            break;
    }
    theta_sketch_serialize(state, output);
    theta_sketch_delete(state);
}

extension_state_t
extension_sketch_handle_deserialize(
    extension_list_u8_t *data)
{
    auto stateptr = theta_sketch_deserialize(data);
    theta_data_set_type(stateptr, IMMUTABLE_SKETCH);
    free(data->ptr);
    return TO_HND(stateptr);
}

double extension_sketch_get_estimate(extension_list_u8_t* data)
{
    auto sketchptr = theta_sketch_deserialize(data);
    auto estimate = theta_sketch_get_estimate(sketchptr);
    theta_sketch_delete(sketchptr);
    free(data->ptr);
    return estimate;
}

void
extension_sketch_union(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* unionptr = theta_union_new_default();
    theta_union_update_with_bytes(unionptr, left);
    theta_union_update_with_bytes(unionptr, right);

    void* sketchptr = theta_union_get_result(unionptr);
    theta_sketch_serialize(sketchptr, output);

    free(left->ptr);
    free(right->ptr);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_intersection(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* interptr = theta_intersection_new_default();
    theta_intersection_update_with_bytes(interptr, left);
    theta_intersection_update_with_bytes(interptr, right);

    void* sketchptr = theta_intersection_get_result(interptr);
    theta_sketch_serialize(sketchptr, output);

    free(left->ptr);
    free(right->ptr);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_a_not_b(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* sketchptr = theta_anotb(left, right);
    free(left->ptr);
    free(right->ptr);
    theta_sketch_serialize(sketchptr, output);
    theta_sketch_delete(sketchptr);
}

void 
extension_sketch_to_string(
    extension_list_u8_t *data,
    extension_string_t *ret)
{
    auto sketchptr = theta_sketch_deserialize(data);
    theta_sketch_to_string(sketchptr, ret);
    free(data->ptr);
    theta_sketch_delete(sketchptr);
}

uint64_t extension_sketch_hash(extension_list_u8_t *data)
{
    HashState hashes;
    MurmurHash3_x64_128(data->ptr, data->len, DEFAULT_SEED, hashes);
    free(data->ptr);
    return (hashes.h1 >> 1);
}

