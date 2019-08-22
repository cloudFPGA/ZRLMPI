ZRLMPI
===================
**MPI Implementation for cloudFPGA**

This is a **cloudFPGA addon (cFa)** to enable ZRLMPI on clodFPGA.

Enable cFa ZRLMPI
-----------------

There are two ways to add this cFa to an existing cloudFPGA project (cFp): 

1. By using cFBuild:
```bash
./cFBuild adorn --cfa-repo=git@github.ibm.com:cloudFPGA/ZRLMPI.git ZRLMPI <path-to-cFp-folder>
```

2. manually:
```bash
$ cd <existing cFp>
$ git submodule add git@github.ibm.com:cloudFPGA/ZRLMPI.git  # or use a zip-folder, if no access to github
$ source env/setenv.sh
$ /bin/bash ./ZRLMPI/install/setup.sh ZRLMPI
$ git add .
$ git commit
```

**The target folder must be named `ZRLMPI`** (in both cases)!


Both procedures should update the `<existing cFp folder>/Makefile` to add new targets, an example is given in `ZRLMPI/install/Makefile.template`.

The Software (SW) application should be placed in `<cFp-folder>/SW/`.


About ZRLMPI
--------------

TODO 




