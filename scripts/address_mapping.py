#!/usr/bin/env python3

import textwrap


key_name = {
    'ch': 'channel',
    'ra': 'rank',
    'bg': 'bankgroup',
    'ba': 'bank',
    'ro': 'row',
    'co': 'column'
            }

mapping_list = ["chrarocobabg",  # chan:rank:row:col:bank
                "chrocobabgra",  # chan:row:col:bank:rank
                "chrababgcoro",  # chan:rank:bank:col:row
                "chrababgroco",  # chan:rank:bank:row:col
                "chrocorababg",  # chan:row:col:rank:bank
                "chrobabgraco",  # chan:row:bank:rank:col
                "rocorababgch",  # row:col:rank:bank:chan
                ]


def generate_code(item, is_last):
    expand_item = key_name[item]
    line1 = "    auto {} = ModuloWidth(hex_addr, config.{}_width_, pos);".format(expand_item, expand_item)
    print(line1)
    if not is_last:
        line2 = "    pos += config.{}_width_;".format(expand_item)
        print(line2)


def ending():
    print("else {")
    print('    cerr << "Unknown address mapping" << endl;')
    print("    exit(-1);")
    print("}")

for i in range(0, len(mapping_list)):
    if i != 0:
        print("else ", end="")
    mapping_type = mapping_list[i]
    parts = textwrap.wrap(mapping_type, 2)
    comment = '// {}:{}:{}:{}:{}:{}'.format(key_name[parts[0]], key_name[parts[1]], key_name[parts[2]], key_name[parts[3]], key_name[parts[4]], key_name[parts[5]])
    print('if(config.address_mapping == "{}") {{  {}'.format(mapping_type, comment))
    for j in range(len(parts)-1, -1, -1):
        if j != 0:
            generate_code(parts[j], False)
        else:
            generate_code(parts[j], True)
    print('    return Address(channel, rank, bankgroup, bank, row, column);')
    print('}')

ending()


