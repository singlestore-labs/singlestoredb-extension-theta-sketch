EXTENSION_NAME=$( find . -iname "*.wit" -exec basename {} .wit ';')
WASM_B64=$(cat ${EXTENSION_NAME}.wasm | base64 -w 0)
WIT_B64=$(cat ${EXTENSION_NAME}.wit | base64 -w 0)

rm -f load_extension.sql

cat <<EOF >> load_extension.sql
CREATE OR REPLACE AGGREGATE theta_sketch_agg(LONGBLOB NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH sketch_agg_init_handle
ITERATE WITH sketch_agg_update_handle
MERGE WITH sketch_agg_merge_handle
TERMINATE WITH sketch_agg_serialize_handle
SERIALIZE WITH sketch_agg_serialize_handle
DESERIALIZE WITH sketch_agg_deserialize_handle;

EOF

VALUE_UDFS=$(cat ${EXTENSION_NAME}.wit | grep 'func(' | grep -v handle | sed -E -e 's/([\w]*):.*/\1/g' | sed 's/-/_/g')
for func in $VALUE_UDFS
do
    cat <<EOF >> load_extension.sql
CREATE OR REPLACE FUNCTION $func AS WASM FROM BASE64 '$WASM_B64' WITH WIT FROM BASE64 '$WIT_B64';

EOF
done

