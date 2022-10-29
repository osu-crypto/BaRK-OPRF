FROM centos:7.9.2009 AS compile_stage

WORKDIR /tmp/BaRK-OPRF/
COPY bOPRFlib ./bOPRFlib
COPY bOPRFmain ./bOPRFmain
COPY thirdparty ./thirdparty
COPY docker ./docker
COPY ["LICENSE", "makefile", "release.mak", "debug.mak", "./"]

#### install yum packages
RUN yum upgrade -y && \
yum groupinstall -y "Development Tools" && \
yum install -y boost wget which

#### build boost
RUN if [ ! -e docker/boost_1_64_0.tar.bz2 ] ; then \
    wget -c 'http://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.bz2/download' -O docker/boost_1_64_0.tar.bz2 ; \
fi && \
tar jxvf docker/boost_1_64_0.tar.bz2 -C thirdparty/linux && \
mv thirdparty/linux/boost_1_64_0 thirdparty/linux/boost && \
cd thirdparty/linux/boost/ && \
./bootstrap.sh && \
./b2 stage --with-system --with-thread --with-filesystem link=static -mt && \
cd ../../../ && \
mkdir thirdparty/linux/includes && \
cp -r thirdparty/linux/boost/boost/ thirdparty/linux/includes/ && \
cp -r thirdparty/linux/boost/boost/* thirdparty/linux/boost/

#### build cryptopp
RUN \
if [ ! -e docker/cryptopp562.zip ] ; then \
    wget http://www.cryptopp.com/cryptopp562.zip -O docker/cryptopp562.zip ; \
fi && \
unzip docker/cryptopp562.zip -d thirdparty/linux/cryptopp && \
cd thirdparty/linux/cryptopp && \
cp ../cryptopp_patch/* . && \
make -j8

#### build miracl_osmt
RUN \
cd thirdparty/linux/miracl/miracl_osmt/source && \
bash linux64_cpp

#### build mpir
RUN  \
if [ ! -e docker/mpir-2.7.0.tar.bz2 ] ; then \
    wget http://mpir.org/mpir-2.7.0.tar.bz2 ; \
fi && \
tar jxvf docker/mpir-2.7.0.tar.bz2 && \
mv mpir-2.7.0 thirdparty/linux/mpir && \
cp thirdparty/linux/mpir_patch/mpirxx.h thirdparty/linux/mpir/ && \
cp thirdparty/linux/mpir_patch/mpir.h thirdparty/linux/mpir/ && \
cd thirdparty/linux/mpir && \
./configure && make -j8


#### build
RUN make -j8

#### release stage
FROM centos:7.9.2009
COPY --from=compile_stage ["/tmp/BaRK-OPRF/Release/bOPRFmain.exe", "/tmp/BaRK-OPRF/LICENSE", "/opt/BaRK-OPRF/"]
RUN ln -s /opt/BaRK-OPRF/bOPRFmain.exe /usr/bin/bOPRFmain
