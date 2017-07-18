# Welcome to libpickup

Libpickup is a shared library to build tinder applications

It is realeased with a sample application and a complete command line interface
!

## Dependecies:
  - webkitgtk-3.0
  - yajl
  - libcurl
  - sqlite3

## You can check your dependencies with :

```
$ pkg-config <pkg-name>
```

## How to use it (cli or sample) :

  * Create the sqlite3 database in ~/.config/pickup/pickup.db with the provided
    schema

  * Create the image cache directory in ~/.cache/pickup/img

  * The lazy boy may do:

```
$ LD_LIBRARY_PATH=binary/libpickup/:binary/liboauth2webkit/ binary/cli/xml --help
```

  * The power user may add in its shell rc
    binary/cli to its PATH and libs to LD_LIBRARY_PATH environment variables :

```
export PATH=$PATH:<right_dir>/binary/cli
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<right_dir>/binary/libpickup:<right_dir>/binary/liboauth2webkit
```
