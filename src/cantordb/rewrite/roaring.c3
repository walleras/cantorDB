module cantordb;

import std::core::mem::alloc;

// Minimal CRoaring bindings. CRoaring itself is built from third_party/roaring.c by project.json.
// Bitmap is an opaque stand-in for roaring_bitmap_t; always used through a pointer.
alias Bitmap = void;

extern fn Bitmap* roaring_bitmap_create_with_capacity(uint cap);
extern fn void roaring_bitmap_free(Bitmap* r);
extern fn void roaring_bitmap_add(Bitmap* r, uint x);
extern fn void roaring_bitmap_remove(Bitmap* r, uint x);
extern fn bool roaring_bitmap_contains(Bitmap* r, uint val);
extern fn ulong roaring_bitmap_get_cardinality(Bitmap* r);
extern fn Bitmap* roaring_bitmap_of_ptr(usz n, uint* vals);
extern fn void roaring_bitmap_and_inplace(Bitmap* r1, Bitmap* r2);
extern fn Bitmap* roaring_bitmap_or(Bitmap* r1, Bitmap* r2);
extern fn Bitmap* roaring_bitmap_and(Bitmap* r1, Bitmap* r2);
extern fn Bitmap* roaring_bitmap_andnot(Bitmap* r1, Bitmap* r2);
extern fn Bitmap* roaring_bitmap_xor(Bitmap* r1, Bitmap* r2);
extern fn bool roaring_bitmap_intersect(Bitmap* r1, Bitmap* r2);
extern fn bool roaring_bitmap_equals(Bitmap* r1, Bitmap* r2);
extern fn bool roaring_bitmap_is_subset(Bitmap* r1, Bitmap* r2);
extern fn bool roaring_bitmap_is_strict_subset(Bitmap* r1, Bitmap* r2);
extern fn void roaring_bitmap_to_uint32_array(Bitmap* r, uint* ans);

fn Bitmap* new_bitmap() => roaring_bitmap_create_with_capacity(0);

// All ids in the bitmap in ascending order. Empty bitmaps give an empty slice
// (nothing to free); otherwise free with alloc::free(heap, arr.ptr).
fn uint[] bitmap_to_array(Bitmap* b, Allocator heap)
{
	usz n = (usz)roaring_bitmap_get_cardinality(b);
	if (n == 0) return {};
	uint[]? arr = alloc::new_array_try(heap, uint, n);
	if (catch arr) return {};
	roaring_bitmap_to_uint32_array(b, arr.ptr);
	return arr;
}
