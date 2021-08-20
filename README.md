
The IODA README link:
https://docs.google.com/document/d/1uvWwgCik9uFjmT2ZhTNeJ9tQ5G7Rurvqq5w8Cg27Ujw/edit


# IODA Artifact #

### Overview of the Artifact

IODA artifact includes the following components:

- ``iodaFEMU``: IODA-enhanced SSD controller
- ``iodaLinux``: IODA-enahnced Linux kernel
- ``iodaScripts``: Utilities to run IODA experiments


All the experiments will run inside a VM, where we use ``iodaLinux`` as the
guest OS and manages a NVMe SSD exposed by ``iodaFEMU``.


To simplify the evaluation process, we provide the binary



IODA environment setup (prerequisite installations and download the IODA VM images):

Artifact repo: https://github.com/huaicheng/IODA-SOSP21-AE.git

To build FEMU (tested on ucare-10)
clone the following FEMU version:

$ git clone https://github.com/huaicheng/iodaFEMU.git
$ cd iodaFEMU
$ git checkout 49b768b

Create build directory and copy installation scripts

$ mkdir build-femu
$ cd build-femu
$ cp ../femu-scripts/femu-copy-scripts.sh .
$ ./femu-copy-scripts.sh

Install dependencies (only Ubuntu/Debian based distributions supported)

$ sudo ./pkgdep.sh

Compile FEMU

$ ./femu-compile.sh

To build Kernel: run yyy
Go out of iodaFEMU directory

    $ cd ../../

Download linux kernel bzImages here: [link to bzImage and bzImage-ioda]
Create directory images and images/bzimages

    $ mkdir -p images/bzimages

Move downloaded bzImages to images/bzimages

    $ mv bzImage images/bzimages
    $ mv bzImage-ioda images/bzimages

Download our IODA VM image here: [link to IODA VM image]
Move downloaded VM image to images/

        $ mv ioda.qcow2 images/

Download our run script here: [link to ioda.sh]
Download result processing scripts here: [link to rtk]

At this point directory hierarchy should be like this:

images
├── bzimages
│   ├── bzImage
│   └── bzImage-ioda
└── ioda.qcow2
iodaFEMU
ioda.sh
rtk


FEMU:
Link: iodaFEMU/harmonia_and_dynamic
commit: 49b768b

Kernel:

IODA: FEMU2.0/tw_and_proactive
Commit:900e39b

Other: iodaLinux/tifa-mk-415-base
Commit: 197ee12

<suppose we have finished setting up the stack here>


Running the Experiments
1.) Steps to run the experiment:

Login to machine (suppose emulab using Martin’s account)

$ ssh -p22 <user>@<machine>.emulab.net

(optional) Create a tmux session so that we can easily add terminal windows. We might need 3 windows to do the experiment smoothly.

$ tmux

Run FEMU with the given VM and kernel image.

    $ cd ioda/sosp21/
    $ ./ioda.sh

FEMU will start booting and keep printing output for about 30-60 seconds. Once it is done, our VM will be available at localhost port 10101

If we use VM image provided in section (A), we can connect to the VM using the following account

    username: huaicheng
    password: ii

Furthermore, the provided image allows passwordless ssh if we run it on emulab. Thus we can ssh into our VM using the following command:

$ ssh -p10101 huaicheng@localhost

By default we will enter /home/huaicheng/tifa/replayer directory:
<screenshot>
Our main script is r.sh . It will do everything we need:
Create RAID5 array using FEMU’s emulated SSDs
Warmup the array to make it reach steady-state, such that further workload will trigger garbage collection
Run trace Azure,[BingIdx,BingSel,Cosmos,DTRS,Exch,LMBE,MSNFS,TPCC] using the base,[iod1, iod2, iod3, ioda, ideal] policy N times
Collect and preprocess IO latencies and various metrics.
Organize them in directories for easy download from the host side.


Line 12 to change traces (e.g. run tpcc and exchange)
tfs=”tpcc-resize-w16.0-20s est-trim-s2700-e3300”

Complete list of traces:

Trace
Filename
Azure
azurestorage-drive2.trace-trim-s350-e530-rerate-0.25-resize-w16.0
BingSelection
bingselection-drive2.trace-trim-s200-e260-resize-w2.0-r2.0
BingIndex
bingindex-drive2.trace-trim-s6300-e6600-rerate-2.0-resize-w4.0
Cosmos
cosmos-drive2.trace-trim-s1890-e2000
DTRS
dtrs-frankenstein2
Exchange
est-trim-s2700-e3300
LMBE
lmbe-resize-r0.25-w3.0-trim-s1600-e1660
MSNFS
msnfs10-20s
TPCC
tpcc-resize-w16.0-20s



Line 13 to change number of runs for each {trace,policy} (e.g. run the {trace,policy} 3 times)
nums=”1 2 3”

Line 27 to change policy (e.g. run base and iod-1)
for i in “nopgc nosync def” “nopgc nosync gct”; do

Complete list of existing policies:

Policy
Change in Script
base
“nopgc nosync def"
iod-1
“nopgc nosync ebusy”
iod-2
“nopgc nosync gct”
iod-3
“nopgc sync100ms gct”
ioda
“nopgc sync100ms ktw” (need to change kernel)
ideal
“nopgc nosync nogc”

