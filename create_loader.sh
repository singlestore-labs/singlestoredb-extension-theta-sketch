EXTENSION_NAME=$( find . -iname "*.wit" -exec basename {} .wit ';')
WASM_B64=$(cat ${EXTENSION_NAME}.wasm | base64 -w 0)
WIT_B64=$(cat ${EXTENSION_NAME}.wit | base64 -w 0)

CMD="CREATE OR REPLACE AGGREGATE theta_sketch(int NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH sketch_init
ITERATE WITH sketch_update
MERGE WITH sketch_union
TERMINATE WITH sketch_serialize
SERIALIZE WITH sketch_serialize
DESERIALIZE WITH sketch_deserialize;\n"

CMD="$CMD CREATE OR REPLACE FUNCTION sketch_estimate AS WASM FROM BASE64 '$WASM_B64' WITH WIT FROM BASE64 '$WIT_B64';\n"

echo "$CMD" > load_extension.sql
