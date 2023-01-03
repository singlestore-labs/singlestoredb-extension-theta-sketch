EXTENSION_NAME=$( find . -iname "*.wit" -exec basename {} .wit ';')
WASM_B64=$(cat ${EXTENSION_NAME}.wasm | base64 -w 0)
WIT_B64=$(cat ${EXTENSION_NAME}.wit | base64 -w 0)
WIT_FUNCTIONS=$(cat ${EXTENSION_NAME}.wit | grep 'func(' | sed -E -e 's/([\w]*):.*/\1/g' | sed 's/-/_/g')
CMD=""
for func in $WIT_FUNCTIONS
do
  CMD="$CMD CREATE OR REPLACE FUNCTION $func AS WASM FROM BASE64 '$WASM_B64' WITH WIT FROM BASE64 '$WIT_B64';\n"
done

CMD="$CMD CREATE OR REPLACE AGGREGATE theta_sketch(int NOT NULL)
RETURNS BLOB NOT NULL
WITH STATE BLOB NOT NULL
INITIALIZE WITH sketch_init
ITERATE WITH sketch_update
MERGE WITH sketch_union
TERMINATE WITH sketch_finalize;"

echo "$CMD" > load_extension.sql
