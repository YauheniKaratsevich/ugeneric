#include <stdlib.h>
#include <string.h>
#include "mem.h"
#include "htbl.h"

#define HTBL_SCALE_FACTOR 2
#define HTBL_INITIAL_NUM_OF_BUCKETS 32
#define REHASH_THRESHOLD (3.0)/(4.0)

struct uhtbl_record {
    ugeneric_t k;
    ugeneric_t v;
    struct uhtbl_record *next;
};
typedef struct uhtbl_record uhtbl_record_t;

struct uhtbl_opaq {
    uhtbl_record_t **buckets;
    size_t number_of_buckets;
    size_t number_of_records;
    bool is_data_owner;
    void_hasher_t hasher;
    void_cmp_t key_cmp;
    void_cpy_t cpy;
    void_cmp_t cmp;
    void_dtr_t dtr;
};

struct uhtbl_iterator_opaq {
    const uhtbl_t *htbl;
    uhtbl_record_t *current_dr;
    size_t bucket;
    size_t records_to_iterate;
};

static void _destroy_buckets(uhtbl_t *h)
{
    for (size_t i = 0; i < h->number_of_buckets; i++)
    {
        uhtbl_record_t *hr = h->buckets[i];
        while (hr)
        {
            uhtbl_record_t *hr_next = hr->next;
            if (h->is_data_owner)
            {
                ugeneric_destroy(hr->k, h->dtr);
                ugeneric_destroy(hr->v, h->dtr);
            }
            ufree(hr);
            hr = hr_next;
        }
    }
}

/*
 * Either a record pointer or NULL when we are at the end of table.
 */
static uhtbl_record_t *_find_next_record(const uhtbl_t *h,
                                        uhtbl_record_t *current_dr,
                                        size_t *bucket)
{
    uhtbl_record_t *next_record = NULL;

    if (current_dr)
    {
        next_record = current_dr->next;
        if (!next_record)
        {
            *bucket += 1;
        }
    }

    while (!next_record && (*bucket < h->number_of_buckets))
    {
        next_record = h->buckets[*bucket];
        if (!next_record)
        {
            *bucket += 1;
        }
    }

    return next_record;
}

/*
 * Either return pointer to corresponded htbl record found by key
 * or return pointer to place where such a record should be placed.
 */
static uhtbl_record_t **_find_by_key(const uhtbl_t *h, ugeneric_t k)
{
    uhtbl_record_t **hr;
    hr = &h->buckets[ugeneric_hash(k, h->hasher) % h->number_of_buckets];
    while (*hr)
    {
        if (ugeneric_compare((*hr)->k, k, h->key_cmp) == 0)
        {
            break;
        }
        hr = &(*hr)->next;
    }

    return hr;
}

static float _get_load_factor(const uhtbl_t *h)
{
    return (float)h->number_of_records / h->number_of_buckets;
}
/*
static void _rehash(uhtbl_t *h)
{
    size_t new_number_of_buckets;
    uhtbl_record_t **new_buckets;

    new_number_of_buckets = HTBL_SCALE_FACTOR * d->number_of_buckets;
    new_buckets = ucalloc(new_number_of_buckets,
                          sizeof(d->buckets[0]));

    for (size_t i = 0; i < d->number_of_buckets; i++)
    {
        uhtbl_record_t *hr = d->buckets[i];
        while (dr)
        {
            uhtbl_record_t *hr_next = dr->next;
            if (d->is_data_owner)
            {
                ugeneric_destroy(dr->k, d->dtr);
                ugeneric_destroy(dr->v, d->dtr);
            }
            ufree(dr);
            dr = dr_next;
        }
    }

    //_destroy_buckets(

    d->buckets = new_buckets;
}
*/

uhtbl_t *uhtbl_create(void)
{
    uhtbl_t *h = umalloc(sizeof(*h));
    h->buckets = ucalloc(HTBL_INITIAL_NUM_OF_BUCKETS,
                         sizeof(h->buckets[0]));
    h->number_of_buckets = HTBL_INITIAL_NUM_OF_BUCKETS;
    h->number_of_records = 0;
    h->is_data_owner = true;
    h->hasher = NULL;
    h->key_cmp = NULL;
    h->cpy = NULL;
    h->cmp = NULL;
    h->dtr = NULL;

    return h;
}

void uhtbl_take_data_ownership(uhtbl_t *h)
{
    UASSERT_INPUT(h);
    h->is_data_owner = true;
}

