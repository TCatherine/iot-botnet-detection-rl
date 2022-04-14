FROM ubuntu:20.04

RUN apt-get update
RUN apt-get -y install gcc g++ python
RUN apt-get -y install software-properties-common

RUN add-apt-repository --remove ppa:maarten-fonville/protobuf
RUN apt-get update

RUN apt-get -y install libzmq5 libzmq5-dev
RUN apt-get -y install libprotobuf-dev
RUN apt-get -y install protobuf-compiler

EXPOSE 5555 5556 5557 5558

ADD . /ns-3
WORKDIR /ns-3

