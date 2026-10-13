import struct

def pack_into(fmt: str|bytes, buffer, offset: int, *v) -> None:
    """
    Pack the values v1, v2, ... according to the format string and write\n
    the packed bytes into the writable buffer buf starting at offset. Note\n
    that the offset is a required argument. See help(struct) for more\n
    on format strings.\n
    """

    struct.pack_into(fmt, buffer, offset, *v)

def unpack_from(fmt: str|bytes, buffer, offset: int = 0) -> tuple:
    """
    Unpack from the buffer starting at offset according to the format\n
    string fmt. The result is a tuple even if it contains exactly one\n
    item. See help(struct) for more on format strings.\n
    """

    return struct.unpack_from(fmt, buffer, offset)