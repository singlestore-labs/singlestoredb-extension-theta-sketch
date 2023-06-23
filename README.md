# Theta sketches in SingleStoreDB

## Introduction

[Theta sketches](https://datasketches.apache.org/docs/Theta/ThetaSketchFramework.html) are a probabilistic data structure intended to approximate the cardinality of all common set operations,
e.g. union, intersection, ..

## Contents
This library provides the following User Defined Aggregates (UDAFs) and UDFs (User Defined Functions).

### `theta_sketch_create_union` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_CREATE_UNION(col)`
- **Arguments**: The column from which to create a union theta sketch.
- **Return Type**: The theta sketch representation as a BLOB.
- **Description**: This is a UDAF that will generate a theta sketch from a column of data and return it as a serialized blob.  Sketches are merged using a `union` operation.  This blob can be used with the included UDFs, discussed below.

### `theta_sketch_create_intersect` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_CREATE_INTERSECT(col)`
- **Arguments**: The column from which to create an intersection theta sketch.
- **Return Type**: The theta sketch representation as a BLOB.
- **Description**: This is a UDAF that will generate a theta sketch from a column of data and return it as a serialized blob.  Sketches are merged using a `union` operation.  This blob can be used with the included UDFs, discussed below.

### `theta_sketch_create_anotb` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_CREATE_ANOTB(col)`
- **Arguments**: The column from which to create an a-not-b theta sketch.
- **Return Type**: The theta sketch representation as a BLOB.
- **Description**: This is a UDAF that will generate a theta sketch from a column of data and return it as a serialized blob.  Sketches are merged using a `union` operation.  This blob can be used with the included UDFs, discussed below.

### `theta_sketch_estimate_union` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_ESTIMATE_UNION(col)`
- **Arguments**: The column from which to estimate using a union theta sketch.
- **Return Type**: The final theta sketch estimate, as a `DOUBLE`.
- **Description**: This is a UDAF that will implicitly generate a theta sketch from a column of data and return the estimate as a double.  Sketches are merged using the `union` operation.

### `theta_sketch_estimate_intersect` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_ESTIMATE_INTERSECT(col)`
- **Arguments**: The column from which to estimate using an intersection theta sketch.
- **Return Type**: The final theta sketch estimate, as a `DOUBLE`.
- **Description**: This is a UDAF that will implicitly generate a theta sketch from a column of data and return the estimate as a double.  Sketches are merged using the `intersect` operation.

### `theta_sketch_estimate_anotb` (UDAF)
- **Type**: Aggregate
- **Syntax**: `THETA_SKETCH_ESTIMATE_ANOTB(col)`
- **Arguments**: The column from which to estimate using an a-not-b theta sketch.
- **Return Type**: The final theta sketch estimate, as a `DOUBLE`.
- **Description**: This is a UDAF that will implicitly generate a theta sketch from a column of data and return the estimate as a double.  Sketches are merged using the `anotb` operation.

### `theta_sketch_merge_union` (UDF)
- **Type**: Scalar Function
- **Syntax**: `THETA_SKETCH_MERGE_UNION(blob, blob)`
- **Arguments**: Two theta sketch blobs generated using the aggregate functions above.
- **Return Type**: A new theta sketch `BLOB` generated using the union operation.
- **Description**: This is a UDF that takes two serialized theta sketch blobs and performs a `union` operation on them.  A new serialized theta sketch blob is returned.

### `theta_sketch_merge_intersect` (UDF)
- **Type**: Scalar Function
- **Syntax**: `THETA_SKETCH_MERGE_INTERSECT(blob, blob)`
- **Arguments**: Two theta sketch blobs generated using the aggregate functions above.
- **Return Type**: A new theta sketch `BLOB` generated using the intersection operation.
- **Description**: This is a UDF that takes two serialized theta sketch blobs and performs an `intersect` operation on them.  A new serialized theta sketch blob is returned.

### `theta_sketch_merge_anotb` (UDF)
- **Type**: Scalar Function
- **Syntax**: `THETA_SKETCH_MERGE_ANOTB(blob, blob)`
- **Arguments**: Two theta sketch blobs generated using the aggregate functions above.
- **Return Type**: A new theta sketch `BLOB` generated using the a-not-b operation.
- **Description**: This is a UDF that takes two serialized theta sketch blobs and performs a `NOT IN` operation on them.  A new serialized theta sketch blob is returned.

### `theta_sketch_apply` (UDF)
- **Type**: Scalar Function
- **Syntax**: `THETA_SKETCH_APPLY(blob)`
- **Arguments**: One theta sketch blobs generated using one of the aggregate functions described above.
- **Return Type**: The theta sketch estimate, as a `DOUBLE`.
- **Description**: This is a UDF that takes a single serialized theta sketch and estimates the number of unique samples in it.

## Building
The Wasm module can be built using the following commands.  The build requires you to first install the [Wasm WASI SDK](https://github.com/WebAssembly/wasi-sdk).  The included Makefile assumes that the version of `clang` from the WASI SDK directory is in your path.  Replace the variable `$WASI_SDK_PATH` below with this directory.
```bash
# Make sure the WASI SDK binaries are in your PATH.
export PATH=$WASI_SDK_PATH/bin:$PATH

# Compile the Wasm module.
make
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

SELECT theta_sketch_apply(theta_sketch_merge_union(theta_sketch_create_union(id1), theta_sketch_create_union(id2))) FROM sketch_input;
```

This next example is similar to the above one, except that it shows how to "save" the theta sketches in User-Defined Variables so they can be re-used.
```sql
CREATE TABLE IF NOT EXISTS sketch_input(id1 int, id2 int);
INSERT INTO sketch_input VALUES (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);

SELECT theta_sketch_create_union(id1) FROM sketch_input INTO @sketch1;
SELECT theta_sketch_create_union(id2) FROM sketch_input INTO @sketch2;

SELECT theta_sketch_apply(theta_sketch_merge_union(@sketch1, @sketch2));
SELECT theta_sketch_apply(theta_sketch_merge_intersect(@sketch1, @sketch2));
```

This example implicitly generates a theta sketch from a column and then immediately returns the estimate.
```sql
CREATE TABLE IF NOT EXISTS sketch_input(id1 int, id2 int);
INSERT INTO sketch_input VALUES (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);

SELECT theta_sketch_estimate_union(id1) FROM sketch_input;
```

## Additional Information
To learn about the process of developing a Wasm UDF or TVF in more detail, please have a look at our [tutorial](https://singlestore-labs.github.io/singlestore-wasm-toolkit/html/Tutorial-Overview.html).

The SingleStoreDB Wasm UDF/TVF documentation is [here](https://docs.singlestore.com/managed-service/en/reference/code-engine---powered-by-wasm.html).

## Resources
* [Theta Sketches](https://datasketches.apache.org/docs/Theta/ThetaSketchFramework.html)
* [Documentation](https://docs.singlestore.com)
* [Twitter](https://twitter.com/SingleStoreDevs)
* [SingleStore forums](https://www.singlestore.com/forum)
* [SingleStoreDB extension template for Rust](https://github.com/singlestore-labs/singlestoredb-extension-rust-template)

