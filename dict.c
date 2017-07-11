#include <stdlib.h>
#include <string.h>
#include "generic.h"
#include "mem.h"
#include "dict.h"
#include "htbl.h"
#include "bst.h"
#include "string_utils.h"

static const udict_vtable_t _uhtbl_vtable = {
    .take_data_ownership = (f_udict_take_data_ownership)uhtbl_take_data_ownership,
    .drop_data_ownership = (f_udict_drop_data_ownership)uhtbl_drop_data_ownership,
    .clear               = (f_udict_clear)uhtbl_clear,
    .put                 = (f_udict_put)uhtbl_put,
    .get                 = (f_udict_get)uhtbl_get,
    .pop                 = (f_udict_pop)uhtbl_pop,
    .has_key             = (f_udict_has_key)uhtbl_has_key,
    .get_size            = (f_udict_get_size)uhtbl_get_size,
    .is_empty            = (f_udict_is_empty)uhtbl_is_empty,
    .serialize           = (f_udict_serialize)uhtbl_serialize,
    .as_str              = (f_udict_as_str)uhtbl_as_str,
    .print               = (f_udict_print)uhtbl_print,
    .fprint              = (f_udict_fprint)uhtbl_fprint,
    .get_void_handlers   = (f_udict_get_void_handlers)uhtbl_get_void_handlers,
    .get_items           = (f_udict_get_items)uhtbl_get_items,
};

static const udict_vtable_t _ubst_vtable = {
    .take_data_ownership = (f_udict_take_data_ownership)ubst_take_data_ownership,
    .drop_data_ownership = (f_udict_drop_data_ownership)ubst_drop_data_ownership,
    .clear               = (f_udict_clear)ubst_clear,
    .put                 = (f_udict_put)ubst_put,
    .get                 = (f_udict_get)ubst_get,
    .pop                 = (f_udict_pop)ubst_pop,
    .has_key             = (f_udict_has_key)ubst_has_key,
    .get_size            = (f_udict_get_size)ubst_get_size,
    .is_empty            = (f_udict_is_empty)ubst_is_empty,
    .serialize           = (f_udict_serialize)ubst_serialize,
    .as_str              = (f_udict_as_str)ubst_as_str,
    .print               = (f_udict_print)ubst_print,
    .fprint              = (f_udict_fprint)ubst_fprint,
    .get_void_handlers   = (f_udict_get_void_handlers)ubst_get_void_handlers,
    .get_items           = (f_udict_get_items)ubst_get_items,
};

static const udict_iterator_vtable_t _uhtbl_iterator_vtable = {
    .next                = (f_udict_iterator_get_next)uhtbl_iterator_get_next,
    .has_next            = (f_udict_iterator_has_next)uhtbl_iterator_has_next,
    .reset               = (f_udict_iterator_reset)uhtbl_iterator_reset,
};

static const udict_iterator_vtable_t _ubst_iterator_vtable = {
    .next                = (f_udict_iterator_get_next)ubst_iterator_get_next,
    .has_next            = (f_udict_iterator_has_next)ubst_iterator_has_next,
    .reset               = (f_udict_iterator_reset)ubst_iterator_reset,
};

static udict_backend_t _default_backend = UDICT_BACKEND_BST_PLAIN;
//static udict_backend_t _default_backend = UDICT_BACKEND_HTBL;

void libugeneric_udict_set_default_backend(udict_backend_t backend)
{
    UASSERT_INPUT(backend > UDICT_BACKEND_DEFAULT);
    UASSERT_INPUT(backend < UDICT_BACKENDS_COUNT);
    _default_backend = backend;
}

udict_backend_t libugeneric_udict_get_default_backend(void)
{
    return _default_backend;
}

udict_t *udict_create(void)
{
    return udict_create_with_backend(UDICT_BACKEND_DEFAULT);
}

udict_t *udict_create_with_backend(udict_backend_t backend)
{
    UASSERT_INPUT(backend >= UDICT_BACKEND_DEFAULT);
    UASSERT_INPUT(backend < UDICT_BACKENDS_COUNT);

    udict_t *d = umalloc(sizeof(*d));
    d->backend = (backend == UDICT_BACKEND_DEFAULT) ? _default_backend : backend;
    switch (d->backend)
    {
        case UDICT_BACKEND_HTBL:
            d->vobj = uhtbl_create();
            d->vtable = &_uhtbl_vtable;
            break;
        case UDICT_BACKEND_BST_PLAIN:
            d->vobj = ubst_create_ext(UBST_NO_BALANCING);
            d->vtable = &_ubst_vtable;
            break;
        case UDICT_BACKEND_BST_RB:
            d->vobj = ubst_create_ext(UBST_RB_BALANCING);
            d->vtable = &_ubst_vtable;
            break;
        default:
            UABORT("internal error");
    }
    return d;
}

void udict_destroy(udict_t *d)
{
    UASSERT_INPUT(d);

    switch (d->backend)
    {
        case UDICT_BACKEND_HTBL:
            uhtbl_destroy(d->vobj);
            break;
        case UDICT_BACKEND_BST_PLAIN:
        case UDICT_BACKEND_BST_RB:
            ubst_destroy(d->vobj);
            break;
        default:
            UABORT("internal error");
    }
    ufree(d);
}

