#!/usr/bin/env python3

import json

# You can obtain source here: https://hg.nginx.org/nginx/raw-file/default/conf/mime.types

def main():
    TYPES = []
    with open("mime.types", "rt") as f:
        for line in f:
            ll = line.replace(';', '').split(maxsplit = 1)
            if len(ll) > 1:
                vals = ll[1].split()
                for v in vals:
                    TYPES.append( (v, ll[0]) )

    print("static struct MIME {")
    print("    const char* ext;")
    print("    const char* mime;")
    print("} MIME_TYPES[] = {")
    for ext, mime in TYPES:
        print("    { %s, %s }," % (json.dumps(ext), json.dumps(mime)))
    print("    { NULL, NULL }")
    print("}")

if __name__ == '__main__':
    main()
