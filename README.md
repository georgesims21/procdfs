# procsys
Once cloned run:
```bash
cmake [-DCMAKE_BUILD_TYPE=Debug] . # from root dir
make -j # for multithreaded make
mkdir <mountdir>
./src/main.c [-f -s -d] <mountdir> # Use flags for running in foreground, single threaded for debugging
```

## Introduction
This is the coding project for my CS bachelor thesis, the aim is to recreate procfs on a distributed level using the FUSE API.

TODO:   
- [ ] 1: **Writing a filesystem that will mostly reflect what is in the procfs**

    - [x] 1.1: Mirror procfs with basic directories and files

    - [x] 1.2: Write simple bash ls/cat commands on the mounted filesystem   
        - [x] 1.2.1: ls procfs == ls procsys
        - [x] 1.2.2: cat files in procfs == cat files in procsys

    - [x] 1.3: Be able to handle standard utility commands like iostat fstat etc
            
        - [x] 1.3.1: Modify networking binaries to use this FS instead of /proc for testing purposes
            - [x] arp
            - [x] hostname
            - [x] ifconfig
            - [x] netstat
            - [x] rarp
            - [x] route

    - [ ] 1.4 (extra): Fix bottleneck in the getattrb operation
        - Due to proc files being 0 size, if you don't manually get the size and save it in the stat struct then no cat commands print. BUT this introduces huge bottleneck when ls'ing all contents

- [ ] 2: **Combine the procfs of two file systems** [DEADLINE: ~24/07~07/08]   
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

    - [x] 2.1: Build a basic client-server model and assume a TCP connection
    - [x] 2.3: Merge logic into filesystem
      - [x] 2.3.1: First model should use server to do all work and comms between clients
          - [plan](doc/networking-read()call-flowchart.pdf)
      - [] 2.3.2: Second model should have all fs' as servers and clients to each other
    - [ ] 2.4: Congregate some files and run tests with net-tools
    - [ ] 2.5 (extra): Convert it into a secure client-server with openssl libs

- [ ] 3: **Hijacking**   
      Fakeroot pretends that you are root. It will give a different file system
      overview as what's actually there, like a container/VM. Then any command run
      within this fakeroot follows that structure. So could replace /proc with
      uberproc in the fakeroot and then calls made within this fakeroot will think
      that uberproc is actually /proc.
      Can use strace <command> to see a syscall trace to see what files etc are
      being accessed by that command, this helps with debugging.

    - [ ] 3.1: Use a mix of mount namespaces and chroot
      - [ ] 3.1.1: Read about [namespaces](https://man7.org/linux/man-pages/man7/namespaces.7.html)
      - [ ] 3.1.2: Read about [mount namespaces](https://man7.org/linux/man-pages/man7/mount_namespaces.7.html)
      - [ ] 3.1.3: Read about [chroot](https://man7.org/linux/man-pages/man2/chroot.2.html)
      - [ ] 3.1.4: Read [tutorial on chroot](https://www.cyberciti.biz/faq/unix-linux-chroot-command-examples-usage-syntax/)

- [ ] 4: **Benchmarking**
