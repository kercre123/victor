from centos:centos6

ENV INSTAREA /usr
ENV IGPROF_VERSION 5.9.10
ENV LIBATOMIC_VERSION 7.2alpha4
ENV LIBUNWIND_VERSION 1.1

RUN yum update -y
RUN yum install -y git
RUN curl -s -L -O http://www.hpl.hp.com/research/linux/atomic_ops/download/libatomic_ops-$LIBATOMIC_VERSION.tar.gz
RUN curl -s -L -O http://download.savannah.gnu.org/releases/libunwind/libunwind-$LIBUNWIND_VERSION.tar.gz
RUN git clone https://github.com/igprof/igprof

RUN yum install -y tar make cmake gcc-c++ automake autoconf
RUN yum install -y gdb pcre-devel

RUN tar xzvf libatomic_ops-$LIBATOMIC_VERSION.tar.gz
RUN cd libatomic_ops-$LIBATOMIC_VERSION && ./configure --prefix=$INSTAREA && make -j 10 install

RUN gtar xzf libunwind-$LIBUNWIND_VERSION.tar.gz
RUN cd libunwind-$LIBUNWIND_VERSION && ./configure CPPFLAGS="-I$INSTAREA/include" CFLAGS="-g -O3" --prefix=$INSTAREA --disable-block-signals
RUN cd libunwind-$LIBUNWIND_VERSION && make -j 10 install

RUN cd igprof && cmake -DCMAKE_INSTALL_PREFIX=$INSTAREA -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-g -O3" . && make -j 20
RUN cd igprof && make install
RUN ln -sf /usr/lib/libigprof.so /usr/lib64/libigprof.so
RUN ln -sf /usr/lib/libunwind.so.8 /usr/lib64/libunwind.so.8

