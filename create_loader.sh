EXTENSION_NAME=$( find . -iname "*.wit" -exec basename {} .wit ';')
WASM_B64=$(cat ${EXTENSION_NAME}.wasm | base64 -w 0)
WIT_B64=$(cat ${EXTENSION_NAME}.wit | base64 -w 0)

CMD=

for MERGE_OP in union intersect anotb ; do
    CMD="$CMD 
CREATE OR REPLACE AGGREGATE theta_sketch_create_${MERGE_OP}(int NOT NULL)
RETURNS LONGBLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH theta_sketch_handle_init
ITERATE WITH theta_sketch_handle_update
MERGE WITH theta_sketch_handle_${MERGE_OP}
TERMINATE WITH theta_sketch_handle_serialize
SERIALIZE WITH theta_sketch_handle_serialize
DESERIALIZE WITH theta_sketch_handle_deserialize
COPYMERGE WITH theta_sketch_handle_${MERGE_OP}_copy
CLONE WITH theta_sketch_handle_clone
DESTROY WITH theta_sketch_handle_destroy;\n"

    CMD="$CMD
CREATE OR REPLACE AGGREGATE theta_sketch_estimate_${MERGE_OP}(int NOT NULL)
RETURNS DOUBLE NOT NULL
WITH STATE HANDLE
AS WASM FROM BASE64 '$WASM_B64'
WITH WIT FROM BASE64 '$WIT_B64'
INITIALIZE WITH theta_sketch_handle_init
ITERATE WITH theta_sketch_handle_update
MERGE WITH theta_sketch_handle_${MERGE_OP}
TERMINATE WITH theta_sketch_handle_estimate
SERIALIZE WITH theta_sketch_handle_serialize
DESERIALIZE WITH theta_sketch_handle_deserialize
COPYMERGE WITH theta_sketch_handle_${MERGE_OP}_copy
CLONE WITH theta_sketch_handle_clone
DESTROY WITH theta_sketch_handle_destroy;\n"

    CMD="$CMD
CREATE OR REPLACE FUNCTION theta_sketch_merge_${MERGE_OP} AS WASM FROM BASE64 '$WASM_B64' WITH WIT FROM BASE64 '$WIT_B64' USING EXPORT 'theta-sketch-${MERGE_OP}';\n"
done

CMD="$CMD
CREATE OR REPLACE FUNCTION theta_sketch_apply AS WASM FROM BASE64 '$WASM_B64' WITH WIT FROM BASE64 '$WIT_B64' USING EXPORT 'theta-sketch-estimate';\n"

echo "$CMD" > load_extension.sql
