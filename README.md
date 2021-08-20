
# IODA Artifact Evaluation - SOSP'21 #

### Overview of the Artifact

IODA artifact includes the following components:

- ``iodaFEMU``: IODA-enhanced SSD controller
- ``iodaLinux``: IODA-enahnced Linux kernel
- ``iodaVM``: a QEMU VM image hosting utilities to run IODA experiments (ucare-07.cs.uchicago.edu:/home/martin/images)

All the experiments will run inside ``iodaVM``, where it uses ``iodaLinux``
as the guest OS and manages a NVMe SSD exposed by ``iodaFEMU``. 

### Foreword ###

- This artifact is mainly setup for reproducing ``Figure 5`` (and
  correspondingly ``Figure 6``) in our submission, which contains IODA results
  of 9 trace workloads, each under 6 IODA modes (``Base``, ``IOD_1``,
  ``IOD_2``, ``IOD_3``, ``IODA``, and ``Ideal``)

- To simplify the evaluation process, we **encourage** you to use our
  pre-compiled binary files of ``iodaFEMU`` and ``iodaLinux`` to save
  compilation time. We also provide compilation instructions for those
  interested.

- All the experiments were done on ``Emulab D430`` machines, with ``Ubuntu
  16.04.1 LTS, GCC: 5.4.0``
  - If you use different servers, please make sure the server has at least 32
    cores, 64GB DRAM, and 80GB disk space

- Estimated time to finish all these experiments: ``1 hours``

### Detailed Steps

0. Prepare the server: Setup an Emulab D430 server, ssh into it. [Martin: TODO]

1. Prepare the IODA environment


Clone the repo and download IODA VM image file: 

```
git clone https://github.com/huaicheng/IODA-AE.git
cd IODA-AE
export IODA_AE_TOPDIR=$(pwd)
cd images
./download-ioda-vm-image.sh
```
At this point directory hierarchy should be like this:

```
├── README.md                                    # README with detailed instructions
├── bin
│   ├── iodaFemuBin                              # Pre-compiled IODA FEMU executable file
│   └── iodaLinuxBin                             # Pre-compiled IODA Linux kernel image
├── images
│   ├── download-ioda-vm-image.sh
│   └── ioda.qcow2                               # IODA VM image file
├── ioda.sh                                      # Script to start IODA VM
├── rtk
└── src                                          # Source code of iodaFEMU and iodaLinux
    ├── iodaFEMU
    └── iodaLinux
```

2. Install IODA dependencies

```
$ sudo ./pkgdep.sh
```

3. Running the Experiments

First of all, to showcase how to run IODA experiments, please refer to our
screencast at http://XXXX to get an idea about each steps.

1) Steps to run the experiment:

