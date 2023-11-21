# Theta sketches in SingleStoreDB

## Introduction

[Theta sketches](https://datasketches.apache.org/docs/Theta/ThetaSketchFramework.html) are a probabilistic data structure intended to approximate the cardinality of all common set operations,
e.g. union, intersection, ..

## Contents
This library provides the following User Defined Aggregates (UDAFs) and UDFs (User Defined Functions).

### `theta_sketch_agg` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_AGG(col)`
- **Arguments**: The column from which to create a theta sketch.
- **Return Type**: The theta sketch representation as a BLOB.
- **Description**: This is a UDAF that will generate a theta sketch from a column of data and return it as a serialized blob.

### `sketch_estimate` (UDF)
- **Type**: Scalar Function
- **Syntax**: `SKETCH_ESTIMATE(blob)`
- **Arguments**: A theta sketch blob generated using `theta_sketch_agg`.
- **Return Type**: The theta sketch estimate, as a `DOUBLE`.
- **Description**: This is a UDF that takes a single serialized theta sketch and estimates the number of unique samples in it.

### `sketch_to_string` (UDF)
- **Type**: Scalar Function
- **Syntax**: `SKETCH_TO_STRING(blob)`
- **Arguments**: A theta sketch blob generated using `theta_sketch_agg`.
- **Return Type**: A string containing diagnostic information.
- **Description**: This is a UDF that takes a single serialized theta sketch and returns a string containing diagnostic information about it.

## Building
The Wasm module can be built using the following commands.  The build requires you to first install the [Wasm WASI SDK](https://github.com/WebAssembly/wasi-sdk).  The included Makefile assumes that the version of `clang` from the WASI SDK directory is in your path.  Replace the variable `$WASI_SDK_PATH` below with this directory.
```bash
# Make sure the WASI SDK binaries are in your PATH.
export PATH=$WASI_SDK_PATH/bin:$PATH

# Compile the Wasm module.
make release
```
The binary will be placed in the file `./extension.wasm`.

## Deployment to SingleStoreDB
To install these functions using the MySQL client, use the following commands.  They assume you have built the Wasm module and your current directory is the root of this Git repo.  Replace '$DBUSER`, `$DBHOST`, `$DBPORT`, and `$DBNAME` with, respectively, your database username, hostname, port, and the name of the database where you want to deploy the functions.
```bash
mysql -u $DBUSER -h $DBHOST -P $DBPORT -D $DBNAME -p < load_extension.sql
```

### Usage
The following is simple example that creates a table with two columns of integers.  It generates a union-based theta sketch for each column, merges them into another union, and then computes the estimate of the merged theta sketch.
```sql
CREATE TABLE IF NOT EXISTS sketch_input(id1 int, id2 int);
INSERT INTO sketch_input VALUES (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);

SELECT sketch_estimate(theta_sketch_agg(id1)) FROM sketch_input;
```

This next example is similar to the above one, except that it shows how to "save" the theta sketches in User-Defined Variables so they can be re-used.
```sql
CREATE TABLE IF NOT EXISTS sketch_input(id1 int, id2 int);
INSERT INTO sketch_input VALUES (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);

SELECT theta_sketch_agg(id1) FROM sketch_input INTO @sketch1;
SELECT sketch_estimate(@sketch1);
```

## Additional Information
To learn about the process of developing a Wasm UDF or TVF in more detail, please have a look at our [tutorial](https://singlestore-labs.github.io/singlestore-wasm-toolkit/html/Tutorial-Overview.html).

The SingleStoreDB Wasm UDF/TVF documentation is [here](https://docs.singlestore.com/managed-service/en/reference/code-engine---powered-by-wasm.html).

## Acknowledgements
This implementation is based on https://github.com/apache/datasketches-postgresql.

## Resources
* [Theta Sketches](https://datasketches.apache.org/docs/Theta/ThetaSketchFramework.html)
* [Documentation](https://docs.singlestore.com)
* [Twitter](https://twitter.com/SingleStoreDevs)
* [SingleStore forums](https://www.singlestore.com/forum)
* [SingleStoreDB extension template for Rust](https://github.com/singlestore-labs/singlestoredb-extension-rust-template)

