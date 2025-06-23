def str_to_ascii(string: str):
    n = len(string)
    bites = n * 8
    value = 0
    for c in string:
        value |= (ord(c) << bites)
        bites -= 8
    return value

print(str_to_ascii("ismael"))
