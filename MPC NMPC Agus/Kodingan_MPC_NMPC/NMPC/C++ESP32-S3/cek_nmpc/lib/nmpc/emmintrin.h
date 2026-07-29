#ifndef EMMINTRIN_H_WRAPPER
#define EMMINTRIN_H_WRAPPER

#include <cstring>
#include <cstdint>

typedef struct {
    double d[2];
} __m128d;

typedef struct {
    int32_t i[4];
} __m128i;

static inline __m128d _mm_loadu_pd(const double* p) {
    __m128d r;
    r.d[0] = p[0];
    r.d[1] = p[1];
    return r;
}

static inline void _mm_storeu_pd(double* p, __m128d a) {
    p[0] = a.d[0];
    p[1] = a.d[1];
}

static inline __m128d _mm_set1_pd(double d) {
    __m128d r;
    r.d[0] = d;
    r.d[1] = d;
    return r;
}

static inline __m128d _mm_add_pd(__m128d a, __m128d b) {
    __m128d r;
    r.d[0] = a.d[0] + b.d[0];
    r.d[1] = a.d[1] + b.d[1];
    return r;
}

static inline __m128d _mm_sub_pd(__m128d a, __m128d b) {
    __m128d r;
    r.d[0] = a.d[0] - b.d[0];
    r.d[1] = a.d[1] - b.d[1];
    return r;
}

static inline __m128d _mm_mul_pd(__m128d a, __m128d b) {
    __m128d r;
    r.d[0] = a.d[0] * b.d[0];
    r.d[1] = a.d[1] * b.d[1];
    return r;
}

static inline __m128d _mm_div_pd(__m128d a, __m128d b) {
    __m128d r;
    r.d[0] = a.d[0] / b.d[0];
    r.d[1] = a.d[1] / b.d[1];
    return r;
}

static inline __m128i _mm_loadu_si128(const __m128i* p) {
    __m128i r;
    std::memcpy(&r, p, sizeof(__m128i));
    return r;
}

static inline void _mm_storeu_si128(__m128i* p, __m128i a) {
    std::memcpy(p, &a, sizeof(__m128i));
}

static inline __m128i _mm_set1_epi32(int i) {
    __m128i r;
    r.i[0] = i;
    r.i[1] = i;
    r.i[2] = i;
    r.i[3] = i;
    return r;
}

static inline __m128i _mm_add_epi32(__m128i a, __m128i b) {
    __m128i r;
    r.i[0] = a.i[0] + b.i[0];
    r.i[1] = a.i[1] + b.i[1];
    r.i[2] = a.i[2] + b.i[2];
    r.i[3] = a.i[3] + b.i[3];
    return r;
}

#endif
