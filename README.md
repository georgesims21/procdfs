# procdfs
## procfs, distributed
### Running procdfs
Once cloned run:
```bash
mkdir build
cd build
mkdir <mountdir>
cmake ..
make -j # for multithreaded make
# Use -f flag for running in foreground, -s single threaded. Must use debugging (-d) flag
./procdfs [-f -s] -d <mountdir> <total-machines> <port-number> <interface-name> <ipfile> <machine-ip>
```
On every machine that you wish to monitor.
### Input parameters
1. ```-d```               : Debug flag, distributed version doesn't work without it. The other two are optional   
2. ```<mountdir>```       : This is the location where procdfs will mount, after running all files can be found inside this location   
3. ```<total-machines>``` : The total number of machines on the network   
4. ```<port-number>```    : The port number you wish to use for this particular machine   
5. ```<interface-name>``` : Specify the interface name this machine should use (unused)   
6. ```<ipfile>```         : A file containing all the IPs of the machines on the network. An example file can be found [here](example-ip-file.txt)   

### Querying a file
Once running, you should be able to query a file in ```<mountdir>``` and get a concatenated version of that corresponding procfs file. For example if there are 3 machines which are connected:
```
cat procdfs/net/dev
```
Should return something similar to this:
![Image showing expected output of the above command](https://github.com/georgesims21/procdfs/blob/master/prototype-procdfs-output.png?raw=true)
