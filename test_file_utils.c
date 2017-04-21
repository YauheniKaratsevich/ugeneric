#include "file_utils.h"
#include "mem.h"
#include "ut_utils.h"

size_t execute_read(const char *path, size_t buffer_size)
{
    size_t read_count = 0;
    generic_t g = file_reader_create(path, buffer_size);
    ASSERT_NO_ERROR(g);

    file_reader_t *fr = G_AS_PTR(g);
    while (file_reader_has_next(fr))
    {
        g = file_reader_read_next(fr);
        ASSERT_NO_ERROR(g);

//        generic_print(g);
        read_count++;
    }

    file_reader_destroy(fr);

    return read_count;
}

void test_file_reader(void)
{
    generic_t g;
    file_reader_t *fr;

    ASSERT_INT_EQ(execute_read("utdata/100", 1), 100);
    ASSERT_INT_EQ(execute_read("utdata/100", 11), 10);
    ASSERT_INT_EQ(execute_read("utdata/100", 1), 100);
    ASSERT_INT_EQ(execute_read("utdata/empty", 1), 0);
    ASSERT_INT_EQ(execute_read("utdata/empty", 1001), 0);

    ASSERT_NO_ERROR(g = file_reader_create("utdata/empty", 10));
    fr = G_AS_PTR(g);
    g = file_reader_get_size(fr);
    ASSERT_INT_EQ(G_AS_SIZE(g), 0);
    file_reader_destroy(fr);

    ASSERT_NO_ERROR(g = file_reader_create("utdata/100", 10));
    fr = G_AS_PTR(g);
    g = file_reader_get_size(fr);
    ASSERT_INT_EQ(G_AS_SIZE(g), 100);
    file_reader_destroy(fr);
}

void test_file_writer(size_t buffer_size)
{
    generic_t g;
    generic_t gr = file_reader_create("ttt", buffer_size);
    ASSERT_NO_ERROR(gr);
    generic_t gw = file_writer_create("ttt2");
    ASSERT_NO_ERROR(gw);

    file_reader_t *fr = G_AS_PTR(gr);
    file_writer_t *fw = G_AS_PTR(gw);
    while (file_reader_has_next(fr))
    {
        g = file_reader_read_next(fr);
        ASSERT_NO_ERROR(g);
        g = file_writer_write_next(fw, G_AS_MCHUNK(g));
        ASSERT_NO_ERROR(g);
    }

    file_reader_destroy(fr);
    file_writer_destroy(fw);
}

void test_file_api(void)
{
    generic_t g;

    g = file_open("", "r");
    ASSERT(G_IS_ERROR(g));
 //   generic_error_print(g);
    generic_error_destroy(g);

    g = file_open("/root", "r");
    ASSERT(G_IS_ERROR(g));
//    generic_error_print(g);
    generic_error_destroy(g);

    ASSERT_NO_ERROR(g = file_get_size("utdata/empty"));
    ASSERT_SIZE_EQ(G_AS_SIZE(g), 0);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    test_file_api();
    test_file_reader();
    //test_file_writer(atoi(argv[1]));

    return 0;
}