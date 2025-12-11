FROM ubuntu:16.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
# We include all possible dependencies for NS-3, DCE, and Quagga
# Note: Python 3.6 is required for NS-3.35, but 16.04 has 3.5. We use deadsnakes PPA.
RUN apt-get update && apt-get install -y \
    software-properties-common \
    wget \
    vim \
    build-essential \
    && add-apt-repository ppa:git-core/ppa \
    && apt-get update && apt-get install -y git \
    && apt-get install -y \
    zlib1g-dev \
    libncurses5-dev \
    libgdbm-dev \
    libnss3-dev \
    libssl-dev \
    libreadline-dev \
    libffi-dev \
    libsqlite3-dev \
    libbz2-dev \
    liblzma-dev \
    tk-dev \
    libdb-dev \
    libexpat1-dev \
    libmpdec-dev \
    libgirepository1.0-dev \
    libcairo2-dev \
    pkg-config \
    libgsl-dev \
    libgtk-3-dev \
    libxml2-dev \
    flex \
    bison \
    libpcap-dev \
    tcpdump \
    libc6-dbg \
    qt5-default \
    libqt5svg5-dev \
    qtbase5-dev \
    qttools5-dev-tools \
    graphviz-dev \
    gawk \
    libsysfs-dev \
    indent \
    mercurial \
    cvs \
    unzip \
    cmake \
    lsb-release \
    && rm -rf /var/lib/apt/lists/*

# Compile Python 3.6 from source
RUN wget https://www.python.org/ftp/python/3.6.9/Python-3.6.9.tgz && \
    tar xvf Python-3.6.9.tgz && \
    cd Python-3.6.9 && \
    ./configure --enable-optimizations --with-ensurepip=install && \
    make -j4 && \
    make altinstall && \
    cd .. && \
    rm -rf Python-3.6.9 Python-3.6.9.tgz

# Set python3 to use python3.6
RUN update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.6 1
RUN update-alternatives --install /usr/bin/python python /usr/local/bin/python3.6 1

# Upgrade pip using get-pip.py (more robust than pip install --upgrade pip on old systems)
RUN wget https://bootstrap.pypa.io/pip/3.6/get-pip.py && python3.6 get-pip.py && rm get-pip.py

# Install python dependencies via pip (distro is required by bake)
# Pin versions for Ubuntu 16.04 compatibility (cairo 1.14.6, glib 2.48)
RUN pip3 install requests distro pygraphviz "pycairo==1.16.0" "PyGObject==3.28.3"

# Create workspace
WORKDIR /workspace
RUN git clone https://gitlab.com/nsnam/bake.git
ENV PATH=$PATH:/workspace/bake
ENV PYTHONPATH=$PYTHONPATH:/workspace/bake

# Manually download Quagga to /tmp to avoid bake download issues
RUN wget -O /tmp/quagga-0.99.20.tar.gz https://src.fedoraproject.org/repo/pkgs/quagga/quagga-0.99.20.tar.gz/64cc29394eb8a4e24649d19dac868f64/quagga-0.99.20.tar.gz

# Patch bakeconf.xml to use the locally downloaded Quagga file
# This bypasses any network/mirror issues during bake download
RUN sed -i 's|https://src.fedoraproject.org/repo/pkgs/quagga/quagga-0.99.20.tar.gz/64cc29394eb8a4e24649d19dac868f64/quagga-0.99.20.tar.gz|file:///tmp/quagga-0.99.20.tar.gz|g' /workspace/bake/bakeconf.xml

# Configure and build NS-3 with DCE and Quagga using Bake
# We use dce-ns3-1.11 and dce-quagga-1.11
RUN bake.py configure -e dce-ns3-1.11 -e dce-quagga-1.11

RUN bake.py download -vvv

# Disable Werror via environment variable (just in case)
ENV CXXFLAGS="-Wno-error"

RUN bake.py build -vvv

# Set environment variables based on bake install
ENV DCE_PATH=/workspace/source/ns-3-dce/build/bin_dce
ENV DCE_ROOT=/workspace/source/ns-3-dce/build
ENV LD_LIBRARY_PATH=/workspace/source/ns-3-dce/build/lib:/workspace/source/ns-3.35/build/lib

# Copy project files (we will mount them instead for easier editing)
RUN mkdir -p scratch/satnet-rfp

CMD ["/bin/bash"]