udict_iterator_t *udict_iterator_create(const udict_t *d)
{
    UASSERT_INPUT(d);

    udict_iterator_t *di = umalloc(sizeof(*di));
    di->dict = d;
    switch (d->backend)
    {
        case UDICT_BACKEND_HTBL:
            di->vobj = uhtbl_iterator_create(d->vobj);
            di->vtable = &_uhtbl_iterator_vtable;
            break;
        case UDICT_BACKEND_BST_PLAIN:
        case UDICT_BACKEND_BST_RB:
            di->vobj = ubst_iterator_create(d->vobj);
            di->vtable = &_ubst_iterator_vtable;
            break;
        default:
            UABORT("internal error");
    }

    return di;
}

void udict_iterator_destroy(udict_iterator_t *di)
{
    if (di)
    {
        switch (di->dict->backend)
        {
            case UDICT_BACKEND_HTBL:
                uhtbl_iterator_destroy(di->vobj);
                break;
            case UDICT_BACKEND_BST_PLAIN:
            case UDICT_BACKEND_BST_RB:
                ubst_iterator_destroy(di->vobj);
                break;
            default:
                UABORT("internal error");
        }
        ufree(di);
    }
}

#define UDICT_ON_BST(d) ((d->backend == UDICT_BACKEND_BST_PLAIN) || \
                         (d->backend == UDICT_BACKEND_BST_RB))

int udict_compare(const udict_t *d1, const udict_t *d2, void_cmp_t cmp)
{
    int diff;
    UASSERT_INPUT(d1);
    UASSERT_INPUT(d2);

    if (d1 == d2)
    {
        return 0;
    }

    if ((d1->backend == UDICT_BACKEND_HTBL) && (d2->backend == UDICT_BACKEND_HTBL))
    {
        diff = uhtbl_compare(d1->vobj, d2->vobj, cmp);
    }
    else if (UDICT_ON_BST(d1) && UDICT_ON_BST(d2))
    {
        diff = ubst_compare(d1->vobj, d2->vobj, cmp);
    }
    else
    {
        uvector_t *items1 = udict_get_items(d1, UDICT_KV);
        uvector_t *items2 = udict_get_items(d2, UDICT_KV);
        if (d1->backend == UDICT_BACKEND_HTBL)
        {
            uvector_sort(items1);
        }
        if (d2->backend == UDICT_BACKEND_HTBL)
        {
            uvector_sort(items2);
        }
        diff = uvector_compare(items1, items2, cmp);
        uvector_destroy(items1);
        uvector_destroy(items2);
    }

    return diff;
}

udict_t *_dcpy(const udict_t *d, bool deep)
{
    udict_t *copy = udict_create_with_backend(d->backend);
    udict_iterator_t *di = udict_iterator_create(d);

    if (d->backend == UDICT_BACKEND_HTBL)
    {
        uhtbl_t *h = (uhtbl_t *)d->vobj;
        uhtbl_t *hc = (uhtbl_t *)copy->vobj;
        uhtbl_set_void_hasher(hc, uhtbl_get_void_hasher(h));
        uhtbl_set_void_key_comparator(hc, uhtbl_get_void_key_comparator(h));
    }

    void_cpy_t cpy = udict_get_void_copier((udict_t *)d);
    deep ? udict_take_data_ownership(copy) : udict_drop_data_ownership(copy);
    udict_set_void_comparator(copy, udict_get_void_comparator((udict_t *)d));
    udict_set_void_destroyer(copy, udict_get_void_destroyer((udict_t *)d));
    udict_set_void_serializer(copy, udict_get_void_serializer((udict_t *)d));
    udict_set_void_copier(copy, cpy);

    while (udict_iterator_has_next(di))
    {
        ugeneric_kv_t kv = udict_iterator_get_next(di);
        ugeneric_t k = deep ? ugeneric_copy(kv.k, cpy) : kv.k;
        ugeneric_t v = deep ? ugeneric_copy(kv.v, cpy) : kv.v;
        udict_put(copy, k, v);
    }
    udict_iterator_destroy(di);

    return copy;
}

void *udict_copy(const udict_t *d)
{
    UASSERT_INPUT(d);
    return _dcpy(d, false);
}

void *udict_deep_copy(const udict_t *d)
{
    UASSERT_INPUT(d);
    return _dcpy(d, true);
}

void udict_set_void_hasher(udict_t *d, void_hasher_t hasher)
{
    UASSERT_INPUT(d);
    UASSERT_INPUT(d->backend == UDICT_BACKEND_HTBL);
    uhtbl_set_void_hasher(d->vobj, hasher);
}

void udict_set_void_key_comparator(udict_t *d, void_cmp_t cmp)
{
    UASSERT_INPUT(d);
    UASSERT_INPUT(d->backend == UDICT_BACKEND_HTBL);
    uhtbl_set_void_key_comparator(d->vobj, cmp);
}