Login to machine (suppose emulab using Martin's account)

We will spin a few Emulab servers for you to use if needed, please let us know.

```
$ ssh -p22 <user>@<machine>.emulab.net
```

Start IODA VM and enter the guest OS:

```
cd ioda/sosp21/
./ioda.sh
```

FEMU will start booting and keep printing output for about 30-60 seconds. Once
it is done, our VM will be available at localhost port 10101.

After the VM is up, we can connect to the VM using the following account

    username: huaicheng
    password: ii

Furthermore, the provided image allows passwordless ssh if we run it on emulab.
Thus we can ssh into our VM using the following command:

$ ssh -p10101 huaicheng@localhost

By default we will enter /home/huaicheng/tifa/replayer directory:

Our main script is ``r.sh``. It automates all the steps we need, including

- Create RAID-5 array using 4 FEMU NVMe SSDs
- Warmup the array to make it reach steady-state, such that further workload
  will trigger garbage collection
- Run trace Azure,[BingIdx,BingSel,Cosmos,DTRS,Exch,LMBE,MSNFS,TPCC] using the
  base,[iod1, iod2, iod3, ioda, ideal] policy N times Collect and preprocess IO
  latencies and various metrics.
- Organize output latency logs in directories for easy download from the host
  side.

- Regarding ``r.sh``, Line 12 to change traces (e.g. run tpcc and exchange)

tfs="tpcc-resize-w16.0-20s est-trim-s2700-e3300"

The Complete list of traces used:

```
Trace Filename
Azure azurestorage-drive2.trace-trim-s350-e530-rerate-0.25-resize-w16.0
BingSelection bingselection-drive2.trace-trim-s200-e260-resize-w2.0-r2.0
BingIndex bingindex-drive2.trace-trim-s6300-e6600-rerate-2.0-resize-w4.0
Cosmos cosmos-drive2.trace-trim-s1890-e2000
DTRS dtrs-frankenstein2
Exchange est-trim-s2700-e3300
LMBE lmbe-resize-r0.25-w3.0-trim-s1600-e1660
MSNFS msnfs10-20s
TPCC tpcc-resize-w16.0-20s
```

- Modify Line 13 to change number of runs for each {trace,policy} (e.g. run the {trace,policy} 3 times)
``nums="1 2 3"``

- Line 27 to change policy (e.g. run base and iod-1) ``for i in "nopgc nosync def" "nopgc nosync gct"; do``

  - Complete list of existing policies:

Policy Change in Script
```
base "nopgc nosync def"
iod-1 "nopgc nosync ebusy"
iod-2 "nopgc nosync gct"
iod-3 "nopgc sync100ms gct"
ioda "nopgc sync100ms ktw" (need to change kernel)
ideal "nopgc nosync nogc"
```

After deciding the traces, policies, and number of runs, simply run the script:

```
$ ./r.sh
```

We need to re-run FEMU using another kernel to use the ``IOD_3`` policy. We
have provided the bzImage. To do this, shutdown the VM:

```
sudo shutdown -h now
```

This will bring us back to the host side. Open ioda.sh

$ vi ioda.sh

Then, comment out line 10 and uncomment line 11.

Before: TODO

After: TODO

Repeat the above steps for all traces and policies we need to run other workloads.


- Plotting the Results

After all is VM-side experiment is done, switch to host and go to rtk directory:

```
$ cd sosp21/rtk
```

Make sure the VM is still up.
Download and process the experiment results. For all traces, run:

```
$ ./doscp.sh sosp21 <trace filename>
```
E.g. to download TPCC and LMBE results:

```
$ ./doscp.sh sosp21 tpcc-resize-w16.0-20s
$ ./doscp.sh sosp21 lmbe-resize-r0.25-w3.0-trim-s1600-e1660
```

The script will:

  - Download all results for a given trace.
  - Create a CDF of latency for every {trace,policy,run} and put them in sosp21/rtk/dat folder.

(optional) If the host machine is headless, most probably we need to move the results to a machine which has GUI. We only need to download the sosp/rtk/dat folder:

    E.g. Download dat folder from emulab to local machine
    rsync -azP -e 'ssh -p22' <user>@<emulab_machine>:<prefix>/sosp21/rtk/dat dat

Make an empty plot and eps folder at the directory with dat.

$ mkdir plot
$ mkdir eps

At this point the directory hierarchy should be similar to this:

We use gnuplot to generate our graph. We can create our own plot file, but since it might be a bit cumbersome to tune the graph style, we provided several templates here:

- TODO

Adjust the file which we want to plot on the ``'plot \'`` section.
E.g. For TPCC, if we want to plot the 1st run of all policies, change every '<long_string>-X-rd_lat.dat' to '<long_string>1-rd_lat.dat'.

The resulting graph will be in folder ``eps/``. Check the graph and tune its xlim, ylim, etc as desired.






--------------------------------------------------------------------------------


### Optinal steps to compile iodaFEMU and iodaLinux from scratch 

- To build iodaFEMU and iodaLinux

```
$ cd $IODA_AE_TOPDIR/src/iodaFEMU
$ git checkout 49b768b
$ mkdir build-femu
$ cd build-femu
$ cp ../femu-scripts/femu-copy-scripts.sh .
$ ./femu-copy-scripts.sh
$ ./femu-compile.sh

# After the above steps, the IODA FEMU binary will appear as build-femu/qemu-system-x86_64, to use it, change ``ioda.sh`` Line 5 to ``IODA_FEMU="src/iodaFEMU/build-femu/qemu-system-x86_64`` to this FEMU binary.
```

- To build ``iodaLinux`` kernel

```
$ cd $IODA_AE_TOPDIR/src/iodaLinux
$ git checkout 900e39b
$ cp ../ioda-config .config
$ make oldconfig
$ make -j32

The kernel bzImage is under ``./arch/x86_64/boot/bzImage``
```

IODA: FEMU2.0/tw_and_proactive
Commit:900e39b

Other: iodaLinux/tifa-mk-415-base
Commit: 197ee12

