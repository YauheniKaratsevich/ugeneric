#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mem.h"
#include "file_utils.h"

struct ufile_reader_opaq {
    FILE *file;
    size_t file_size;
    size_t read_offset;
    void *buffer;
    size_t buffer_size;
};

struct ufile_writer_opaq {
    FILE *file;
};

// Default handler does nothing besides propogating error up.
static ugeneric_t _default_error_handler(ugeneric_t io_error, void *ctx)
{
    (void)ctx;
    return io_error;
}

ufile_error_handler_t _error_handler = _default_error_handler;
void *_error_handler_ctx = NULL;

static ugeneric_t _get_position(FILE *f)
{
    long pos = ftell(f);
    if (pos == -1)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    return G_SIZE(pos);
}

static ugeneric_t _set_position(FILE *f, size_t position)
{
    if (fseek(f, position, SEEK_SET) == -1)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }
    return G_NULL;
}

static ugeneric_t _get_file_size(FILE *f, bool save_pos)
{
    long saved_pos;
    long fsize;
    ugeneric_t g;

    if (save_pos)
    {
        if (G_IS_ERROR(g = _get_position(f)))
        {
            return g;
        }
        saved_pos = G_AS_SIZE(g);
    }

    if (fseek(f, 0, SEEK_END) == -1)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    if (G_IS_ERROR(g = _get_position(f)))
    {
        return g;
    }
    fsize = G_AS_SIZE(g);

    if (save_pos)
    {
        if (G_IS_ERROR(g = _set_position(f, saved_pos)))
        {
            return g;
        }
    }

    return G_SIZE(fsize);
}

ugeneric_t ufile_get_size(const char *path)
{
    UASSERT_INPUT(path);

    ugeneric_t g = ufile_open(path, "r");
    if (G_IS_ERROR(g))
    {
        return g;
    }

    FILE *f = G_AS_PTR(g);
    g = _get_file_size(f, false);

    if (fclose(f) != 0)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    return g;
}

ugeneric_t ufile_read_to_string(const char *path)
{
    UASSERT_INPUT(path);

    ugeneric_t g = ufile_get_size(path);
    if (G_IS_ERROR(g))
    {
        return g;
    }
    size_t fsize = G_AS_SIZE(g);

    g = ufile_open(path, "r");
    if (G_IS_ERROR(g))
    {
        return g;
    }

    FILE *f = G_AS_PTR(g);
    char *t = umalloc(fsize + 1);
    if (fread(t, 1, fsize, f) < fsize)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }
    t[fsize] = 0;

    if (fclose(f) != 0)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    return G_STR(t);
}

ugeneric_t ufile_read_lines(const char *path)
{
    UASSERT_INPUT(path);

    ugeneric_t g = ufile_read_to_string(path);
    if (G_IS_ERROR(g))
    {
        return g;
    }

    char *str = G_AS_STR(g);
    uvector_t *v = ustring_split(str, "\n");
    ufree(str);

    if (strlen(G_AS_STR(uvector_get_back(v))) == 0)
    {
        /*  Remove an empty string from the end of the vector
         *  which appears after split when the last line ends
         *  with \n which is usually true.
         */
        ufree(G_AS_STR(uvector_pop_back(v)));
    }

    return G_PTR(v);
}

ugeneric_t ufile_open(const char *path, const char *mode)
{
    UASSERT_INPUT(path);
    UASSERT_INPUT(mode);

    FILE *file = fopen(path, mode);
    if (!file)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    return G_PTR(file);
}

ugeneric_t ufile_close(FILE *f)
{
    UASSERT_INPUT(f);

    if (0 != fclose(f))
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    return G_NULL;
}

ugeneric_t ufile_reader_create(const char *path, size_t buffer_size)
{
    UASSERT_INPUT(path);
    UASSERT_INPUT(buffer_size);

    ugeneric_t g;
    if (G_IS_ERROR(g = ufile_open(path, "r")))
    {
        return g;
    }
    ufile_reader_t *fr = umalloc(sizeof(*fr));
    fr->buffer = umalloc(buffer_size);
    fr->buffer_size = buffer_size;
    fr->file = G_AS_PTR(g);
    fr->read_offset = 0;

    if (G_IS_ERROR(g = _get_file_size(fr->file, true)))
    {
        return g;
    }

    fr->file_size = G_AS_SIZE(g);

    return G_PTR(fr);
}

