#include <Sim5320.h>
#include <Sim5320Client.h>
#include <PubSubClient.h>

#define FONA_RST 0

Sim5320       sim(FONA_RST, Serial1);
Sim5320Client sim_client(sim);
PubSubClient  mqtt_client("broker.hivemq.com",1883,NULL,sim_client);


void setup() {
  
    pinMode(FONA_RST,OUTPUT);

    Serial.begin(115200);
    Serial1.begin(115200);

    Serial.println(F("SIM5320 basic test"));
    Serial.println(F("Initializing....(May take 3 seconds)"));

    if(sim.begin())
    {
        Serial.println(F("Device Detected"));
        Serial.println(F("Enabling GPS"));
        sim.enableGPS(true);
        Serial.println(F("Acquiring Network."));
        if(sim.connect("hologram"))
        {
        Serial.println(F("Network acquired."));
        }else{
        Serial.println(F("Could not acquire network."));
        }
    } else {
        Serial.println(F("Could not communicate with Sim"));
        while(1);
    }
    
}

void loop() {
  Serial.print(F("FONA> "));
    while (!Serial.available())
    {
        if (sim.available())
        {
            Serial.write(sim.read());
        }
    }

    char command = Serial.read();
    Serial.println(command);

    switch (command)
    {
    case '?':
    {
        printMenu();
        break;
    }
    case 'm':
    {
        Serial.println("Starting MQTT Client.");
        if (sim.connected())
        {
            Serial.println(F("Connecting to HiveMQ Broker"));
            if (mqtt_client.connect("fonaClient"))
            {
                Serial.println(F("MQTT Client connected."));
                Serial.println("Publishing to /fona3g -> 'online'");
                mqtt_client.publish("/fona3g", "online");

                Serial.println("Subscribing to /fona3g2 Topic");
                mqtt_client.subscribe("/fona3g2");
                
                Serial.println("Looping for MQTT");
                int seconds = 60;
                while(mqtt_client.connected() && sim.connected())
                {
                    if(Serial.available())
                    {
                        char a = Serial.read();
                        if(a == '#')
                        {
                            Serial.println("Task interrupted by user.");
                            break;
                        }
                    }
                    mqtt_client.loop();

                    if(++seconds >= 20)
                    {
                    Serial.println("Sending GPS Coordinate");
                    char gpsbuffer[120];
                    sim.getGPS(0,gpsbuffer,120);
                    mqtt_client.publish("/fona3g", gpsbuffer);
                    seconds = 0;
                    }
                    delay(500);
                }

                //mqtt_client.subscribe("inTopic");
                mqtt_client.disconnect();
                Serial.println("MQTT Client disconnected.");
            }
            else
            {
                Serial.println(F("Could not connect to MQTT Server"));
            }
        } else {
            Serial.println("Network disconnected.");
        }
        break;
    }
       case 'S':
    {
        Serial.println(F("Creating SERIAL TUBE"));
        bool passthru_enable = true;
        while (passthru_enable)
        {
            while (Serial.available())
            {
                delay(1);
                char a = Serial.read();
                if(a == '#')
                {
                    passthru_enable = false;
                } else {
                    sim.write(a);
                }
            }
            if (sim.available())
            {
                Serial.write(sim.read());
            }
        }
        break;
    }

    default:
    {
        Serial.println(F("Unknown command"));
        printMenu();
        break;
    }
    }
    // flush input
    flushSerial();
    while (sim.available())
    {
        Serial.write(sim.read());
    }
  
}

void flushSerial()
{
    while (Serial.available())
        Serial.read();
}

void printMenu(void)
{
    Serial.println(F("-------------------------------------"));
    Serial.println(F("[m] Start MQTT Client"));
    Serial.println(F("[?] Print this menu"));
    Serial.println(F("[S] create Serial passthru tunnel"));
    Serial.println(F("-------------------------------------"));
}