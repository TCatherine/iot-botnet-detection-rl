FROM python:3.9-slim

EXPOSE 5555 5556 5557 5558

COPY . ./agents

RUN python3 -m pip install -r./agents/requirements.txt
RUN python3 -m pip3 install ./../ns-3.29/src/opengym/model/ns3gym


CMD ["python3", "./main.py"]