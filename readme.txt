# minimal requirements for C++:
apt-get install gcc g++ python

# to install protobuf-3.6 on ubuntu 16.04:
sudo add-apt-repository ppa:maarten-fonville/protobuf
sudo apt-get update

apt-get install libzmq5 libzmq5-dev
apt-get install libprotobuf-dev
apt-get install protobuf-compiler

# build enviroment
./waf configure

# Python request
pip install torch numpy mlflow
pip3 install ./src/opengym/model/ns3gym

#run enviroment
./waf --run "my_env"

#run python
cd agent
./python main.py
