#include <stdio.h>
#include <string.h>
#include "generic.h"
#include "dict.h"
#include "vector.h"
#include "mem.h"
#include "asserts.h"
#include "file_utils.h"
#include "ut_utils.h"

void test_types(void)
{
    ugeneric_t g_integer = G_INT(333);
    ugeneric_t g_real = G_REAL(33.33);
    ugeneric_t g_string = G_STR(ustring_dup("hello ugeneric"));
    ugeneric_t g_cstring = G_STR("hello constant ugeneric");
    ugeneric_t g_size = G_SIZE(123412341);
    ugeneric_t g_null = G_NULL;
    ugeneric_t g_true = G_TRUE;
    ugeneric_t g_false = G_FALSE;
    ugeneric_t g_vector = G_VECTOR(uvector_create());
    ugeneric_t g_dict = G_DICT(udict_create());

    UASSERT(G_IS_INT(g_integer));
    UASSERT(G_IS_REAL(g_real));
    UASSERT(G_IS_STR(g_string));
    UASSERT(G_IS_CSTR(g_cstring));
    UASSERT(G_IS_STRING(g_cstring));
    UASSERT(G_IS_STRING(g_string));
    UASSERT(G_IS_SIZE(g_size));
    UASSERT(G_IS_NULL(g_null));
    UASSERT(G_IS_TRUE(g_true));
    UASSERT(G_IS_FALSE(g_false));
    UASSERT(G_IS_VECTOR(g_vector));
    UASSERT(G_IS_DICT(g_dict));

    ugeneric_t g = G_MEMCHUNK(&g, sizeof(g));
    UASSERT(G_IS_MEMCHUNK(g));
    UASSERT(G_AS_MEMCHUNK_DATA(g) == &g);
    UASSERT(G_AS_MEMCHUNK_SIZE(g) == sizeof(g));
    umemchunk_t t = {.size = 5, .data = "1234\xff"};
    char *str = umemchunk_as_str(t);
    UASSERT_STR_EQ("31323334ff", str);
    ufree(str);

    ugeneric_destroy(g_vector, NULL);
    ugeneric_destroy(g_dict, NULL);
    ugeneric_destroy(g_string, NULL);
}

void test_generic(void)
{
    //printf("%zu\n", sizeof(ugeneric_t));

    char *str = ugeneric_as_str(G_STR("generic"), NULL);
    UASSERT(strcmp(str, "\"generic\"") == 0) ;
    ufree(str);
}

void test_random(void)
{
    while (true)
    {
        printf("%d\n", random_from_range(0, 20));
    }
}