ugeneric_t udict_convert_to_struct(const udict_t *d, size_t struct_size,
                                   const ugeneric_convertor_data_descriptor_t *gcdd)
{
    UASSERT_INPUT(d);
    UASSERT_INPUT(gcdd);

    struct __array_placeholder {
        size_t len;
        void *cells;
    };
    const size_t cells_offset = offsetof(struct __array_placeholder, cells);

    const ugeneric_convertor_data_descriptor_t *i = gcdd;
    void *p = uzalloc(struct_size);

    while (i->name)
    {
        ugeneric_t g = udict_get(d, G_CSTR(i->name), i->dflt);
        if (G_IS_ERROR(g))
        {
            return G_ERROR(ustring_fmt("default value for field '%s' is not provided", i->name));
        }
        if (i->is_array)
        {
            if (ugeneric_get_type(g) != G_VECTOR_T)
            {
                goto format_error;
            }
        }
        else
        {
            if (i->type != ugeneric_get_type(g))
            {
                goto format_error;
            }
        }
        switch (i->type)
        {
            case G_STR_T:
                if (i->is_array)
                {
                    uvector_t *v = G_AS_PTR(g);
                    char **q = NULL;
                    size_t len = uvector_get_size(v);
                    if (len)
                    {
                        q = umalloc(len * sizeof(*q));
                        ugeneric_t *cells = uvector_get_cells(v);
                        for (size_t j = 0; j < len; j++)
                        {
                            ugeneric_t e = cells[j];
                            if (ugeneric_get_type(e) != i->type)
                            {
                                goto format_error;
                            }
                            q[j] = ustring_dup(G_AS_STR(e));
                        }
                    }
                    *(size_t *)((char *)p + i->offset) = len;
                    *(char ***)((char *)p + i->offset + cells_offset) = q;
                }
                else
                {
                    *(char **)((char *)p + i->offset) = ustring_dup(G_AS_STR(g));
                }
                break;
            case G_INT_T:
                if (i->is_array)
                {
                    uvector_t *v = G_AS_PTR(g);
                    long int *q = NULL;
                    size_t len = uvector_get_size(v);
                    if (len)
                    {
                        q = umalloc(len * sizeof(*q));
                        ugeneric_t *cells = uvector_get_cells(v);
                        for (size_t j = 0; j < len; j++)
                        {
                            ugeneric_t e = cells[j];
                            if (ugeneric_get_type(e) != i->type)
                            {
                                goto format_error;
                            }
                            q[j] = G_AS_BOOL(e);
                        }
                    }
                    *(size_t *)((char *)p + i->offset) = len;
                    *(long int **)((char *)p + i->offset + cells_offset) = q;
                }
                else
                {
                    *(long int *)((char *)p + i->offset) = G_AS_INT(g);
                }
                break;
            case G_BOOL_T:
                if (i->is_array)
                {
                    uvector_t *v = G_AS_PTR(g);
                    bool *q = NULL;
                    size_t len = uvector_get_size(v);
                    if (len)
                    {
                        q = umalloc(len * sizeof(*q));
                        ugeneric_t *cells = uvector_get_cells(v);
                        for (size_t j = 0; j < len; j++)
                        {
                            ugeneric_t e = cells[j];
                            if (ugeneric_get_type(e) != i->type)
                            {
                                goto format_error;
                            }
                            q[j] = G_AS_BOOL(e);
                        }
                    }
                    *(size_t *)((char *)p + i->offset) = len;
                    *(bool **)((char *)p + i->offset + cells_offset) = q;
                }
                else
                {
                    *(bool *)((char *)p + i->offset) = G_AS_BOOL(g);
                }
                break;
            case G_DICT_T:
                if (i->is_array)
                {
                    uvector_t *v = G_AS_PTR(g);
                    void **q = NULL;
                    size_t len = uvector_get_size(v);
                    if (len)
                    {
                        q = umalloc(len * sizeof(*q));
                        ugeneric_t *cells = uvector_get_cells(v);
                        for (size_t j = 0; j < len; j++)
                        {
                            ugeneric_t e = cells[j];
                            if (ugeneric_get_type(e) != i->type)
                            {
                                goto format_error;
                            }
                            ugeneric_t t = udict_convert_to_struct(G_AS_PTR(e), i->field_size, i->field_descriptor);
                            if (G_IS_ERROR(t))
                            {
                                return t;
                            }
                            q[j] = G_AS_PTR(t);
                        }
                    }
                    *(size_t *)((char *)p + i->offset) = len;
                    *(void ***)((char *)p + i->offset + cells_offset) = q;
                }
                else
                {
                    ugeneric_t t = udict_convert_to_struct(G_AS_PTR(g), i->field_size, i->field_descriptor);
                    if (G_IS_ERROR(t))
                    {
                        return t;
                    }
                    *(void **)((char *)p + i->offset) = G_AS_PTR(t);
                }
                break;
            default:
                goto format_error;

        }
        i++;
    }

    return G_PTR(p);

format_error:
    ufree(p);
    return G_ERROR(ustring_fmt("data descriptor format error"));
}
