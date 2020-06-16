# procsys

Once cloned run:
```bash
cmake [-DCMAKE_BUILD_TYPE=Debug] . # from root dir
make -j # for multithreaded make
mkdir <mountdir>
./src/main.c [-f -s -d] <mountdir> # Use flags for running in foreground, single threaded for debugging
```

## Introduction
This is the coding project for my CS bachelor thesis, the aim is to recreate procfs and sysfs on a distributed level using the FUSE API.

TODO:   
- [ ] 1: **Writing a filesystem that will mostly reflect what is in the procfs**

    ~- [ ] 1.1: Create the filesystem datastructures   
        - [x] 1.1.1: Tree (NODEs) to allow for parent, child and sibling node pointers for traversal, plus directory/file name [tree.c](src/tree.c)   
        - [x] 1.1.2: Queue to allow for the BFS search [queue.c](src/queue.c)   
        - [ ] 1.1.3: BFS algorithm~   

    - [ ] 1.2: Write simple bash ls/cat commands on the mounted filesystem   

    - [ ] 1.3: Be able to handle standard utility commands like iostat fstat etc

        - [ ] 1.3.1: Write bash (or c) scripts which replicate these commands to run on this new fs.   
            - [x] ' cat /proc/net/dev '       - Packet stats [packet-bytes](test/packet-bytes.sh)   
            - [ ] ' cat /proc/net/netstat '   - Tcp stats. First ones are counters   
            - [ ] ' ifconfig '                - Getting info from above files   
            - [ ] ' ifstat '                  - Periodically reading the files above and subtracting each window to show what has been transmitted per second   
            - [ ] Replicate some other networking utilities - [info about proc files (1.4 is networking)](https://www.kernel.org/doc/Documentation/filesystems/proc.txt)

- [ ] 2: **Combine the procfs of two file systems**   
      Assume there is a server and the rest are client filesystems. Assume already
      established tcp connection between the server (my machine) where I am running
      the uberproc, and the client machine. This must be done in the FUSE fs code.
      When there is a cat command on one of the files within the new fs (which is
      mounted in /home NOT in /proc) it will get the info from the local procfs,
      then get the info from the connected client in the same way and congregate the
      results according to the requesting command (data specific). Can get 5 or 6
      popular files (ones that are requested from /proc the most).
      Cheap and dirty: there is a server and all others are clients are connected
      (basic client/server model that I assumed)
 
- [ ] 3: **Go from 2 to n multiple clients**   
      Should be smooth if using multiple vagrant vms as clients to test

- [ ] 4: **Hijacking**   
      Fakeroot pretends that you are root. It will give a different file system
      overview as what's actually there, like a container/VM. Then any command run
      within this fakeroot follows that structure. So could replace /proc with
      uberproc in the fakeroot and then calls made within this fakeroot will think
      that uberproc is actually /proc.
      Can use strace <command> to see a syscall trace to see what files etc are
      being accessed by that command, this helps with debugging.
      
- [ ] 5: **Do the same for sysfs**

- [ ] 6: **Benchmarking**
