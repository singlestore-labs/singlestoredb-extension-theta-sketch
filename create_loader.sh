#!/usr/bin/bash

EXTENSION_NAME=$( find . -iname "*.wit" -exec basename {} .wit ';')
WASM_B64=$(cat ${EXTENSION_NAME}.wasm | base64 -w 0)
WIT_B64=$(cat ${EXTENSION_NAME}.wit | base64 -w 0)

rm -f load_extension.sql

cat <<EOF >> load_extension.sql
CREATE OR REPLACE AGGREGATE theta_sketch_build_agg(LONGBLOB NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH sketch_handle_init
ITERATE WITH sketch_handle_build_accum
MERGE WITH sketch_handle_union_merge
TERMINATE WITH sketch_handle_serialize
SERIALIZE WITH sketch_handle_serialize
DESERIALIZE WITH sketch_handle_deserialize;

EOF

cat <<EOF >> load_extension.sql
CREATE OR REPLACE AGGREGATE theta_sketch_build_raw_agg(BIGINT UNSIGNED NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH sketch_handle_init
ITERATE WITH sketch_handle_build_accum_raw
MERGE WITH sketch_handle_union_merge
TERMINATE WITH sketch_handle_serialize
SERIALIZE WITH sketch_handle_serialize
DESERIALIZE WITH sketch_handle_deserialize;

EOF

cat <<EOF >> load_extension.sql
CREATE OR REPLACE AGGREGATE theta_sketch_union_agg(LONGBLOB NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH sketch_handle_init
ITERATE WITH sketch_handle_union_accum
MERGE WITH sketch_handle_union_merge
TERMINATE WITH sketch_handle_serialize
SERIALIZE WITH sketch_handle_serialize
DESERIALIZE WITH sketch_handle_deserialize;

EOF

cat <<EOF >> load_extension.sql
CREATE OR REPLACE AGGREGATE theta_sketch_intersection_agg(LONGBLOB NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH sketch_handle_init
ITERATE WITH sketch_handle_intersection_accum
MERGE WITH sketch_handle_intersection_merge
TERMINATE WITH sketch_handle_serialize
SERIALIZE WITH sketch_handle_serialize
DESERIALIZE WITH sketch_handle_deserialize;

EOF

VALUE_NAMES=($(cat ${EXTENSION_NAME}.wit | grep 'func(' | grep -v handle | sed -E -e 's/([\w]*):.*/\1/g' | sed 's/-/_/g'))
VALUE_EXPORTS=($(cat ${EXTENSION_NAME}.wit | grep 'func(' | grep -v handle | sed -E -e 's/([\w]*):.*/\1/g'))

for I in `seq 1 ${#VALUE_NAMES[@]}` ; do
    cat <<EOF >> load_extension.sql
CREATE OR REPLACE FUNCTION theta_${VALUE_NAMES[$I-1]}
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
USING EXPORT '${VALUE_EXPORTS[$I-1]}';

EOF
done

