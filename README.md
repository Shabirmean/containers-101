# Simple Linux containers from scratch in C

The repository has two files `container-101.c` and `container-102.c`. To run either of them you must be `root` in the host in which you are attempting to run them on. This is because some system calls _(like clone() and exec())_ requires root permissions,

---
### container-101.c

Simple C program with 3 functions `main()`, `child_function()` and `get_input_args()`. When run the program clones into a new process and executes the `child_function()`. The `child_functio()` just takes in any arguments passed into the program and tries to execute it.

```sh
root> gcc container-101.c -o container-101
root> ./container-101 "/bin/ls"

Welcome to containers 101!
--------------------------
PARENT >> PID is [1891]
PARENT >> PID of my child is [1892]         # Child PID as seen by the parent
CHILD  >> PID is [1]                        # Child PID as seen by the child (inside container)
CHILD  >> execve("/bin/ls", "(null)", NULL)
--------------------------
folder-outside-container container-101  container-101.c


root> gcc container-101.c -o container-101
root> ./container-101 "/bin/ls -al"

Welcome to containers 101!
--------------------------
PARENT >> PID is [1894]
PARENT >> PID of my child is [1895]         # Child PID as seen by the parent
CHILD  >> PID is [1]                        # Child PID as seen by the child (inside container)
CHILD  >> execve("/bin/ls", "-al", NULL)
--------------------------
total 2036
drwxr-xr-x  6 shabirmean shabirmean    4096 Mar 10 16:34 .
drwxr-xr-x  3 root       root          4096 Mar  4 04:49 ..
-rwxr-xr-x  1 shabirmean shabirmean   17288 Mar 10 16:34 container-101
-rw-r--r--  1 shabirmean shabirmean    3349 Mar 10 16:34 container-101.c
drwxr-xr-x  2 root       root          4096 Mar 10 16:59 folder-outside-container
```

You can also run a shell as the child process, which means that you are _running a shell inside the `container`_.

```sh
root> ./container-101 "/bin/sh"

Welcome to containers 101!
--------------------------
PARENT >> PID is [1902]
CHILD  >> PID is [1]
PARENT >> PID of my child is [1903]
CHILD  >> execve("/bin/sh", "(null)", NULL)
--------------------------
$ ls                           # Here we are inside the container (the child shell process)
folder-outside-container container-101  container-101.c
$ pwd
/home/shabirmean
$ exit                        # This exits out of the container and takes us to the original shell
```

One thing to notice above is that even whilst inside the container shell we could see the `folder-outside-container` directory with `ls`. In `container-102` we will jail the container to a restricted view, so that nothing from the host is visible except what's allowed.

---
### container-102.c

This is a build up on top of `container-101.c`. The additional things this acheives are:
- Jail the container to a restricted view of the file-system. For this we download the [`Alpine Linux`](https://github.com/yobasystems/alpine) image. In the examples shown below, [this image](http://nl.alpinelinux.org/alpine/v3.7/releases/x86_64/alpine-minirootfs-3.7.0-x86_64.tar.gz) is used and extracted into a folder named `rootfs`.
- Takes in an additional argument to configure a new `hostname` for the container
- Sets up new environment variables for the container process

```sh
root> ls -l             # we are in th ehost machine here
total 2020
-rw-r--r--  1 shabirmean shabirmean 2000208 Nov 30  2017 alpine-minirootfs-3.7.0-x86_64.tar.gz
-rw-r--r--  1 shabirmean shabirmean    3349 Mar 10 16:34 container-101.c
-rw-r--r--  1 shabirmean shabirmean    4770 Mar 10 16:34 container-102.c
drwxr-xr-x  2 root       root          4096 Mar 10 16:59 folder-outside-container
drwx------ 19 root       root          4096 Mar 10 17:12 rootfs
root> echo $PATH        # compare this PATH var in the host machine against the one iniside the container
/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
root> gcc container-102.c -o container-102
root> ./container-102 -h newhost -c "/bin/sh"

Welcome to containers 101!
--------------------------
PARENT >> PID is [1985]
PARENT >> PID of my child is [1986]
CHILD  >> PID is [1]
CHILD  >> execve("/bin/sh", "(null)", NULL)
--------------------------
/ $ ls                   # we are inside the container process here
                         # only folders within the alpine-image
bin    dev    etc    home   lib    media  mnt    proc   root   run    sbin   srv    sys    tmp    usr    var
/ $ pwd                  # we cannot see anything outside the rootfs folder
/                        # it looks like we are viewing a separate filesystem but its just inside rootfs
/ $ env
SHLVL=1
PWD=/
/ $ hostname
newhost
/ $ echo $PATH           # PATH env is different inside the container
/sbin:/usr/sbin:/bin:/usr/bin
/ $ exit
```

