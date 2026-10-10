from pathlib import Path

VALTYPES = {0x7f: 'i32', 0x7e: 'i64', 0x7d: 'f32', 0x7c: 'f64', 0x7b: 'v128', 0x70: 'funcref', 0x6f: 'externref'}


def leb(data, pos, signed=False):
    result = shift = 0
    while True:
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7f) << shift
        shift += 7
        if not byte & 0x80:
            if signed and byte & 0x40:
                result -= 1 << shift
            return result, pos


def name(data, pos):
    length, pos = leb(data, pos)
    return data[pos:pos + length].decode('utf8', 'replace'), pos + length


def signatures(path):
    data = Path(path).read_bytes()
    assert data[:4] == b'\0asm'
    pos = 8
    types, functions, names = [], [], {}
    imported = 0
    while pos < len(data):
        section = data[pos]
        size, pos = leb(data, pos + 1)
        end = pos + size
        if section == 1:
            count, p = leb(data, pos)
            for _ in range(count):
                assert data[p] == 0x60
                p += 1
                n, p = leb(data, p)
                params = [VALTYPES[data[p + i]] for i in range(n)]
                p += n
                n, p = leb(data, p)
                results = [VALTYPES[data[p + i]] for i in range(n)]
                p += n
                types.append((tuple(params), tuple(results)))
        elif section == 2:
            count, p = leb(data, pos)
            for _ in range(count):
                _, p = name(data, p)
                _, p = name(data, p)
                kind = data[p]
                p += 1
                if kind == 0:
                    index, p = leb(data, p)
                    functions.append(index)
                    imported += 1
                elif kind == 1:
                    p += 1
                    flags, p = leb(data, p)
                    _, p = leb(data, p)
                    if flags & 1:
                        _, p = leb(data, p)
                elif kind == 2:
                    flags, p = leb(data, p)
                    _, p = leb(data, p)
                    if flags & 1:
                        _, p = leb(data, p)
                elif kind == 3:
                    p += 2
                else:
                    raise ValueError(kind)
        elif section == 3:
            count, p = leb(data, pos)
            for _ in range(count):
                index, p = leb(data, p)
                functions.append(index)
        elif section == 0:
            section_name, p = name(data, pos)
            if section_name == 'name':
                while p < end:
                    kind = data[p]
                    sub, p = leb(data, p + 1)
                    sub_end = p + sub
                    if kind == 1:
                        count, q = leb(data, p)
                        for _ in range(count):
                            index, q = leb(data, q)
                            value, q = name(data, q)
                            names[value] = index
                    p = sub_end
        pos = end
    result = {}
    for value, index in names.items():
        params, results = types[functions[index]]
        result[value] = '(' + ','.join(params) + ')->' + (','.join(results) or 'void')
    return result

