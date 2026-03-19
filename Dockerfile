FROM debian:bookworm-slim

RUN apt-get update && \
	apt-get install -y \
	ca-certificates \
	git \
	build-essential	\
	cmake \
	ninja-build \
	curl \
	wget \
	lsb-release \
	software-properties-common \
	gnupg \
	python3 \
	python3-pip \
	python3-requests && \
	update-ca-certificates

RUN wget https://apt.llvm.org/llvm.sh && \
	chmod +x llvm.sh && \
	./llvm.sh 18

RUN apt-get install -y libc++-dev libc++abi-dev

ENV CXX=clang++-18

WORKDIR /Webserv

COPY ./ /Webserv

CMD ["/bin/sh", "-c", "cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Debug && cmake --build build/ --parallel $(nproc) && cd build && ctest --output-on-failure -VV"]