void test_parse(void)
{
    typedef struct {
        const char *in;
        const char *out;
        const char *err;
    } tcase_t;

    tcase_t tc[] = {
        {"[]", "[]", NULL},
        {"{}", "{}", NULL},
        {"[{}]", "[{}]", NULL},
        {"[{},{}]", "[{}, {}]", NULL},
        {"[[],[]]", "[[], []]", NULL},
        {"[[[[[]]]]]","[[[[[]]]]]", NULL},
        {"[1]", "[1]", NULL},
        {"[1,2,3]", "[1, 2, 3]", NULL},
        {"[1,2,3,]", "[1, 2, 3]", NULL},
        {"{ }", "{}", NULL},
        {"[ ]", "[]", NULL},
        {"\"t\\\"tt\"", "\"t\\\"tt\"", NULL},
        {"\"str'ing\"", "\"str'ing\"", NULL},
        {"\"\\\"\\\"\\\"\"", "\"\\\"\\\"\\\"\"", NULL},
        {"\"\\\\\\\\\"", "\"\\\\\"", NULL},
        {"[ ]", "[]", NULL},
        {"{ }", "{}", NULL},
        {"null", "null", NULL},
        {"true", "true", NULL},
        {"false", "false", NULL},
        {"\"str\"", "\"str\"", NULL},
        {"12345", "12345", NULL},
        {"-69.38","-69.38", NULL},
        {"'plata o plomo'","\"plata o plomo\"", NULL},
        {"[1,2,3,4]", "[1, 2, 3, 4]", NULL},
        {"{1:2}", "{1: 2}", NULL},
        {"{1:2, true: false}", "{1: 2, true: false}", NULL},
        {"{1: {2: {true: false}}}", "{1: {2: {true: false}}}", NULL},
        {"{1:[1], 2:[2]}", "{1: [1], 2: [2]}", NULL},
        {"1.2E34", "1.2e+34", NULL},
        {"[", NULL, "Parsing failed at offset 1"},
        {"[],", NULL, "Parsing failed at offset 2"},
        {"{},", NULL, "Parsing failed at offset 2"},
        {",", NULL, "Parsing failed at offset 0"},
        {"[0,,]", NULL, "Parsing failed at offset 3"},
        {"null,", NULL, "Parsing failed at offset 4"},
        {"\"str", NULL, "Parsing failed at offset 4"},
        {"[{]}", NULL, "Parsing failed at offset 2"},
        {"[1,2,}", NULL, "Parsing failed at offset 5"},
        {"{1,2,}", NULL, "Parsing failed at offset 2"},
        {"]", NULL, "Parsing failed at offset 0"},
        {"}", NULL, "Parsing failed at offset 0"},
        {"a", NULL, "Parsing failed at offset 0"},
        {"&", NULL, "Parsing failed at offset 0"},
        {"", NULL, "Parsing failed at offset 0"},
        {"{true: {false: [];}}", NULL, "Parsing failed at offset 17"},
        {"1 ", "1", NULL},
        {"1.01 ", "1.01", NULL},
        {"1\n", "1", NULL},
        {"1.01\n", "1.01", NULL},
        {"1\t", "1", NULL},
        {"-", NULL, "Parsing failed at offset 0"},
        {"[-]", NULL, "Parsing failed at offset 1"},
        {"[-3-]", NULL, "Parsing failed at offset 3"},
        {"--3", NULL, "Parsing failed at offset 0"},
        {"0.0", "0", NULL},
        {"-0.0", "-0", NULL},
        {"1.0", "1", NULL},
        {"-1.0", "-1", NULL},
        {"1.5", "1.5", NULL},
        {"-1.5", "-1.5", NULL},
        {"3.1416", "3.1416", NULL},
        {"2E20", "2e+20", NULL},
        {"2e20", "2e+20", NULL},
        {"2E+20", "2e+20", NULL},
        {"2E-20", "2e-20", NULL},
        {"-1E10", "-1e+10", NULL},
        {"-1e10", "-1e+10", NULL},
        {"-1E+10", "-1e+10", NULL},
        {"-1E-10", "-1e-10", NULL},
        {"1.234E+10", "1.234e+10", NULL},
        {"1.234E-10", "1.234e-10", NULL},
        {"0.9868011474609375", "0.986801", NULL},
        {"45913141877270640000.0", "4.59131e+19", NULL},
        {"0.017976931348623157e+310", "1.79769e+308", NULL},
        {"5708990770823839207320493820740630171355185152001e-3", "5.70899e+45", NULL},
        {0}
    };

    // We need to have the dict to be sorted
    libuugeneric_udict_set_default_backend(UDICT_BACKEND_UBST_RB);

    tcase_t *t = tc;
    while (t->in)
    {
        ugeneric_t g = ugeneric_parse(t->in);
        if (t->out)
        {
            if (G_IS_ERROR(g))
            {
                ugeneric_error_print(g);
                ugeneric_error_destroy(g);
                UABORT("test failed");
            }
            char *out = ugeneric_as_str(g, NULL);
            UASSERT_STR_EQ(out, t->out);
            ufree(out);
            //ugeneric_print(g);
            ugeneric_destroy(g, NULL);
        }
        else
        {
            if (!G_IS_ERROR(g))
            {
                //ugeneric_print(g);
            }
            UASSERT(G_IS_ERROR(g));
            //ugeneric_error_print(g);
            if (!ustring_starts_with(G_AS_STR(g), t->err))
            {
                fprintf(stdout, "'%s' != '%s'\n", G_AS_STR(g), t->err);
                UABORT("test failed");
            }
            ugeneric_error_destroy(g);
        }
        t++;
    }
}

void test_large_parse(void)
{
    const char *path = "utdata/json.json";
    ugeneric_t g = ufile_read_to_string(path);
    UASSERT_NO_ERROR(g);
    char *json = G_AS_STR(g);
    g = ugeneric_parse(json);
    UASSERT_NO_ERROR(g);
    ufree(json);

    //ugeneric_print(g);

    ugeneric_destroy(g, NULL);
}

void test_serialize(void)
{
    udict_t *d = udict_create();
    udict_t *dempty = udict_create();
    uvector_t *v = uvector_create();
    uvector_t *vempty = uvector_create();
    uvector_append(v, G_NULL);
    uvector_append(v, G_TRUE);
    uvector_append(v, G_FALSE);
    uvector_append(v, G_VECTOR(vempty));
    uvector_append(v, G_DICT(dempty));
    uvector_append(v, G_INT(-1));
    uvector_append(v, G_INT(2));
    uvector_append(v, G_REAL(3.4));
    uvector_append(v, G_SIZE(1888888888888881));
    uvector_append(v, G_PTR(NULL));
    uvector_set_destroyer(v, ufree);
    udict_put(d, G_CSTR("key"), G_VECTOR(v));

    char *str = ugeneric_as_str(G_DICT(d), NULL);
    UASSERT_STR_EQ(str, "{\"key\": [null, true, false, [], {}, -1, 2, 3.4, 1888888888888881, &(nil)]}");
    ufree(str);
    udict_destroy(d);
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        ugeneric_t g = ufile_read_to_string(argv[1]);
        if (G_IS_ERROR(g))
        {
            puts(G_AS_STR(g));
            abort();
        }
        char *str = G_AS_STR(g);
        puts(G_AS_STR(g));
        ugeneric_t res = ugeneric_parse(str);
        if (G_IS_ERROR(res))
        {
            puts(G_AS_STR(res));
        }
        else
        {
            ugeneric_print(res, NULL);
        }
    }

    test_types();
    //test_random();
//    test_generic();
    test_parse();
    test_large_parse();
    test_serialize();
}