void uhtbl_drop_data_ownership(uhtbl_t *h)
{
    UASSERT_INPUT(h);
    h->is_data_owner = false;
}

void uhtbl_set_destroyer(uhtbl_t *h, void_dtr_t dtr)
{
    UASSERT_INPUT(h);
    h->dtr = dtr;
}

void uhtbl_set_comparator(uhtbl_t *h, void_cmp_t cmp)
{
    UASSERT_INPUT(h);
    h->cmp = cmp;
}

void uhtbl_set_hasher(uhtbl_t *h, void_hasher_t hasher)
{
    UASSERT_INPUT(h);
    h->hasher = hasher;
}

/*
 * Puts without copy, keys and values which contains pointers
 * may cause issue if you forget who owns the data.
 */
void uhtbl_put(uhtbl_t *h, ugeneric_t k, ugeneric_t v)
{
    UASSERT_INPUT(h);
    uhtbl_record_t **hr = _find_by_key(h, k);

    if (_get_load_factor(h) >= REHASH_THRESHOLD)
    {
        //_rehash(d);
        // todo: implement me
    }

    if (*hr)
    {
        // Update existing.
        if (h->is_data_owner)
        {
            ugeneric_destroy((*hr)->k, h->dtr);
            ugeneric_destroy((*hr)->v, h->dtr);
        }
        (*hr)->k = k;
        (*hr)->v = v;
    }
    else
    {
        // Insert a new one.
        *hr = umalloc(sizeof(uhtbl_record_t));
        (*hr)->k = k;
        (*hr)->v = v;
        (*hr)->next = NULL;
        h->number_of_records += 1;
    }
}

/* Returns either data stored in htbl or
 * vdef if data is not found by the key.
*/
ugeneric_t uhtbl_get(const uhtbl_t *h, ugeneric_t k, ugeneric_t vdef)
{
    UASSERT_INPUT(h);
    uhtbl_record_t **hr = _find_by_key(h, k);

    return (*hr) ? (*hr)->v : vdef;
}

ugeneric_t uhtbl_pop(uhtbl_t *h, ugeneric_t k, ugeneric_t vdef)
{
    UASSERT_INPUT(h);
    ugeneric_t ret = vdef;
    uhtbl_record_t **hr = _find_by_key(h, k);

    if (*hr)
    {
        uhtbl_record_t *del = *hr;
        ret = del->v;
        ugeneric_destroy(del->k, h->dtr);
        *hr = (*hr)->next;
        ufree(del);
        h->number_of_records -= 1;
    }

    return ret;
}

void uhtbl_destroy(uhtbl_t *h)
{
    if (h)
    {
        _destroy_buckets(h);
        ufree(h->buckets);
        ufree(h);
    }
}

void uhtbl_clear(uhtbl_t *h)
{
    UASSERT_INPUT(h);
    _destroy_buckets(h);
    memset(h->buckets, 0, h->number_of_buckets * sizeof(h->buckets[0]));
    h->number_of_records = 0;
}

void uhtbl_serialize(const uhtbl_t *h, ubuffer_t *buf)
{
    UASSERT_INPUT(h);
    UASSERT_INPUT(buf);

    ubuffer_append_byte(buf, '{');
    uhtbl_iterator_t *hi = uhtbl_iterator_create(h);
    while (uhtbl_iterator_has_next(hi))
    {
        ugeneric_kv_t kv = uhtbl_iterator_get_next(hi);
        ugeneric_serialize(kv.k, buf, NULL);
        ubuffer_append_data(buf, ": ", 2);
        ugeneric_serialize(kv.v, buf, NULL);
        if (uhtbl_iterator_has_next(hi))
        {
            ubuffer_append_data(buf, ", ", 2);
        }
    }
    uhtbl_iterator_destroy(hi);
    ubuffer_append_byte(buf, '}');
}

char *uhtbl_as_str(const uhtbl_t *h)
{
    UASSERT_INPUT(h);

    ubuffer_t buf = {0};
    uhtbl_serialize(h, &buf);
    ubuffer_null_terminate(&buf);

    return buf.data;
}

int uhtbl_print(const uhtbl_t *h)
{
    UASSERT_INPUT(h);
    return uhtbl_fprint(h, stdout);
}

int uhtbl_fprint(const uhtbl_t *h, FILE *out)
{
    UASSERT_INPUT(h);
    UASSERT_INPUT(out);

    char *str = uhtbl_as_str(h);
    int ret = fprintf(out, "%s\n", str);
    ufree(str);

    return ret;
}

