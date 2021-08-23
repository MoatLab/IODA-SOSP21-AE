
# IODA Artifact Evaluation - SOSP'21 #

### Overview of the Artifact

IODA artifact includes the following components:

- ``iodaFEMU``: IODA-enhanced SSD controller
- ``iodaLinux``: IODA-enahnced Linux kernel (based on Linux v4.15)
- ``iodaVM``: a QEMU VM image hosting utilities to run IODA experiments

All the experiments will run inside ``iodaVM``, where it uses ``iodaLinux``
as the guest OS and manages a NVMe SSD exposed by ``iodaFEMU``. 

### Foreword ###

- This artifact is mainly setup for reproducing ``Figure 5`` (and
  correspondingly ``Figure 6``) in our submission, which contains IODA results
  of 9 trace workloads, each under 6 IODA modes (``Base``, ``IOD_1``,
  ``IOD_2``, ``IOD_3``, ``IODA``, and ``Ideal``)

- To simplify the evaluation process, we **encourage** you to use our
  pre-compiled Linux kernel ``bzImage`` of ``iodaLinux`` (already shipped
  with``iodaVM``) to save time. 

- All the experiments are done on ``Emulab D430`` nodes, tested under ``Ubuntu
  16.04.1 LTS, GCC: 5.4.0`` and ``Ubuntu 20.04 LTS, GCC: 9.3.0`` (recommended).
  - If you choose to use other types of servers, please make sure it has at
    least 32 cores, 64GB DRAM, and 80GB disk space, and better stick to a
    similar host OS environment as mentioned above.

- Estimated time to finish all these experiments: ``10-20 hours``

**To showcase the steps to setup and run IODA experiments, please refer to our
screencast at https://asciinema.org/a/431726. We encourage you to watch the
video first before following the detailed instructions below.**

### Detailed Steps

#### Prepare the physical server

Setup an Emulab D430 server, ssh into it. If you don't have Emulab/CloudLab
access, please let us know on hotcrp and we can help spin up a server under our
account and provide you the access.

#### Prepare the IODA environment

Clone the repo and download IODA VM image file: 

```
mkdir -p ~/git
cd ~/git
git clone https://github.com/huaicheng/IODA-SOSP21-AE.git
ln -s IODA-SOSP21-AE ae
cd ae
export IODA_AE_TOPDIR=$(pwd)
cd images
./download-ioda-vm-image.sh
cd ${IODA_AE_TOPDIR}
```
At this point directory hierarchy should be like this:

```

├── build.sh                        # Script to build IODA
├── images
│   ├── download-ioda-vm-image.sh
│   └── ioda.qcow2                  # IODA VM image
├── README.md                       # README with detailed instructions
├── rtk                             # Plotting toolkit
│   ├── all.sh
│   ├── clear.sh
│   ├── dat
│   ├── eps
│   ├── plot
│   ├── script
│   └── w.txt
├── run-vm.sh                       # Script to run IODA VM
├── src
│   ├── iodaFEMU                    # IODA-enhanced FEMU SSD controller
│   └── iodaLinux                   # IODA-enhanced Linux kernel (v4.15)
└── utils
    ├── iodaExp                     # Utilities to run IODA experiments in IODA VM (the VM already has it setup)
    └── ioda-pkgdep.sh              # Script to install IODA dependencies

```


#### Build IODA

```
$ sudo ./build.sh
```
This script will install IODA dependencies, and build iodaFEMU and iodaLinux.
The compiled binaries are:

- iodaFEMU: ``src/iodaFEMU/build-femu/x86_64-softmmu/qemu-system-x86_64``
- iodaLinux: ``src/iodaLinux/arch/x86/boot/bzImage``

#### Run the Experiments

First, start IODA VM and enter the guest OS:

```
cd ${IODA_AE_TOPDIR}
./run-vm.sh
```

FEMU will start booting and keep printing output for about 30-60 seconds. Once
the guest OS is up, ssh into it using the following account

    username: huaicheng
    password: ii

Specifically, do the following

```
$ ssh -p10101 huaicheng@localhost
# then input the password
```

**Note: From now on, all the operations are done in the VM.**

We provide some automation scripts in the VM to simplify the process to run the
experiments:

```
$ cd iodaExp
$ source ioda-env.sh # setup IODA env variables
$ cd traceExp
$ ss # this will create an RAID-5 array and age the FEMU SSDs

$ source r.sh # r.sh defines several functions to run experiments in batch, check it out

# A test run, let's run a "test" workload under all IODA modes, after it finishes, the latency log files are under "sosp21-ae-rst/"
$ run_sgl_all test

# Now, to run all experiments in batch: (this will take >10hours to finish, please wait patiently)
$ run_bat_all 

# After it finishes, similarly check all the raw latency logs under "sosp21-ae-rst" and use them for plotting
```


### Plotting

The ``rtk`` contains scripts to plot the latency CDFs using ``gnuplot``. If the
server you use for running IODA experiments doesn't have GUI, you could
consider copying the ``rtk`` folder to a desktop Linux machine with ``gnuplot``
installed.

The ``rtk`` folder contains the latency percentile data used in the paper. To
replot the graph, simply run ``cd rtk; gnuplot plot/all.plt``. The output
figure is ``eps/all.eps``.

After you have finished all the experiments, first copy out the corresponding
read latency log files to ``rtk/raw/xxx``. For example, copy
``tpcc-{base,iod1,iod2,iod3,ioda,ideal}-rd_lat.log`` from the IODA VM to
``rtk/raw/tpcc/tpcc-{base,iod1,iod2,iod3,ioda,ideal}.log``.  Then, run ``cd
rtk; ./all.sh tpcc`` to refresh the percentile data. Repeat this step for all
the workloads. Then, ``gnuplot plot/all.plt`` again to generate the final
figure. 

Check ``eps/all.eps`` for the new results.
