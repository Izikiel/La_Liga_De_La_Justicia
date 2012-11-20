

# Remote File System
```
.
├── rfs
│   ├── headers_RFS 
|   |── source_RFS  
│   └── README.md
└── README.md
```

* Server_Threading.h and .c are the files which handle concurrence.

* ext2_block_types.h and .c are the files where the ext2 specification is implemented.

* ext2_def.h is a file where some specific ext2 values are defined as macros.

* manage_fs.h and .c are the files which handle the memory mapping of the filesytem.

* procesador_de_pedidos.h and .c are the files which handle the communication between the rfs and the fsc (written
  by Federico Lopez Luksenberg).

* file_operations.h and .c are the files which handle the operations with the filesystem, such as creating files, reading files, etc...