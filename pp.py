class GenericsPrettyPrinter:
    ''' Pretty printing of generic_t instances.
    '''
    def __init__(self, val):
        self.val = val

    def to_string (self):
        t = self.val['type']['type']
        v = self.val['value']

        return str(t)[:-2] + "{" + {
            "G_ERROR_T": str(v['err']),
            "G_NULL_T":  "",
            "G_PTR_T":   str(v['ptr']),
            "G_STR_T":   str(v['str']),
            "G_CSTR_T":  str(v['cstr']),
            "G_INT_T":   str(v['integer']),
            "G_REAL_T":  str(v['real']),
            "G_CHR_T":   str(v['chr']),
            "G_SIZE_T":  str(v['size']),
            "G_BOOL_T":  str(v['boolean']),
            "G_VECTOR_T": str(v['ptr']),
            "G_DICT_T":   str(v['ptr']),
            "G_MCHUNK_T": str(v['ptr']),
        }.get(str(t), "unknown") + "}"

def print_generic_t(val):
    if str(val.type) == 'generic_t':
        return GenericsPrettyPrinter(val)
    return None

gdb.pretty_printers.append(print_generic_t)
