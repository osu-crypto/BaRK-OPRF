# Batched Oblivious PRF
This is the implementation of our CCS 2016 paper: **Efficient Batched Oblivious PRF with Applications to Private Set Intersection**[[ePrint](https://...)]. Our code utilizes some parts for OT Extension implementation, and the [`libPSI`] (https://github.com/osu-crypto/libPSI) framework provided by [Peter Rindal](http://web.engr.oregonstate.edu/~rindalp/). We would like to thank Peter Rindal for contributing libraries and helpful suggestions to our protocol implementation. For any questions related to the implementation, please contact Ni Trieu at trieun@oregonstate.edu

## Abstract
---
.......

## Installations
---
###Required libraries
  * [`Boost`](https://sourceforge.net/projects/boost/)
  * [`Crypto++`](http://www.cryptopp.com/)
  * [`Miracl`](https://github.com/miracl/MIRACL)
  * [`Mpir`](http://mpir.org/)
  
Please read [`libPSI`](https://github.com/osu-crypto/libPSI) for more detail about how to install the required libraries
### Building the Project
After cloning project from git,
##### Windows:
1. build pOPRFlib project
2. add argument for bOPRFmain project (for example: -t)
3. run bOPRFmain project
 
##### Linux
1. make
2. for test:
	./Release/main.exe -t
	
### Test
Our database is generated randomly. We have 2 functions: 
#### 1. Unit Test: 
test PSI result for a small number of inputs (2^12), shows whether the program computes a right PSI. This test runs on one terminal:

	./main.exe -t
	
#### 2. Simulation: 
Using two terminals, compute PSI in 6 cases with the number of input (2^8, 2^12, 2^16, 2^18, 2^20, 2^24). For each case, we run the code 10 times to compute PSI. The outputs include the average online/offline/total runtime (displayed on the screen) and the output.txt file. Note that these parameters can be customized in the code.
On the Sender's terminal, run:

	./main.exe -r 0
	
On the Receiver's terminal, run:
	
	./main.exe -r 1
### NOTE
Currently, we implement bOPRF and PSI in the same module. We plan to separate the code for each problem shortly. We will also consider multi-threaded implementation in our future work.
## References
---
[1] 
[2]