After deciding the traces, policies, and number of runs, simply run the script:
    $ ./r.sh

We need to re-run FEMU using another kernel to use the ioda policy. We have provided the bzImage. To do this, shutdown the VM:

    $ sudo shutdown -h now

This will bring us back to the host side. Open ioda.sh
    $ vi ioda.sh

Then, comment out line 10 and uncomment line 11.

Before

After


Repeat step (c) - (f) for all traces and policies we need.

If we need to run the other policies again, restore the default kernel by commenting out line 11 and uncommenting line 10.

C. Plotting the Results

After all is VM-side experiment is done, switch to host and go to rtk directory:

    $ cd sosp21/rtk

Make sure the VM is still up.
Download and process the experiment results. For all traces, run:

$ ./doscp.sh sosp21 <trace filename>

E.g. to download TPCC and LMBE results:
$ ./doscp.sh sosp21 tpcc-resize-w16.0-20s
$ ./doscp.sh sosp21 lmbe-resize-r0.25-w3.0-trim-s1600-e1660

The script will:
Download all results for a given trace.
Create a CDF of latency for every {trace,policy,run} and put them in sosp21/rtk/dat folder.

(optional) If the host machine is headless, most probably we need to move the results to a machine which has GUI. We only need to download the sosp/rtk/dat folder:

    E.g. Download dat folder from emulab to local machine
    rsync -azP -e ‘ssh -p22’ <user>@<emulab_machine>:<prefix>/sosp21/rtk/dat dat

Make an empty plot and eps folder at the directory with dat.

$ mkdir plot
$ mkdir eps

At this point the directory hierarchy should be similar to this:


We use gnuplot to generate our graph. We can create our own plot file, but since it might be a bit cumbersome to tune the graph style, we provided several templates here:

<INSERT LINK>

Adjust the file which we want to plot on the ‘plot \’ section.
E.g. For TPCC, if we want to plot the 1st run of all policies, change every ‘<long_string>-X-rd_lat.dat’ to ‘<long_string>1-rd_lat.dat’.

The resulting graph will be in folder eps/. Check the graph and tune its xlim, ylim, etc as desired.


- README, w/ detailed steps to reproduce the results
    - Setup
        - OS
        - Dependencies
        - Steps to run the experiments
        - Performance tuning
        - How to plot the results
        - Recorded video to demonstrate how to run the exps
        -

=======================================================================


Table of Contents (raw):

- README, w/ detailed steps to reproduce the results
    - Setup
        - OS
        - Dependencies
        - Steps to run the experiments
        - Performance tuning
        - How to plot the results
        - Recorded video to demonstrate how to run the exps
        -

- Source code and necessary packages
    - iodaFEMU (source code + bin)
    - iodaLinux (source code + bin)
    - IODA VM Image
    - Scripts for plotting


0. Martin, put the path to our VM image here: /home/martin/tifa-vm.qcow2

(Optional : if you use emulab machine and your image is in ucare machine) Set up NFS-server
Every node run in emulab is publicly visible using name
 <node>.<experiment_name>.<project>.emulab.net.
We can either use this name (means we must change it for every experiment) or just allow any node from .emulab.net to connect to ucare machine (technically less secure ?). The steps below will give access to any node from .emulab.net.

To setup NFS server on ucare (source:http://nfs.sourceforge.net/nfs-howto/ar01s03.html) :
Check existence of nfs-kernel-server.
Do sudo systemctl status nfs-kernel-server -> Service found or not ?
If not, install : sudo apt-get install nfs-kernel-server
Modify /etc/exports
sudo vi /etc/exports
Add entry : /home/<your_shared_folder> *emulab.net (rw)
        ‘*’ will match any subdomain. Refer to source link for rw and other parameters
Modify /etc/hosts.deny
sudo vi /etc/hosts.deny
Append entries :
portmap:ALL
lockd:ALL
mountd:ALL
rquotad:ALL
statd:ALL
Modify /etc/hosts.allow
sudo vi /etc/hosts.allow
portmap: .emulab.net
lockd: .emulab.net
mountd: .emulab.net
rquotad: .emulab.net
statd: .emulab.net

These are services related to remotely mounting folder via nfs. Basically we try to only allow machines from .emulab.net to mount.

Change permission for folders and files : sudo chmod XXX <folders/files>

To mount from emulab node :
mkdir <target_path_for_mount>
sudo mount ucare-09-river.cs.uchicago.edu:<path_to_shared_folder> <target_path>
Just leave this mounted.

Later, don’t forget to change tifa-test.sh script, -drive parameter to match the path of image in your mounted folder (explained in step 2 below)

Compile TIFA linux kernel and FEMU on host machine (aka your Emulab machine):

Kernel repo: https://github.com/huaicheng/tifaLinux , use branch “tifa-bb”

In my environment, I compiled the kernel on host machine and pass in parameters to allow FEMU to load it …


I suggest you to use the same setup as mine, so we need to compile the kernel on host machine, and add the above BOLD line in your tifa-test.sh script.

Kernel compilation HOWTO (do this on your host machine)

$ mkdir ~/git
# if you don’t have access permission to this repo, ask HC for help
$ git clone https://github.com/huaicheng/tifaLinux
$ cd tifaLinux
$ git checkout tifa-bb
# download config for linux (kernel configuration file)
$
# do compilation
$ make -jX # replace X with # of physical cores in your machine to accelerate the process, e.g. X=16 if you have 16 cores on that machine. Use -nproc to know the # of physical cores.


// After the compilation is done, you can find the kernel image which will be used to boot the VM as arch/x86_64/boot/bzImage, as mentioned above, make sure to add the bold lines into a VM running script.


Compile FEMU:

tifaFEMU repo: https://github.com/huaicheng/tifaFEMU, use branch “tifa-bb”
Follow the instructions at https://github.com/ucare-uchicago/femu README to compile FEMU under build-tifa/ directory


From build-tfia folder, run ./tifa-test.sh under build-tifa/ folder, the VM should boot ..

Here is what my FEMU script (tifa-test.sh) looks like:
ucare-
OCKERNEL=$HOME/git/tifaLinux/arch/x86_64/boot/bzImage

sudo x86_64-softmmu/qemu-system-x86_64 \
    -name "tifaVM" \
    -cpu host \
    -smp 16 \
    -m 16G \
    -enable-kvm \
    -kernel "${OCKERNEL}" \
    -append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0 nokaslr log_buf_len=128M loglevel=4" \
    -boot menu=on \
    -drive file=$IMGDIR/u14s.qcow2,if=ide,cache=none,aio=native,format=qcow2 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -netdev user,id=user0,hostfwd=tcp::10101-:22 \
    -device virtio-net-pci,netdev=user0 \
    -nographic | tee log 2>&1 \

As mentioned above:
    1.) Match the -drive parameter to your mounted ucare path
