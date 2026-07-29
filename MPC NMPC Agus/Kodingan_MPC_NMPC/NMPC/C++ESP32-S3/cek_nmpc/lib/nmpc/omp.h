#ifndef OMP_H_WRAPPER
#define OMP_H_WRAPPER

typedef int omp_nest_lock_t;
typedef int omp_lock_t;

static inline void omp_init_nest_lock(omp_nest_lock_t *lock) { (void)lock; }
static inline void omp_destroy_nest_lock(omp_nest_lock_t *lock) { (void)lock; }
static inline void omp_set_nest_lock(omp_nest_lock_t *lock) { (void)lock; }
static inline void omp_unset_nest_lock(omp_nest_lock_t *lock) { (void)lock; }

static inline int omp_get_max_threads(void) { return 1; }
static inline int omp_get_num_threads(void) { return 1; }
static inline int omp_get_thread_num(void) { return 0; }

#endif
