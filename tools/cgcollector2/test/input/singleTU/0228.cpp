// Test to make sure we do not mangle the name of this function wrong again
// According to the LLVM discourse, this has internal linkage and as such both manglings
// GCC's and Clang's are permissable.
extern "C" {
typedef unsigned short int __uint16_t;
static __inline __uint16_t __uint16_identity(__uint16_t __x) { return __x; }
}