void uhtbl_dump_to_dot(const uhtbl_t *h, FILE *out)
{
    UASSERT_INPUT(h);
    UASSERT_INPUT(out);

    fprintf(out, "%s %s {\n", "higraph", "uhtbl_name");
    fprintf(out, "    rankhir=LR;\n");
    fprintf(out, "    node [shape=record];\n");

    // buckets
    fprintf(out, "    node0 [label = \"");
    for (size_t i = 0; i < h->number_of_buckets; i++)
    {
        fprintf(out, "<f%zu>|", i);
    }
    fprintf(out, "\"];\n");

    size_t j = 1;
    for (size_t i = 0; i < h->number_of_buckets; i++)
    {
        uhtbl_record_t *hr = h->buckets[i];
        if (hr)
        {
            size_t k = j;
            fprintf(out, "\n");
            while (hr)
            {
                char *k = ugeneric_as_str(hr->k, NULL);
                char *v = ugeneric_as_str(hr->v, NULL);
                fprintf(out, "    node%zu [label = \"{ <data> '%s':'%s' | <ref> }\"];\n",
                        j++, k, v);
                hr = hr->next;
                ufree(k);
                ufree(v);
            }
            fprintf(out, "    node0:f%zu -> node%zu:data;\n", i, k);
            while (k < (j - 1))
            {
                fprintf(out, "    node%zu:ref -> node%zu:data;\n", k, k + 1);
                k++;
            }
        }
    }

    fprintf(out, "}\n");
}

uhtbl_iterator_t *uhtbl_iterator_create(const uhtbl_t *h)
{
    UASSERT_INPUT(h);

    uhtbl_iterator_t *hi = umalloc(sizeof(*hi));
    hi->htbl = h;
    hi->bucket = 0;
    hi->records_to_iterate = h->number_of_records;
    hi->current_dr = NULL;

    return hi;
}

ugeneric_kv_t uhtbl_iterator_get_next(uhtbl_iterator_t *hi)
{
    UASSERT_INPUT(hi);
    UASSERT_MSG(hi->records_to_iterate, "iteration is done");
    UASSERT_MSG(hi->htbl->number_of_records, "container is empty");

    hi->current_dr = _find_next_record(hi->htbl, hi->current_dr, &hi->bucket);
    ugeneric_kv_t kv = {.k = hi->current_dr->k, .v = hi->current_dr->v};
    hi->records_to_iterate -= 1;

    return kv;
}

bool uhtbl_iterator_has_next(const uhtbl_iterator_t *hi)
{
    UASSERT_INPUT(hi);
    return hi->records_to_iterate;
}

void uhtbl_iterator_reset(uhtbl_iterator_t *hi)
{
    UASSERT_INPUT(hi);
    hi->bucket = 0;
    hi->current_dr = hi->htbl->buckets[0];
    hi->records_to_iterate = hi->htbl->number_of_records;
}

void uhtbl_iterator_destroy(uhtbl_iterator_t *hi)
{
    if (hi)
    {
        ufree(hi);
    }
}

size_t uhtbl_get_size(const uhtbl_t *h)
{
    UASSERT_INPUT(h);
    return h->number_of_records;
}

bool uhtbl_is_empty(const uhtbl_t *h)
{
    UASSERT_INPUT(h);
    return h->number_of_records == 0;
}

bool uhtbl_has_key(const uhtbl_t *h, ugeneric_t k)
{
    UASSERT_INPUT(h);
    return *_find_by_key(h, k) != NULL;
}

uvector_t *uhtbl_get_keys(const uhtbl_t *h)
{
    uhtbl_iterator_t *hi = uhtbl_iterator_create(h);
    uvector_t *v = uvector_create();
    uvector_reserve_capacity(v, h->number_of_records);
    uvector_drop_data_ownership(v);
    while (uhtbl_iterator_has_next(hi))
    {
        uvector_append(v, uhtbl_iterator_get_next(hi).k);
    }
    uhtbl_iterator_destroy(hi);

    return v;
}

uvector_t *uhtbl_get_values(const uhtbl_t *h)
{
    uhtbl_iterator_t *hi = uhtbl_iterator_create(h);
    uvector_t *v = uvector_create();
    uvector_reserve_capacity(v, h->number_of_records);
    uvector_drop_data_ownership(v);
    while (uhtbl_iterator_has_next(hi))
    {
        uvector_append(v, uhtbl_iterator_get_next(hi).v);
    }
    uhtbl_iterator_destroy(hi);

    return v;
}