2.) Make sure OCKERNEL is the path to your compiled linux kernel bzImage

Some explanations over the above FEMU parameters:

“-smp 16” represents # of CPUs allocated to the VM (here in VM, you will see 16 CPUs)

“-m 16G” represents VM DRAM size (here in VM, you will see total DRAM size is 16 GB)

“-device femu,devsz_mb=12288,femu_mode=1” represents an emulated FEMU NVMe SSD to the VM, the SSD size is 12288MB (12GB)

You need to carefully assign #CPUs,#DRAM,#SSD-size according to the total DRAM amount on the host machine, in the above script, it requires the host to have at least 12GB * 4 + 16GB = 64GB. If your host machine don’t have >64GB DRAM, there are some extra changes we need to tune, so please do let me know if this is your case.



VM account info:
user: huaicheng
password: ii


ssh into the VM from host terminal:

$ ssh -p10101 huaicheng@localhost

The port number 10101 is specified in your tifa-test.sh script, “hostfwd=tcp::10101-:22”, if you use a different port number there, change to it in the above ssh command accordingly.


Still on host machine, run the following program to make sure all the cores are working in full-speed mode:

$ wget http://people.cs.uchicago.edu/~huaicheng/pm-qos.c
$ gcc pm-qos.c -o pmqos
$ sudo nohup ./pmqos & # let this program run in the background, do this as first step after the host machine is rebooted

// i7z can be installed via: sudo apt-get install i7z
$ sudo i7z  # it should show that all cores are running at highest frquency and 100% of the time in C0 state



Once you’re inside guest OS:

(0). check your kernel version

$ uname -a

This should give you something like this:

Linux u14s 4.15.0-rc4-tifa-host+ #90 SMP Wed Jan 16 09:26:55 CST 2019 x86_64 x86_64 x86_64 GNU/Linux

Where, make sure you have 4.15.0-rc-tifa-host, which means you are using the kernel you just compiled.

(1). create raid5 device

$ cd tifa
$ ./mk-r5.sh # this will create raid5 device
# check raid5 device status:
$ cat /proc/mdstat

Personalities : [raid6] [raid5] [raid4]
md0 : active raid5 nvme3n1[3] nvme2n1[2] nvme1n1[1] nvme0n1[0]
      37724160 blocks super 1.2 level 5, 4k chunk, algorithm 2 [4/4] [UUUU]

unused devices: <none>


(2). warmup the emulated SSDs to reach steady state

$ ./ss
# this will take a few minutes


(3). replay the TPCC workload

We can run the workload under 3 different modes:

default (“def”): the default case, where vanilla raid5 code is used
TIFA (“gct”): we do automatic reconstruction when IOs meet GC
NoGC (“nogc”): perfect case, where no GC caused tail latencies

Overall, under /home/huaicheng/tifa/replayer, there is a script file named “tifa”, you can use it for running all three modes in one shot,

double check script “r.sh”, line 8, make sure it’s

        for exp in "def" “nogc" "gct"; do

In this case, when you do “./tifa”, it will run “def” “nogc” “gct” one by one.

As an initial start, run one mode at a time and get the results, then generate the CDF graph .. so you have a feeling about how it works ..

    e.g. For only running the “def” case, in “r.sh”, change line 8 to
            for exp in "def"; do



Run “r” script (which calls tifa):
Modify FP variable to desired name ex. tpcc-new-test

$ ./r.sh

After it’s completed, all the related log files will be put into a folder named “tpcc-new-test”

Inside that folder, the most important one is the latency log file: tpcc-new-test.log

The latency log file contains latency numbers for both reads and writes, we are only interested in read latencies, so we need to filter out reads first:

$ ./filter_rd_log.sh tpcc-new-test.log

This will give you an output file named “tpcc-new-test-rd_lat.log”, which will be used for plotting the CDF.
The r.sh script already does this for you



