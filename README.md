ZRLMPI
===================
**IBM cloudFPGA message-passing interface for network-attached FPGAS (ZRLMPI)**


Introduction
------------------

This is a implementation of a subset of the [message-passing interface (MPI) standard](https://en.wikipedia.org/wiki/Message_Passing_Interface) for network-attached FPGAs using HLS libraries and the concept of [transpilation](https://ieeexplore.ieee.org/abstract/document/9307074).

ZRLMPI is also a **cloudFPGA addon (cFa)** to enable ZRLMPI on cloudFPGA using the [cFDK](https://github.com/cloudFPGA/cFDK), as explained [below](#Enable-cFa-ZRLMPI).
Additionally, ZRLMPI is used as hardware-agnostic communication framework within the [IBM cloudFPGA Distributed Operator Set Architectures (DOSA)](https://github.com/cloudFPGA/DOSA) compiler.

The goal of ZRLMPI is to bring CPUs and FPGAs to work together efficiently using a single source of code. It is a proof-of-concept implementation to show that it is possible and beneficial to take existing MPI-based applications that were developed for CPU clusters, and execute them on a reconfigurable heterogeneous HPC cluster without any code modifications.

The concepts of ZRLMPI are explained in our two papers [here (PDF)](https://0xcaffee.blog/posts/attachments/FCCM20.pdf) and [here (PDF)](https://0xcaffee.blog/posts/attachments/H2RC20.pdf).
A brief description of the developed interfaces and protocol is documented in [./doc/ZRLMPI_protocol.md](./doc/ZRLMPI_protocol.md). A detailed description of the protocol can be found [here](https://doi.org/10.5281/zenodo.7957659).


Citation
------------------

If you use this software in a publication, please cite our two papers introducing and explaining the concept of *transpilation*:

```
@InProceedings{zrlmpi,
  author       = {Burkhard Ringlein and Francois Abel and Alexander Ditter and Beat Weiss and Christoph Hagleitner and Dietmar Fey},
  title        = {{ZRLMPI: A Unified Programming Model for Reconfigurable Heterogeneous Computing Clusters}},
  year         = {2020},
  month        = may,
  pages        = {220},
  publisher    = {IEEE},
  date         = {3-6 May 2020},
  doi          = {10.1109/FCCM48280.2020.00051},
  isbn         = {978-1-7281-5803-7},
  journaltitle = {Proceedings of the 28\textsuperscript{th} IEEE International Symposium on Field-Programmable Custom Computing Machines (FCCM)},
  location     = {Fayetteville, Arkansas}
}

@InProceedings{zrlmpi_transpilation,
  author       = {Burkhard Ringlein and Francois Abel and Alexander Ditter and Beat Weiss and Christoph Hagleitner and Dietmar Fey},
  journaltitle = {2020 IEEE/ACM International Workshop on Heterogeneous High-performance Reconfigurable Computing (H2RC)},
  title        = {{Programming Reconfigurable Heterogeneous Computing Clusters Using MPI With Transpilation}},
  year         = {2020},
  month        = nov,
  pages        = {1--9},
  publisher    = {IEEE},
  date         = {13 Nov 2020},
  doi          = {10.1109/H2RC51942.2020.00006}
}
```


License
------------------

ZRLMPI is released under the Apache 2.0 License.


Enable cFa ZRLMPI
-----------------

There are two ways to add this cFa to an existing cloudFPGA project (cFp): 

1. By using cFBuild:
```bash
./cFBuild adorn --cfa-repo=git@github.com:cloudFPGA/ZRLMPI.git ZRLMPI <path-to-cFp-folder>
```

2. manually:
```bash
$ cd <existing cFp>
$ git submodule add git@github.com:cloudFPGA/ZRLMPI.git  # or use a zip-folder, if no access to github
$ source env/setenv.sh
$ /bin/bash ./ZRLMPI/install/setup.sh ZRLMPI
$ git add .
$ git commit
```

**The target folder must be named `ZRLMPI`** (in both cases)!


Both procedures should update the `<existing cFp folder>/Makefile` to add new targets, an example is given in `ZRLMPI/install/Makefile.template`.

The Software (SW) application should be placed in `<cFp-folder>/SW/`.


### Developing with ZRLMPI

1. develop software in `<cFp-folder>/APP`
2. call `ZRLMPI.CC`
  - make sure ZRLMPI.CC is build (unifed etc.)
  - it creates SW and HW source files and copies them into their destination (`ROLE/hls/mpi_wrapper/` and `SW/`)
  - it calls all necessary Makefiles
3. call ZRLMPI.RUN 
  - it uploads the image
  - creates/reuse a cluster
  - etc.

The user should not be required to change files in this git (submodule). This git should be only a library to the user, like cFDK.






