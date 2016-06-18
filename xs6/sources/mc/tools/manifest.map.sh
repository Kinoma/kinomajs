#!/bin/sh

cd $2
list=`ls -1`

(\
    i=0; \
    for f in $list; do echo "static const unsigned char _$i[] = { "; xxd -i < $f; echo "};"; i=`expr $i + 1`; done; \
    echo "static const struct {const char *name; const unsigned char *data; unsigned int size;} mc_mapped_files[] = {"; \
    i=0; \
    for f in $list; do echo "{\"$f\", _$i, sizeof(_$i)},"; i=`expr $i + 1`; done; \
    echo "};" \
) > $1