/*
 * Read to memory provided by caller or to internal buffer
 * if last argument is NULL.
 */
ugeneric_t ufile_reader_read(ufile_reader_t *fr, size_t size, void *buffer)
{
    UASSERT_INPUT(fr);

    if (!buffer)
    {
        // Expand internal buffer if it is not sufficient.
        if (fr->buffer_size < size)
        {
            ufree(fr->buffer);
            fr->buffer = umalloc(size);
            fr->buffer_size = size;
        }
    }

    void *dst = buffer ? buffer : fr->buffer;
    size_t r = fread(dst, 1, size, fr->file);

    // Short read, either EOF reached or I/O error.
    if (r < size)
    {
        if (!feof(fr->file))
        {
            return _error_handler(G_ERROR_IO, _error_handler_ctx);
        }
    }
    fr->read_offset += r;
    return (buffer ? G_MEMCHUNK(buffer, r) : G_MEMCHUNK(fr->buffer, r));
}

bool ufile_reader_has_next(const ufile_reader_t *fr)
{
    return fr->read_offset < fr->file_size;
}

ugeneric_t ufile_reader_get_file_size(ufile_reader_t *fr)
{
    UASSERT_INPUT(fr);
    return _get_file_size(fr->file, true);
}

size_t ufile_reader_get_buffer_size(const ufile_reader_t *fr)
{
    UASSERT_INPUT(fr);
    return fr->buffer_size;
}

ugeneric_t ufile_reader_get_position(const ufile_reader_t *fr)
{
    UASSERT_INPUT(fr);
    return _get_position(fr->file);
}

ugeneric_t ufile_reader_set_position(ufile_reader_t *fr, size_t position)
{
    UASSERT_INPUT(fr);

    ugeneric_t g = _set_position(fr->file, position);
    if (!G_IS_ERROR(g))
    {
        fr->read_offset = position;
    }
    return g;
}

ugeneric_t ufile_reader_destroy(ufile_reader_t *fr)
{
    ugeneric_t g = G_NULL;
    if (fr)
    {
        g = ufile_close(fr->file);
        ufree(fr->buffer);
        ufree(fr);
    }

    return g;
}

ugeneric_t ufile_writer_create(const char *path)
{
    UASSERT_INPUT(path);

    ugeneric_t g;
    if (G_IS_ERROR(g = ufile_open(path, "w")))
    {
        return g;
    }
    ufile_writer_t *fw = umalloc(sizeof(*fw));
    fw->file = G_AS_PTR(g);

    return G_PTR(fw);
}

ugeneric_t ufile_writer_write(ufile_writer_t *fw, umemchunk_t mchunk)
{
    UASSERT_INPUT(fw);

    size_t size = fwrite(mchunk.data, 1, mchunk.size, fw->file);

    // Short write, either EOF reached or I/O error.
    if (size < mchunk.size)
    {
        return _error_handler(G_ERROR_IO, _error_handler_ctx);
    }

    return G_NULL;
}

ugeneric_t ufile_writer_get_file_size(ufile_writer_t *fw)
{
    UASSERT_INPUT(fw);
    return _get_file_size(fw->file, true);
}

ugeneric_t ufile_writer_get_position(const ufile_writer_t *fw)
{
    UASSERT_INPUT(fw);
    return _get_position(fw->file);
}

ugeneric_t ufile_writer_set_position(ufile_writer_t *fw, size_t position)
{
    UASSERT_INPUT(fw);
    return _set_position(fw->file, position);
}

ugeneric_t ufile_writer_destroy(ufile_writer_t *fw)
{
    ugeneric_t g = G_NULL;
    if (fw)
    {
        g = ufile_close(fw->file);
        ufree(fw);
    }

    return g;
}

void libugeneric_set_file_error_handler(ufile_error_handler_t error_handler,
                                        void *error_handler_ctx)
{
    _error_handler = error_handler;
    _error_handler_ctx = error_handler_ctx;
}
