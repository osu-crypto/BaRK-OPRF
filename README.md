# Batched Oblivious PRF
This is the implementation of our [CCS 2016](http://dl.acm.org/citation.cfm?id=2978381)  paper: **Efficient Batched Oblivious PRF with Applications to Private Set Intersection**[[ePrint](https://eprint.iacr.org/2016/799)]. 

Evaluating on a single server (`2 36-cores Intel Xeon CPU E5-2699 v3 @ 2.30GHz and 256GB of RAM`) with a single thread per party, our protocol requires only `3.8` seconds to securely compute the intersection of `2^20`-size sets, regardless of the bit length of the items.

## Installations

### Quick Start with Docker

- For Internet less user, You can download [`boost_1_64_0.tar.bz2`](http://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.bz2/download) , [`cryptopp562.zip`](http://www.cryptopp.com/cryptopp562.zip) , [`mpir-2.7.0.tar.bz2`](http://mpir.org/mpir-2.7.0.tar.bz2) into [`./docker/`](docker/) directory manually before  run docker build.

```bash
docker build -f docker/Dockerfile -t bark_oprf:quick_start .
docker run -it bark_oprf:quick_start /bin/bash
bOPRFmain #inside container
```



### Required libraries

 C++ compiler with C++11 support. There are several library dependencies including [`Boost`](https://sourceforge.net/projects/boost/), [`Crypto++`](http://www.cryptopp.com/), [`Miracl`](https://github.com/miracl/MIRACL), and [`Mpir`](http://mpir.org/). Our code has been tested on both Windows (Microsoft Visual Studio) and Linux. To install the required libraries: 
  * windows: open PowerShell,  `cd ./thirdparty`, and `.\all_win.ps1` 
  * linux: `cd ./thirdparty`, and `bash .\all_linux.get`.

 #### Some Compiling Issues & How to Fix (raised by users):
1. Problem with compiling mpir: dowgrade sed to version 4.2.2-8 (for Debian Sid). Read more [`here`](https://github.com/wbhart/mpir/pull/184)
2. Run this project with version>=6 of g++:  add static casts `(static_cast<int>(0x...))` to lines 27-34 in wake.cpp of crypto++
3. Error with `_mm_cvtsi64_si128`: try on a 64 bit system

### Building the Project
After cloning project from git,
##### Windows:
1. build bOPRFlib project
2. add argument for bOPRFmain project (for example: -t)
3. run bOPRFmain project

##### Linux:
1. make
2. for test:
	./Release/bOPRFmain.exe -t

## Test

Our database is generated randomly. We have 2 functions: 
#### 1. Unit Test: 
test PSI result for a small number of inputs (2^12), shows whether the program computes a right PSI. This test runs on one terminal:

	./Release/bOPRFmain.exe -t

#### 2. Simulation: 
Using two terminals, compute PSI in 6 cases with the number of input (2^8, 2^12, 2^16, 2^20, 2^24). For each case, we run the code 10 times to compute PSI. The outputs include the average online/offline/total runtime (displayed on the screen) and the output.txt file. Note that these parameters can be customized in the code.

##### Same machine
On the Sender's terminal, run:

	./Release/bOPRFmain.exe -r 0

On the Receiver's terminal, run:
	
	./Release/bOPRFmain.exe -r 1

##### Different machine	
On the Sender's terminal, run:

	./Release/bOPRFmain.exe -r 0 -ip <ipAdrress:portNumber>

On the Receiver's terminal, run:
	
	./Release/bOPRFmain.exe -r 1 -ip <ipAdrress:portNumber> 	

## Acknowledgements
Our code utilizes some parts of: 
* [`cryptoTools`](https://github.com/ladnir/cryptoTools) provided by [Peter Rindal](http://web.engr.oregonstate.edu/~rindalp/). We would like to thank Peter Rindal for contributing libraries and helpful suggestions to our protocol implementation. 
* [`ENCRYPTO_utils`](https://github.com/encryptogroup/ENCRYPTO_utils) provided by [Michael Zohner](https://sites.google.com/site/mizohner/).

We would like to thank all the users for pointing out the compiling bugs that appear on different operation systems and supporting us to fix them

For computing 2-party PSI with **NO** stash bins, we refer to efficient  [`libPSI`](https://github.com/osu-crypto/libPSI).

## Help
For any questions on building or running the library, please contact [`Ni Trieu`](http://people.oregonstate.edu/~trieun/) at trieun at oregonstate dot edu