(4). Use “rtk” to generate CDF graphs

@Ronald, please fill this part … tell Andrew how to use rtk to plot CDF ...

Download rtk on your host machine: https://github.com/huaicheng/rtk
Follow the instructions in rtk’s readme to create an appropriate folder and modify all.sh, we are making lat-cdf graph
scp the rd_lat.log from the VM onto your host machine (or even on to your local machine? wherever you want)
run all.sh
Your plot will exist in eps/ and your gnuplot script will be in plot/
scp from remote physical machine to your local machine
Inspect the graph on your local machine
Change parameters in the gnuplot script (I do set autoscale first normally to get an idea of what the data looks like)
gnuplot plot/gnuplot-script-name.plot
Copy, inspect, modify gnuplot script and repeat until you get readable graph
Get something like this?



----------

Your task ZERO will be following the above instructions to run TPCC, generate the TPCC-CDF graph I sent you in the group chat (see below). Make sure you do this step first and then go to hacking task.






--------------------------------------------------

Running SyncGC-FEMU from Ronald

Updated parameters and GC synchronization is in tifaFEMU branch “ronald”
    tifa-test.sh in that branch is the script I use to launch the VM
    temp_r.sh is the script that I use within the VM to run tests (gives an idea of how I use the flip commands)

Before running a workload, if you want to...
To enable Synchronized GC, run flip 8 in the VM
To disable Synchronized GC, run flip 9 in the VM
To adjust how big this window is, run flip [12/13/14/15/16/17/18] in the VM
So far, 100MS window looks best, but 40MS, 200MS, 400MS look okay as well

Options for flip command are in nvme-core.c
enum {
    FEMU_ENABLE_GC_DELAY = 1,
    FEMU_DISABLE_GC_DELAY = 2,

     FEMU_ENABLE_DELAY_EMU = 3,
    FEMU_DISABLE_DELAY_EMU = 4,

     FEMU_RESET_ACCT = 5,
    FEMU_ENABLE_LOG = 6,
    FEMU_DISABLE_LOG = 7,

     FEMU_SYNC_GC = 8,
    FEMU_UNSYNC_GC = 9,

     FEMU_ENABLE_LOG_FREE_BLOCKS = 10,
    FEMU_DISABLE_LOG_FREE_BLOCKS = 11,

     FEMU_WINDOW_1S = 12,
    FEMU_WINDOW_100MS = 13,
    FEMU_WINDOW_2S = 14,
    FEMU_WINDOW_10MS = 15,
    FEMU_WINDOW_40MS = 16,
    FEMU_WINDOW_200MS = 17,
    FEMU_WINDOW_400MS = 18,

};
Check out femu_flip_cmd function in nvme-core.c to see what they do


Martin, let’s use his code to repeat the EST experiment first.

-----



Analyze trace characteristics

Scripts to analyze trace characteristics and examples of existing characteris

Go to the link below to see existing trace characteristics graph (currently there are characteristics for DAPPS, DTRS, EST, LMBE, MNSFS, and TPCC) :
https://github.com/huaicheng/FEMU2.0/tree/master/trace/characteristics

Scripts to generate the graphs, along with instructions on how to use it, can be found in the parent folder :
https://github.com/huaicheng/FEMU2.0/tree/master/trace/

