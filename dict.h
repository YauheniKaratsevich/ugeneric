#ifndef UDICT_H__
#define UDICT_H__

#include "generic.h"
#include "void.h"
#include "vector.h"

typedef enum {
    UDICT_BACKEND_DEFAULT,
    UDICT_BACKEND_BST_PLAIN,
    UDICT_BACKEND_BST_RB,
    //UDICT_BACKEND_BST_SPLAY,
    UDICT_BACKEND_HTBL,
    UDICT_BACKENDS_COUNT, // keep it last
} udict_backend_t;

void libugeneric_udict_set_default_backend(udict_backend_t backend);
udict_backend_t libugeneric_udict_get_default_backend(void);

typedef void       (*f_udict_take_data_ownership)(void *d);
typedef void       (*f_udict_drop_data_ownership)(void *d);
typedef void       (*f_udict_clear)(void *d);
typedef void       (*f_udict_put)(void *d, ugeneric_t k, ugeneric_t v);
typedef ugeneric_t (*f_udict_get)(const void *d, ugeneric_t k, ugeneric_t vdef);
typedef ugeneric_t (*f_udict_pop)(void *d, ugeneric_t k, ugeneric_t vdef);
typedef bool       (*f_udict_has_key)(const void *d, ugeneric_t k);
typedef size_t     (*f_udict_get_size)(const void *d);
typedef bool       (*f_udict_is_empty)(const void *d);
typedef void       (*f_udict_serialize)(const void *d, ubuffer_t *buf);
typedef char      *(*f_udict_as_str)(const void *d);
typedef int        (*f_udict_print)(const void *d);
typedef int        (*f_udict_fprint)(const void *d, FILE *out);
typedef uvoid_handlers_t *(*f_udict_get_void_handlers)(void *d);
typedef uvector_t *(*f_udict_get_items)(const void *d, udict_items_kind_t kind);

typedef ugeneric_kv_t (*f_udict_iterator_get_next)(void *di);
typedef bool          (*f_udict_iterator_has_next)(const void *di);
typedef void          (*f_udict_iterator_reset)(void *di);


typedef struct {
    f_udict_take_data_ownership take_data_ownership;
    f_udict_drop_data_ownership drop_data_ownership;
    f_udict_clear               clear;
    f_udict_put                 put;
    f_udict_get                 get;
    f_udict_pop                 pop;
    f_udict_has_key             has_key;
    f_udict_get_size            get_size;
    f_udict_is_empty            is_empty;
    f_udict_serialize           serialize;
    f_udict_as_str              as_str;
    f_udict_print               print;
    f_udict_fprint              fprint;
    f_udict_get_void_handlers   get_void_handlers;
    f_udict_get_items           get_items;
} udict_vtable_t;

typedef struct {
    f_udict_iterator_get_next   next;
    f_udict_iterator_has_next   has_next;
    f_udict_iterator_reset      reset;
} udict_iterator_vtable_t;

typedef struct {
    udict_backend_t backend;
    void *vobj;
    const udict_vtable_t *vtable;
} udict_t;

typedef struct {
    udict_backend_t backend;
    const udict_t *dict;
    void *vobj;
    const udict_iterator_vtable_t *vtable;
} udict_iterator_t;

udict_t *udict_create(void);
udict_t *udict_create_with_backend(udict_backend_t backend);
static inline void udict_take_data_ownership(udict_t *d) {d->vtable->take_data_ownership(d->vobj);}
static inline void udict_drop_data_ownership(udict_t *d) {d->vtable->drop_data_ownership(d->vobj);}
static inline void udict_clear(udict_t *d) {d->vtable->clear(d->vobj);}
static inline void udict_put(udict_t *d, ugeneric_t k, ugeneric_t v) {d->vtable->put(d->vobj, k, v);}
static inline ugeneric_t udict_get(const udict_t *d, ugeneric_t k, ugeneric_t vdef) {return d->vtable->get(d->vobj, k, vdef);}
static inline ugeneric_t udict_pop(udict_t *d, ugeneric_t k, ugeneric_t vdef) {return d->vtable->pop(d->vobj, k, vdef);}
static inline bool udict_has_key(const udict_t *d, ugeneric_t k) {return d->vtable->has_key(d->vobj, k);}
static inline size_t udict_get_size(const udict_t *d) {return d->vtable->get_size(d->vobj);}
static inline bool udict_is_empty(const udict_t *d) {return d->vtable->is_empty(d->vobj);}
static inline void udict_serialize(const udict_t *d, ubuffer_t *buf) {d->vtable->serialize(d->vobj, buf);}
static inline char *udict_as_str(const udict_t *d) {return d->vtable->as_str(d->vobj);}
static inline int udict_print(const udict_t *d) {return d->vtable->print(d->vobj);}
static inline int udict_fprint(const udict_t *d, FILE *out) {return d->vtable->fprint(d->vobj, out);}
static inline uvector_t *udict_get_items(const udict_t *d, udict_items_kind_t kind) {return d->vtable->get_items(d->vobj, kind);}
void udict_destroy(udict_t *d);
int udict_compare(const udict_t *d1, const udict_t *d2, void_cmp_t cmp);
void udict_set_void_hasher(udict_t *d, void_hasher_t hasher);
void udict_set_void_key_comparator(udict_t *d, void_cmp_t cmp);

static inline uvector_t *udict_get_keys(const udict_t *d) {return udict_get_items(d, UDICT_KEYS);}
static inline uvector_t *udict_get_values(const udict_t *d) {return udict_get_items(d, UDICT_VALUES);}

void *udict_copy(const udict_t *d);
void *udict_deep_copy(const udict_t *d);
udict_iterator_t *udict_iterator_create(const udict_t *d);
static inline ugeneric_kv_t udict_iterator_get_next(udict_iterator_t *di) {return di->vtable->next(di->vobj);}
static inline bool udict_iterator_has_next(const udict_iterator_t *di) {return di->vtable->has_next(di->vobj);}
static inline void udict_iterator_reset(udict_iterator_t *di) {di->vtable->reset(di->vobj);}
void udict_iterator_destroy(udict_iterator_t *di);

struct _convertor_data_descriptor {
    const char *name;
    ugeneric_type_e type;
    size_t offset;
    ugeneric_t dflt;
    bool is_array;
    struct _convertor_data_descriptor *field_descriptor; // for nested structures
    size_t field_size; // for nested structures
};
typedef struct _convertor_data_descriptor ugeneric_convertor_data_descriptor_t;

ugeneric_t udict_convert_to_struct(const udict_t *d, size_t struct_size,
                                   const ugeneric_convertor_data_descriptor_t *gcdd);

static inline uvoid_handlers_t *udict_get_void_handlers(udict_t *d) {return d->vtable->get_void_handlers(d->vobj);}
DECLARE_VOID_FUNCS(udict)

#endif
