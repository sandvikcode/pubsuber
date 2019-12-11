FROM gcc:9.2

LABEL Description="This image is used to build Pubsuber" Vendor="Sandvik CODE"

RUN apt-get -y update
RUN apt-get install -y software-properties-common
RUN apt-get install -y apt-utils build-essential cmake ninja-build autoconf automake libtool \
 make pkg-config tar git python vim curl unzip \
 libssl-dev zlib1g-dev libc-ares-dev

COPY cook-*.sh /tpsdeps/
WORKDIR /tpsdeps

RUN ./cook-cares.sh
RUN ./cook-protobuf.sh
RUN ./cook-grpc.sh
RUN ./cook-spdlog.sh

COPY . /pubsuber
WORKDIR /pubsuber

CMD ["./cook-this.sh"]