We can also use the trace-edit tool to get a more detailed numerical characteristics.  To do this :
Clone trace-edit repo (git clone https://github.com/huaicheng/replayer).
In replayer/, create in/ and out/ folders.
Put our target traces in in/ folder.
Run trace-edit.py :
        python trace-edit.py -file<trace_filename> -characteristic

Detailed numerical characteristics of the trace can be found in <trace_filename>-characteristic.txt file in out/ folder.

Other than giving detailed numerical characteristics, trace-edit can also be used to modify traces (resizing I/Os, rerating I/Os, cutting traces, etc). See readme for all supported cases.

For example, by using -characteristic argument on est trace :
python trace-edit.py -file est -characteristic

We will get this result in out/est-characteristic.txt :

Title: est
Total time: 3599.69s
IO Count: 179856
IO per second: 49.96
% Read: 19.63
Read (KB) / sec: 150.46
Reads per second: 9.81
% Write: 80.37
Write (KB) / sec: 1115.03
Writes per second: 40.15
% randomWrite in Write: 61.28

---Size bucket (in KB) -- [0-32,32-64,64-128,128-256,256-512,512-1024]---
Bucketed read count: [32384, 2918, 0, 9, 0, 0, 0], Total: 35311
Bucketed write count: [88739, 55805, 0, 0, 0, 1, 0], Total: 144545

Bucketed read size: [376.48 MB, 150.50 MB, 0B, 1.93 MB, 0B, 0B, 0B], Total: 528.91 MB
Bucketed write size: [472.34 MB, 3.37 GB, 0B, 0B, 0B, 1024.00 KB, 0B], Total: 3.83 GB

---Note: Small I/O is equal or smaller than 32KB; big I/O is larger than 32KB---
Total small random writes: 78726
Number of small random writes / sec: 21.87
Total big writes: 55806
Number of big writes / sec: 15.50
Score (#bigWrites/#smallWrites): 0.63

---Whisker plot information: min, 25%, med, 75%, max---
Read size (Byte) : 512, 4096, 8192, 24576, 225280
Write size (Byte) : 512, 4096, 8192, 65536, 1048576
Time interval (ms) : 0.00, 0.10, 0.23, 0.52, 4250.16
Number of writes to the same block: 1.0, 2.0, 3.0, 17.0, 42.0

We can rerate the trace (modify request inter-arrival time) to make its workload less intensive :

python trace-edit.py -file est -rerate 2
This will produce est-modified.trace in out/, which has 2x more inter-arrival time compared to default est

mv out/est-modified.trace in/est-rerate2
python trace-edit.py -file est-rerate2 -characteristic

We will get this result in est-rerate2-characteristic.txt :

Title: est-rerate2
Total time: 7199.38s
IO Count: 179856
IO per second: 24.98
% Read: 19.63
Read (KB) / sec: 75.23
Reads per second: 4.90
% Write: 80.37
Write (KB) / sec: 557.51
Writes per second: 20.08
% randomWrite in Write: 61.28

---Size bucket (in KB) -- [0-32,32-64,64-128,128-256,256-512,512-1024]---
Bucketed read count: [32384, 2918, 0, 9, 0, 0, 0], Total: 35311
Bucketed write count: [88739, 55805, 0, 0, 0, 1, 0], Total: 144545

Bucketed read size: [376.48 MB, 150.50 MB, 0B, 1.93 MB, 0B, 0B, 0B], Total: 528.91 MB
Bucketed write size: [472.34 MB, 3.37 GB, 0B, 0B, 0B, 1024.00 KB, 0B], Total: 3.83 GB

---Note: Small I/O is equal or smaller than 32KB; big I/O is larger than 32KB---
Total small random writes: 78726
Number of small random writes / sec: 10.94
Total big writes: 55806
Number of big writes / sec: 7.75
Score (#bigWrites/#smallWrites): 0.63

---Whisker plot information: min, 25%, med, 75%, max---
Read size (Byte) : 512, 4096, 8192, 24576, 225280
Write size (Byte) : 512, 4096, 8192, 65536, 1048576
Time interval (ms) : 0.00, 0.20, 0.46, 1.03, 8500.32
Number of writes to the same block: 1.0, 2.0, 3.0, 17.0, 42.0

Note that many values are half the default est trace, showing that new trace has become 2x less intensive.

Vanilla FEMU (Base)
This is base FEMU with some parameters changed in order to simulate real SSDs better
https://github.com/huaicheng/tifaFEMU/commit/2c2b4e95c8f89948081445d9dea4ae8a6beed2eb
The VM script :
sudo x86_64-softmmu/qemu-system-x86_64 \
    -name "tifaVM" \
    -cpu host \
    -smp 24 \
    -m 16G \
    -enable-kvm \
    -kernel "${OCKERNEL}" \
    -append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0 nokaslr log_buf_len=128M loglevel=4" \
    -boot menu=on \
    -drive file=$IMGDIR/u14s.qcow2,if=ide,cache=none,aio=native,format=qcow2 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -device femu,devsz_mb=12288,femu_mode=1 \
    -netdev user,id=user0,hostfwd=tcp::10101-:22 \
    -device virtio-net-pci,netdev=user0 \
    -nographic | tee ~/log 2>&1 \
Sync GC (aka Time Window, Rotating GC)
This segment of code contains the logic for Sync GC
https://github.com/huaicheng/tifaFEMU/blob/harmonia_and_dynamic/hw/block/femu/ftl/ftl.c#L799-L810
If you’re looking for a commit to rebase your code on top of, this commit has fewer changes and the same logic
https://github.com/huaicheng/tifaFEMU/blob/f4409ea578ddda91d8e361b70662c3fc0b585e1d/hw/block/femu/ftl/ftl.c#L771-L778
This code denies GC from starting when it isn’t within the SSD’s time window
An SSD’s time window is determined by its ID. This ID is then compared with the current time to determine whether it can start GC.
There is also a buffer period at the end of every time window, so GC won’t start at the end of the time window and run over into the next SSD’s time window
Refer to above (Running SyncGC-FEMU from Ronald) for turning on sync GC

Faster warmup
This segment of code contains the logic for a faster warmup that’s supposed to act similarly to the ssd-steadystate fio scripts used. This should be much faster than writing to devices with fio
https://github.com/huaicheng/tifaFEMU/blob/b1714707608097d8835ddbef36d1d426d2fe77ff/hw/block/femu/ftl/ftl.c#L72-L153
Notes:
A short set of IOs still need to be sent to the RAID device before the trace file can be replayed (I use 1 second of randwrites)
The first trace file replayed will have worse latencies. Because of this, I run the nogc case first when running a series of trials. For more reliable results, use old warmup scripts
To perform the warmup on startup, you can insert this line of code into ssd_init
https://github.com/huaicheng/tifaFEMU/blob/b1714707608097d8835ddbef36d1d426d2fe77ff/hw/block/femu/ftl/ftl.c#L503
To repeat the warmup, I set up these flip commands:
https://github.com/huaicheng/tifaFEMU/blob/b1714707608097d8835ddbef36d1d426d2fe77ff/hw/block/femu/nvme-core.c#L916-L918
flip 100 does sequential and then random warmup
I also have extraneous code in this commit related to resetting, saving, and restoring SSD state. Those changes can be ignored

Using Emulab
Creating an Account
Go to emulab’s signup page : https://www.emulab.net/portal/signup.php
Fill in your desired username, password, full name, and email. Don’t use your institutional email if you’re from different institution (e.g., use gmail, etc).
Fill in UCARE’s info :
Institutional affiliation : University of Chicago
Country : United States
State : Illinois
City : Chicago
Join existing project, Project Name : ucare
Upload your machine’s ssh public key
Using the machines
    Machines in emulab are in the form of VM instances launched within Emulab’s servers. We can either configure our own VM image or use an image (‘profile’) other group member has created.
2.1 Launching VM

    Assuming you want to use existing TiFA-FEMU experiment profile,

Go to ‘Project Profiles’ -> click tifa_femu_d430 play button.

In the VM instantiation wizard, at

Step 1 : Select a profile -> click Next ; Step 2 will be skipped; Step 3 : Finalize -> click Next; Step 4 : Schedule -> Choose your start time and desired duration (can be extended later)

Emulab will start to instantiate your VM. If it failed (usually due to node reservation), you can start over using shorter duration or other profile, e.g. tifa-femu-d820

After it says ‘ready’, click the ‘List view’ tab.

Copy the SSH command and paste it to your terminal. From here on you can operate emulab’s machine as a remote server.
2.2 Running TiFA VM
    Refer to the beginning of this README.
2.3 Additional Info & Tips
Use tmux to save your work session
Our persistent free space (not wiped after VM termination) is bounded by ucare’s & our home directory (/users/<your_username>) free space, whichever run out first. Use df -h to see this.
tifa_femu_d430 has 80GB unpersistent free space at /vm_images/.
Caution : This will be wiped out at every termination.


Proactive Kernel






Run YCSB on top of FEMU+TIFA Stack:

(1). Install YCSB






@Fadhil, Put your steps to run FileBench here:

(1). Boot the VM, create the RAID device, do warmup, etc..





@Fadhil, Put the steps to passthrough NVMe devices to VM and run Ronald’s kernel with raid0-latency-tracing

Measuring DCSSD RAID 0 performance
The goal of this experiment is to measure the impact of GC (SubIO slowdown) in NVMe SSD RAID-0. We did the experiments inside a VM so we need to passthrough all the NVMe SSD devices to the VM

Preparing FEMU
Clone FEMU from the repository here: https://github.com/ucare-uchicago/femu
Follow the instruction in the README.md to compile it
Preparing Modified Kernel
We have modified linux-4.15 kernel, so we can track RAID 0 sub-IO latencies. The modified kernel can be accessed here: https://github.com/huaicheng/FEMU2.0/tree/update_subio_prof/linux-4.15-rc4-subio
Clone the FEMU2.0 repo to your machine, change the branch to update_subio_prof
Change directory to “linux-4.15-rc14-subio”
Config the kernel so the nvme & raid (md) are activated.
In the kernel directory use this command to access the config menu:
        make menuconfig
Search for “nvme” configuration by typing “/” then “nvme”, after that select “1”. Make sure all the nvme is enabled (indicated by “*”)

Search for RAID 0 configuration by typing “/” then “md”, after that select “1” (Multiple devices driver support (RAID and LVM)). Make sure the RAID-0 (striping) mode is enabled, as indicated with “*”.

Save the configuration as “.config”
Besides following step 4, You can also download our predefined config with this command:
$ wget http://people.cs.uchicago.edu/~huaicheng/config -O .config
After you are done with the configuration you can compile the kernel with this command:
    $ make -jX

replace X with # of physical cores in your machine to accelerate the process, e.g. X=16 if you have 16 cores on that machine. Use -nproc to know the # of physical cores.
The compilation will produce bzImage file. Now the kernel is ready to be used!

Passing NVMe devices to VM (Passthrough NVMe)
Make sure IOMMU is enabled in grub.
sudo vi /etc/default/grub
        GRUB_CMD_LINUX = “... intel_iommu=on iommu=pt ...”    # Append if they don’t exist
        sudo update-grub
        sudo reboot        # reboot system to take effect

Make sure all the NVMe devices is connected to your machine with
“lspci | grep -i non-volatile”.

The output should be like this:
17:00.0 Non-Volatile memory controller: HGST, Inc. Ultrastar SN200 Series NVMe SSD (rev 02)
18:00.0 Non-Volatile memory controller: Samsung Electronics NVMe SSD Controller 172Xa/172Xb (rev 01)
65:00.0 Non-Volatile memory controller: Intel Corporation Express Flash NVMe P4500/P4600
b3:00.0 Non-Volatile memory controller: Micron Technology Inc 9100 PRO NVMe SSD (rev 05)

The numbers like "17:00.0", "18:00.0" are the PCI IDs for the NVMe devices. We need this ID to detach the device from host OS. To detach the device from host OS we have this “vfio-bind” script:
vfio-bind
#!/bin/bash

modprobe vfio-pci
modprobe vfio_iommu_type1

for dev in "$@"; do
        vendor=$(cat /sys/bus/pci/devices/$dev/vendor)
        device=$(cat /sys/bus/pci/devices/$dev/device)
        if [ -e /sys/bus/pci/devices/$dev/driver ]; then
                echo $dev > /sys/bus/pci/devices/$dev/driver/unbind
        fi
        echo $vendor $device > /sys/bus/pci/drivers/vfio-pci/new_id
done


To detach a device run this command:
./vfio-bind <devices ID>
For example : ./vfio-bind 0000:17:00.0

Check whether the device already detached from host OS using “lspci”:
    e.g.  lspci -s 17:00.0 -vvv  | grep "driver"
It should shows:
    Kernel driver in use: vfio-pci
If the driver is vfio-pci then it already successfully detached from the host OS.










Config the VM to passthrough the NVMe devices by adding this parameter for each devices:
-device vfio-pci,host=02:00.0
With 02:00.0 is the device’s ID. Here is the example script to run the FEMU VM:
teafa-test.sh
#!/bin/bash

OCKERNEL=git/FEMU2.0/linux-4.15-rc4-subio/arch/x86_64/boot/bzImage # modified kernel
IMGDIR=/home/huaicheng/images/tifa

sudo git/femu/build-femu/x86_64-softmmu/qemu-system-x86_64 \
    -name "teafaVM" \
    -cpu host \
    -smp 24 \
    -m 32G \
    -enable-kvm \
    -kernel "${OCKERNEL}" \
    -append "root=/dev/sda1 console=ttyS0,115200n8 console=tty0 biosdevname=0 net.ifnames=0 nokaslr log_buf_len=128M loglevel=4" \
    -boot menu=on \
    -drive file=$IMGDIR/u14s.qcow2,if=ide,cache=none,aio=native,format=qcow2 \
    -device vfio-pci,host=65:00.0 \
    -device vfio-pci,host=17:00.0 \
    -device vfio-pci,host=18:00.0 \
    -device vfio-pci,host=b3:00.0 \
    -nographic | tee ~/log 2>&1 \

Make sure to use the kernel we already compiled in the previous step B.

Boot the VM and type “sudo nvme list” in the guest OS, it will show the result like this:

Now we’ve already successfully passed through 4 NVMe SSDs to the VM.

Configure the RAID 0
After successfully passthrough all the SSD to the VM, now we will configure RAID0 with all of those devices. First, make sure all the nvme ssds are ready with “sudo nvme list” command, just like in the prev step.
Create the RAID0 from those 4 nvme devices with this script. Just run “sudo ./make-raid0.sh” inside the VM (guest OS).
make-raid0.sh
#!/bin/bash

if [ -f /dev/md127 ]; then
    sudo mdadm --stop /dev/md127
    sudo mdadm --zero-superblock /dev/nvme0n1
    sudo mdadm --zero-superblock /dev/nvme1n1
    sudo mdadm --zero-superblock /dev/nvme2n1
    sudo mdadm --zero-superblock /dev/nvme3n1
fi

sudo mdadm --create /dev/md127 --assume-clean --level=0 -c 4 --raid-devices=4 /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1

Make sure the raid-0 is active with this command: “cat /proc/mdstat”, it should show this result:

Now you have the RAID0 configured in /dev/md127 and  you’re ready to run the measurement inside the VM.

Additional Note: Removing RAID-0 Configuration
You can use this script to remove the RAID-0:
unraid0.sh
#!/bin/bash

sudo umount -l /dev/md127
sudo mdadm --stop /dev/md127
sudo mdadm --zero-superblock /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1
if [ -f /dev/md127 ]; then
    sudo rm /dev/md127
fi


Checking Sub-IO Latencies Logging is Working
If you are using our modified kernel (read step B), it will log all sub-IO latencies in the RAID-0 after we are calling a syscall function. The kernel only support RAID-0 with 4 disks.

First, check the kernel used in your VM with this command:
$ uname -r
It should return:
4.15.0-rc4
Run simple fio read/write I/O to the RAID-0, it is /dev/md127 if you’re configure it with our previous explanation in step D. You can use this fio command:
$ sudo fio --name=simpleread --filename=/dev/md127 --bs=16k --direct=1 --rw=randread --ioengine=psync --size=1G

We should use --filename=/dev/md127 to target our RAID devices.
We should use --bs=16k because the modified kernel only support this bs. (4disks * 4k)
We should use --direct=1 so the I/O is directly served by the RAID disks.
We should use --ioengine=psync which is the default engine to run synchronize I/O in Linux
Then we need to print the sub-IO latencies to the kernel log using syscall(333); You can compile and use this program to do so:

dump-subio-lat-to-kern-log.c
include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    syscall(333);
    printf("RAID-0 sub-io latency has been dumped to /var/log/kern.log, check it out!\n");

    return 0;
}


To compile the program you can use this command:
$ gcc dump-subio-lat-to-kern-log.c -o a.out
Then simply run the program: $ sudo ./a.out

Note: before triggering the syscall and printing all the sub-IO latencies you can clear the kernel log using this command:     $ echo “” | sudo tee /var/log/kern.log

Now, you should be able to see all the sub-io latencies in the kernel log (/var/log/kern.log). You can check it with this command:
$ sudo tail /var/log/kern.log

The result is like this:
Dec 29 21:37:59 u14s kernel: [  940.999045] 172286, 75588, 67688, 112515
Dec 29 21:37:59 u14s kernel: [  940.999046] 89978, 80849, 67993, 110149
Dec 29 21:37:59 u14s kernel: [  940.999048] 89172, 79623, 69606, 99543
Dec 29 21:37:59 u14s kernel: [  940.999051] 90511, 77371, 67667, 105589
Dec 29 21:37:59 u14s kernel: [  940.999052] 90418, 77241, 67483, 109968
Dec 29 21:37:59 u14s kernel: [  940.999054] 169876, 81299, 67664, 111239
Dec 29 21:37:59 u14s kernel: [  940.999056] 157325, 82066, 68397, 110954
Dec 29 21:37:59 u14s kernel: [  940.999058] 168899, 81844, 68280, 111065
Dec 29 21:37:59 u14s kernel: [  940.999060] 91433, 76437, 69789, 111148
Dec 29 21:37:59 u14s kernel: [  940.999062] 19997, 79027, 69254, 111036

    Each line in the log is all sub-IO latencies spread in 4 disks from a single IO

Measuring RAID 0 performance with FIO (Running the experiments)
All script to run the experiments are available at https://github.com/huaicheng/TeaFA/tree/master/fadhil/slowdown
do_raid_test.sh             main script to run the experiments
raid_test.fio            template fio script for all kwps experiment
raid_test_nogc.fio        template fio script for nogc experiment
get_slowdown_data.cpp        script to extract subio lat from kernel’s log
raw2dat.py            quick plot the slowdown CDF using matplotlib (python2)
Copy all the required script to your VM
Modify array in do_raid_test.sh and raw2dat.py to run various kwps experiments. Use 0 for NoGC experiment.
Edit kwps array in do_raid_test.sh at line 4.
Edit kwps array in raw2dat.py at line 20.
To uncapped the write noise, you need to change do_raid_test.sh ar line 56
From:
    sed -i "42s/^rate_iops=.*/rate_iops=${rate_iops_job}/" ${fio_file}
To:
    sed -i "42s/^rate_iops=.*/rate_iops=${i}k/" ${fio_file}
Run the experiments with ./do_raid_test.sh
The result graph is slodown_cdf.png
You can use slowdown_*kwps.log as a raw data to further processing it with gnuplot
The log output file is formatted like this:
0, 1.53942, 1.06627
1, 1.86535, 1.15772
2, 12.1307, 1.06505
3, 1.39794, 1.04448
4, 3.69124, 1.38545
5, 3.50179, 1.02642
6, 2.08996, 1.04859
7, 1.15993, 1.08276
8, 1.79952, 1.17259
9, 3.14476, 1.06542

The first column is the IO index, the second column is the last slowdown, the third column is the second slowdown.
To make gnuplot graph you can access all the script in TeaFA paper svn repo at Figures/fadhil/varywrite. Copy all the raw data to `raw` directory, modify `all.sh` to access your raw data, then run ./all.sh to get the raw plot file.
Edit the generated raw plot file to plot the graph correctly (coloring, size, unit, line label)

Note: to plot 1st vs 2nd slowdown we can use the same raw data file. Because the rtk script process the data from 2nd column, we need to move the 2nd slowdown data from column 3 to column 2 using awk.
    awk -F"," '{print $1"," $3", "0}' raw_file > raw_file_2nd
Measuring SSD Overall R/W Bandwidth and Latency using FIO

Tested devices:
Micron Technology Inc 9100 PRO NVMe SSD
Intel Corporation Express Flash NVMe P4500/P4600
Samsung MZPLL1T6HEHP-00003
HGST, Inc. Ultrastar SN200 Series NVMe SSD

Detailed results:
https://docs.google.com/spreadsheets/d/14HoIbsjPlaeN1tWU20ylUZ2ur0bZ3EDGR_SNXzDmMps/edit?usp=sharing

Here are the steps used to run the measurement:
Warmup the SSD using dd twice, this is to make sure that the SSD already in steady state. Run this command:

for each in {1..2}; do dd if=/dev/random of=/dev/${ssd} bs=1M oflag=direct ; done &

Run the measurement using fio with these fiofiles. Use --readwrite=randread to measure read latency & BW, and --readwrite=randwrite to measure write latency & BW. Don’t forget to use --filename=/dev/nvme0n1 (or other) to target specific SSD.

Measuring device latency
lat_profiling.fio
[profile]
direct=1
runtime=60
time_based=1
size=6000G
thread
group_reporting
ioengine=libaio
iodepth=1
bs=4k
numjobs=1


Measuring device bandwidth
bw_profiling.fio
[profile]
direct=1
runtime=60
time_based=1
size=6000G
thread
group_reporting
ioengine=libaio
iodepth=128
bs=256k
numjobs=16




Measuring device IOPS
iops_profiling.fio
[profile]
direct=1
e0n1
runtime=60
time_based=1
size=6000G
thread
group_reporting
ioengine=libaio
iodepth=128
bs=4k
numjobs=16


How to adjust Emulated Blackbox FEMU SSD Size

devsz_mb is the size of SSD, used for IO, which will be exposed to the guest OS.
If we want to overprovision the SSD, currently, we can do it by changing parameters in function ssd_init_params() in ftl.c
The parameters including:
spp->secsz = 512;
spp->secs_per_pg = 8;
spp->pgs_per_blk = 256;
spp->blks_per_pl = 832; /* 52GB */
spp->pls_per_lun = 1;
spp->luns_per_ch = 8;
spp->nchs = 8;


By default, the FTL code uses 75% overprovisioning. So for example, if we use devsz_mb = 52GB, then we need to adjust these parameters so that the calculated size is 52 GB / 75% = 69 GB.

