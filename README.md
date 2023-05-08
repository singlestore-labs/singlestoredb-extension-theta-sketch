# Theta sketches in SingleStoreDB

## Introduction

[Theta sketches](https://datasketches.apache.org/docs/Theta/ThetaSketchFramework.html) are a probabilistic data structure intended to approximate the cardinality of all common set operations,
e.g. union, intersection, ..

## Usage in SingleStore

The library can import the following wasm functions usable with an aggregate function:
* `sketch_init_handle`: initializes an empty sketch
* `sketch_update_handle`: updates the sketch with a new sample
* `sketch_union_handle`: merges two sketches through a `union` operation
* `sketch_intersect_handle`: merges two sketches through an `intersect` operation
* `sketch_anotb_handle`: merges two sketches through a `NOT IN` operation
* `sketch_estimate_handle`: returns the estimated number of unique samples in the sketch
* `sketch_serialize_handle`: serializes a sketch into its compact binary form
* `sketch_deserialize_handle`: deserializes a sketch from its compact binary form 

Note: These functions assume they're called within an aggregate context with `HANDLE` state.

in addition the aggregate function `theta_sketch` can be created as follows:
```sql
CREATE AGGREGATE theta_sketch(int NOT NULL)
RETURNS BLOB NOT NULL
WITH STATE HANDLE
AS WASM FROM LOCAL INFILE "extension.wasm"
INITIALIZE WITH sketch_init_handle
ITERATE WITH sketch_update_handle
MERGE WITH sketch_union_handle
TERMINATE WITH <sketch_estimate_handle | sketch_serialize_handle>
SERIALIZE WITH sketch_serialize_handle
DESERIALIZE WITH sketch_deserialize_handle;
```

In addition the following [UDFs](https://docs.singlestore.com/managed-service/en/reference/code-engine---powered-by-wasm.html) are available which operate
on serialized sketches:

* `sketch_union`: Merges two serialized sketches through a `union` operation
* `sketch_intersect`: Merges two serialized sketches through an `intersect` operation
* `sketch_anotb`: Merges two serialized sketches through a `NOT IN` operation
* `sketch_estimate`: Returns the estimate number of unique samples in the serialized sketch

### Examples

An example set of `MySQL` statements can be found in `test.sql`:
```sql
create table IF NOT EXISTS sketch_input (id1 int, id2 int);
insert into sketch_input values
    (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);

select sketch_estimate(sketch_union(theta_sketch(id1), theta_sketch(id2))) from sketch_input;
```

## Tools

To use the tools in this repo, you will need to have Docker installed on your system.  Most of these tools can be installed locally as well:

### [clang](https://clang.llvm.org)
The Clang compiler and toolchain.  The exact compiler version may differ between containers; see below for specifics.

### [rust/cargo](https://www.rust-lang.org)
The Rust compiler with the *stable* toolchain.  It has been configured with compilation targets for the default platform: *wasm32-wasi*.

### [WASI SDK](https://github.com/WebAssembly/wasi-sdk)
Utilities to support the WASI toolchain.

### [WIT bindgen](https://github.com/WebAssembly/wasi-sdk)
A language binding generator for the WIT IDL.

## Useful other tools

### [writ](https://github.com/singlestore-labs/writ)
A utility to help test Wasm functions locally without the need to create a separate driver program.  Please see its dedicated [Git Repository](https://github.com/singlestore-labs/writ) for more information.

### [pushwasm](https://github.com/singlestore-labs/pushwasm)
A utility that allows you to easily import your locally-built Wasm function into SingleStoreDB as a UDF or TVF.  Please see its dedicated [Git Repository](https://github.com/singlestore-labs/pushwasm) for more information.

## Development

The project provides a simple `Makefile` which can be run to automatically generate the C++ bindings, compile the sources into a Wasm binary & generate the SQL import scripts to load the UDFs/TVFs into a SingleStore instance.

Alternatively each step can be done individually using the following workflow:

1. Start with the module interface, as defined in the [extension.wit](https://github.com/singlestore-labs/singlestoredb-extension-cpp-template/blob/main/extension.wit) file. Using the `wit-bindgen` tool you can generate the C stubs required to call the newly defined Wasm functions: 
    ```sh
    wit-bindgen c --export extension.wit
    ```

1. Compile your program using the clang compiler provided by `WASI-SDK`:
    ```sh
    clang++                          \
        -fno-exceptions              \
        --target=wasm32-unknown-wasi \
        -mexec-model=reactor         \
        -I.                          \
        -o extension.wasm            \
        extension.cpp extension_impl.cpp
    ```

1. Once the Wasm binary has been created, you can create a SQL import statement using the [create_loader.sh](https://github.com/singlestore-labs/singlestoredb-extension-cpp-template/blob/main/create_loader.sh) script. This creates a `load_extension.sql` file importing the TVF/UDFs using Base64:
    ```sh
    mysql -h <my-host> -u <my-yser> -p -D <my-database> < load_extension.sql
    ```
    Alternatively you can use the `pushwasm` tool to push a single UFD/TVF:
    ```sh
    pushwasm udaf -n theta_sketch --wasm extension.wasm --wit extension.wit --abi canonical --conn 'mysql://<my-user>@<my-host>:3306/<my-database>'
        --state HANDLE \
        --type 'int not null' \
        --arg 'int not null' \
        --init sketch_init_handle \
        --iter sketch_update_handle \ 
        --merge sketch_intersect_handle \ 
        --terminate sketch_estimate_handle \
        --serialize sketch_serialize_handle \
        --deserialize sketch_deserialize_handle
    ```

## Additional Information

To learn about the process of developing a Wasm UDF or TVF in more detail, please have a look at our [tutorial](https://singlestore-labs.github.io/singlestore-wasm-toolkit/html/Tutorial-Overview.html).

The SingleStoreDB Wasm UDF/TVF documentation is [here](https://docs.singlestore.com/managed-service/en/reference/code-engine---powered-by-wasm.html).

## Resources

* [Documentation](https://docs.singlestore.com)
* [Twitter](https://twitter.com/SingleStoreDevs)
* [SingleStore forums](https://www.singlestore.com/forum)
* [SingleStoreDB extension template for Rust](https://github.com/singlestore-labs/singlestoredb-extension-rust-template)

