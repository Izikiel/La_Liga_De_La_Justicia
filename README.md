La_Liga_De_La_Justicia
======================

Academic project of the operating systems subject at utn frba.
It's a remote filesystem server with a client and a cache.

This project consists of 3 different parts: A client, a server, and a cache.

The client was built by Federico Gabriel Lopez Luksenberg, using fuse. He also built the communications library
between the client and the server.

The cache was built by Aldana Laura Quintana Munilla, using memcached and a couple of linked lists. 
One of the caveats of the project was that you could only allocate memory for the cache at the beggining of the program,
and that was all the memory you were allowed while running the cache, so Aldana had to create a couple of lists to manage
that memory.

The server was built by Ezequiel Dario Gambaccini. It consisted of implementing Ext 2 revision 0 with sparse superblock 
for managing the file system, as well as the typical file system operations:
  
Files -> create                     Directories -> list
          read                                     create
          write                                    delete
          delete
          open
          close
          truncate 

The server also had to manage concurrency, as it allowed having connections from several clients at a time.

Server_Threading.h and .c are the files which handle concurrence.
