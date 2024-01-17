#include <Arduino.h>
#include <stdio.h>

//Pin allocation
int ledpin1 = 0;
int ledpin2 = 1;
int ledpin3 = 2;
int ledpin4 = 13;
int rstpin = 3;
int cfgpin = 4;
int rtspin = 5;

int serial_wait(int delay_microsec);
int serial2_wait(int delay_microsec);

void setup()
{
  //Pin initialisation
  {
    pinMode(ledpin1, OUTPUT);
    pinMode(ledpin2, OUTPUT);
    pinMode(ledpin3, OUTPUT);
    pinMode(ledpin4, OUTPUT);
    pinMode(rstpin, OUTPUT);
    pinMode(cfgpin, OUTPUT);
    pinMode(rtspin, OUTPUT);
  }
  
  //Pin standard mode
  {
    digitalWrite(rstpin, HIGH);
    digitalWrite(cfgpin, HIGH);
    digitalWrite(rtspin, HIGH);
    
    digitalWrite(ledpin1, LOW);
    digitalWrite(ledpin2, LOW);
    digitalWrite(ledpin3, LOW);
    digitalWrite(ledpin4, HIGH);
  }

  //Serial configuration
  {
    Serial.begin(19200);
    Serial2.setRX(7);
    Serial2.begin(19200);
  }
  
  //Waiting for first user input
  {
    while(Serial.available() == 0);
    Serial.read();
    Serial.print("Setup complete. Entering normal operation mode. | ");
  }
}

void loop()
{
  // [1xxx] Serial from module
  if(Serial2.available() != 0)
  {
    char InBuffer[8] = {0};
    //Puts received string into 8 byte array
    for(int i = 0; i < 8 && serial2_wait(600); i++)
    {
      InBuffer[i] = Serial2.read();
    }
    int q;
    //Checks for a "C" char within the input
    for(q = 0; InBuffer[q] != 'C' && q <= 4; q++);
    //Checks for the string "CMD" within the Input
    if(InBuffer[q] == 'C' && InBuffer[q+1] == 'M' && InBuffer[q+2] == 'D') 
    {
      //Checks for a char "0" following a "CMD" string to enter testmode 0
      if(InBuffer[q+3] == '0') 
      {
        //Engages configuration mode
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          //Engages testmode 0
          Serial2.write('0');
          Serial.write("Test 0 command received. Transmitting configuration data. | ");
          serial2_wait(5000);
          char OutBuffer[129] = {0};
          //The testmode 0 data is put into an output buffer
          for(int i=0; serial2_wait(5000) != 0; i++) 
          {
            OutBuffer[i] = Serial2.read();
          }
          //Sends output buffer to receiver
          for(int j=0; j<129; j++) 
          {
            Serial2.write(OutBuffer[j]);
          }
        }
      }
      else
      {
        Serial.write("Error 1001 (irregular command received). Printing the input data and entering normal operation mode. ");
        for(int i = 0; i < 8; i++)
        {
          Serial.write(InBuffer[i]);
        }
        Serial.print(" | ");
      } 
    }
    //No command detected
    else
    {
      for(int i = 0; i < 8; i++)
      {
        Serial.write(InBuffer[i]);
      }
      Serial.print(" | ");
    }
  }
    

  // [2xxx] Serial from user
  if(Serial.available() != 0)
  {
    char inByte = Serial.read();

    // [21xx] Sending mode
    if(inByte == 's' || inByte == 'S')
    {
      Serial.print("Entered sending mode. Enter the string to be transmitted. ");
      char inBuffer[129] = {0};
      while(Serial.available() == 0);
      while(Serial.available() != 0)
      {
        int i;
        for(i = 0; i < 128 && Serial.available(); i++)
        {
          inBuffer[i] = Serial.read();
        }
        Serial.print("Successfully transmitted ");
        for(int j = 0; j < i; j++)
        {
          Serial2.write(inBuffer[j]);
          Serial.write(inBuffer[j]);
        }
        Serial.print(". ");
        delay(20);
      }
      Serial.print("Sending complete. Entering normal operation mode. | ");
    }

    // [22xx] Configuration mode
    else if(inByte == 'c' || inByte == 'C')
    {
      digitalWrite(cfgpin, LOW);
      serial2_wait(5000);
      digitalWrite(cfgpin, HIGH);
      //Check for regular feedback from module
      if(Serial2.available() == 1 && Serial2.read() == '>')
      {
        Serial.print("Entered configuration mode. Enter the desired command. Enter X to exit. ");
        Serial.write('>');
        //Entering communication loop
        while(true)
        {
          //Waiting for user input
          while(Serial.available() == 0);
          Serial2.write(Serial.read());

          //Waiting for module response
          serial2_wait(5000);
          if(Serial2.available() == 0)
          {
            break;
          }
          else
          {
            while(serial2_wait(5000) != 0)
            {
              Serial.write(Serial2.read());
            }
          }
        }
        Serial.print("Configuration finished. Entering normal operation mode. | ");
      }
      else
      {
        Serial2.write('X');
        Serial.print("Error 2201 (received either no or irregular response from module). Entering normal operation mode. | ");
      }
    }

    // [23xx] Command mode
    else if(inByte == 'b' || inByte == 'B')
    {
      Serial.print("Entered command mode. Enter M for memory reset, N for memory configuration, R for reset, S for RSSI reading, U for temperature reading, V for voltage reading, Z for sleep mode and 0 for non-volatile memory reading. Enter X to Exit. | ");
      while(!Serial.available());
      inByte = Serial.read();

      // [231x] Exit command mode
      if(inByte == 'x' || inByte == 'X')
      {
        Serial.print("Entering normal operation mode. | ");
      }

      // [232x] Memory reset
      else if(inByte == 'm' || inByte == 'M')
      {
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        //Check for feedback from module
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          Serial2.print("@RC");
          serial2_wait(5000);
          if(Serial2.available() == 1 && Serial2.read() == '>')
          {
            Serial.print("Memory reset complete. Entering normal operation mode. | ");
          }
          else
          {
            Serial.print("Error 2322 (received either no or irregular response from module). Entering normal operation mode. | ");
          }
        }
        else
        {
          Serial.print("Error 2321 (received either no or irregular response from module). Entering normal operation mode. | ");
        }
        Serial2.write('X');
      }

      // [233x] Memory configuration
      else if(inByte == 'n' || inByte == 'N')
      {
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        //Check for feedback from module
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          Serial2.write('M');
          serial2_wait(5000);
          if(Serial2.available() == 1 && Serial2.read() == '>')
          {
            Serial.print("Entered memory configuration. Enter the desired address. Enter X to exit. ");
            if(serial_wait(20000000) != 0)
            {
              inByte = Serial.read();
              if(inByte == 'X')
              {
                Serial.print("Exiting memory configuration. Entering normal operation mode. | ");
              }
              else if(inByte <= 0x35)
              {
                Serial2.write(Serial.read());
                Serial.print("Enter the desired value. ");
                if(serial_wait(20000000) != 0)
                {
                  inByte = Serial.read();
                  if(inByte <= 0x35)
                  {
                    Serial2.write(Serial.read());
                    Serial.print("Finished memory configuration. Entering normal operation mode. | ");
                  }
                  else 
                  {
                    Serial.print("Error 2334 (received irregular input from user). Entering normal operation mode. | ");
                  }
                }
              }
              else
              {
                Serial.print("Error 2333 (received irregular input from user). Entering normal operation mode. | ");
              }
            }
            Serial2.write(0xFF);
            while(Serial2.available())
            {
              Serial2.read();
            }
          }
          else
          {
            Serial.print("Error 2332 (received either no or irregular response from module). Entering normal operation mode. | ");
          }
        }
        else
        {
          Serial.print("Error 2331 (received either no or irregular response from module). Entering normal operation mode. | ");
        }
        Serial2.write('X');
      }

      // [234x] Reset
      else if(inByte == 'r' || inByte == 'R')
      {
        digitalWrite(rstpin, LOW);
        delay(50);
        digitalWrite(rstpin, HIGH);
        delay(50);
        Serial.print("Reset complete. Entering normal operation mode. | ");
      }

      // [235x] RSSI reading
      else if(inByte == 's' || inByte == 'S')
      {
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        //Check for feedback from module
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          Serial2.print('S');
          serial2_wait(5000);
          if(Serial2.available() != 0)
          {
            inByte = Serial2.read();
            Serial.print("RSSI-Reading:");
            Serial.write(inByte);
            Serial.print(" Signal Strength:");
            Serial.print(-0.5 * (float)inByte, DEC);
            Serial.print(" RSSI-Reading finished. Entering normal operation mode. | ");
          }
          else
          {
            Serial.print("\nError 2352 (received no response from module). Entering normal operation mode. | ");
          }
        }
        else
        {
            Serial.print("Error 2351 (received either no or irregular response from module). Entering normal operation mode. | ");
        }
        Serial2.write('X');
      }

      // [236x] Temperature reading
      else if(inByte == 'u' || inByte == 'U')
      {
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        //Check for feedback from module
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          Serial2.print('U');
          serial2_wait(5000);
          if(Serial2.available() != 0)
          {
            inByte = Serial2.read();
            Serial.print("TEMP-Reading:");
            Serial.write(inByte);
            Serial.print(" Temperature:");
            Serial.write(inByte - 128);
            while(Serial2.available())
            {
              Serial2.read();
            }
            Serial.print(" Temperature-Reading finished. Entering normal operation mode. | ");
          }
          else
          {
            Serial.print("Error 2362 (received no response from module). Entering normal operation mode. | ");
          }
        }
        else
        {
            Serial.print("Error 2361 (received either no or irregular response from module). Entering normal operation mode. | ");
        }
        Serial2.write('X');
      }

      // [237x] Voltage reading
      else if(inByte == 'v' || inByte == 'V')
      {
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        //Check for feedback from module
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          Serial2.print('V');
          serial2_wait(5000);
          if(Serial2.available() != 0)
          {
            inByte = Serial2.read();
            Serial.print("VCC-Reading:");
            Serial.print(inByte);
            Serial.print(" Voltage:");
            Serial.print((double)inByte * 0.03, DEC);
            while(Serial2.available())
            {
              Serial2.read();
            }
            Serial.print(" Voltage-Reading finished. Entering normal operation mode. | ");
          }
          else
          {
            Serial.print("Error 2372 (received no response from module). Entering normal operation mode. | ");
          }
        }
        else
        {
            Serial.print("Error 2371 (received either no or irregular response from module). Entering normal operation mode. | ");
        }
        Serial2.write('X');
      }

      // [238x] Sleep mode
      else if(inByte == 's' || inByte == 'S')
      {
        digitalWrite(rtspin, LOW);
        Serial.print("Entered sleep mode. Press any character to exit. ");
        while(!Serial.available());
        Serial.read();
        digitalWrite(rtspin, HIGH);
        Serial.print("Exited sleep mode. Entering normal operation mode. | ");
      }

      // [239x] Non-volatile memory reading
      else if(inByte == '0')
      {
        digitalWrite(cfgpin, LOW);
        serial2_wait(5000);
        digitalWrite(cfgpin, HIGH);
        //Check for feedback from module
        Serial.write(Serial2.available());
        if(Serial2.available() == 1 && Serial2.read() == '>')
        {
          Serial2.write('0');
          serial2_wait(5000);
          if(Serial2.available() != 0)
          {
            char inBuffer[129] = {0};
            for(int i = 0; serial2_wait(5000) != 0; i++)
            {
              inBuffer[i] = Serial2.read();
            }
            Serial.print(" (0x00) RF Channel [4,0x04]:");
            Serial.write(inBuffer[0x00]);
            Serial.print(" (0x01) RF Power [5,0x05]:");
            Serial.write(inBuffer[0x01]);
            Serial.print(" (0x02) RF Data rate [3,0x03]:");
            Serial.write(inBuffer[0x02]);
            Serial.print(" (0x04) SLEEP Mode [0,0x00]:");
            Serial.write(inBuffer[0x04]);
            Serial.print(" (0x05) RSSI Mode [0,0x00]:");
            Serial.write(inBuffer[0x05]);
            Serial.print(" (0x0E) Packet length high [0,0x00]:");
            Serial.write(inBuffer[0x0E]);
            Serial.print(" (0x0F) Packet length low [128,0x80]:");
            Serial.write(inBuffer[0x0F]);
            Serial.print(" (0x10) Packet timeout [124,0x7C]:");
            Serial.write(inBuffer[0x10]);
            Serial.print(" (0x11) Packet end character [0,0x00]:");
            Serial.write(inBuffer[0x11]);
            Serial.print(" (0x14) Address mode [2,0x02]:");
            Serial.write(inBuffer[0x14]);
            Serial.print(" (0x15) CRC mode [2,0x02]:");
            Serial.write(inBuffer[0x15]);
            Serial.print(" (0x19) Unique ID 1 [1,0x01]:");
            Serial.write(inBuffer[0x19]);
            Serial.print(" (0x1B) Unique ID 2 [1,0x01]:");
            Serial.write(inBuffer[0x1B]);
            Serial.print(" (0x1D) Unique ID 3 [1,0x01]:");
            Serial.write(inBuffer[0x1D]);
            Serial.print(" (0x1F) Unique ID 4 [1,0x01]:");
            Serial.write(inBuffer[0x1F]);
            Serial.print(" (0x1A) System ID 1 [1,0x01]:");
            Serial.write(inBuffer[0x1A]);
            Serial.print(" (0x1C) System ID 2 [1,0x01]:");
            Serial.write(inBuffer[0x1C]);
            Serial.print(" (0x1E) System ID 3 [1,0x01]:");
            Serial.write(inBuffer[0x1E]);
            Serial.print(" (0x20) System ID 4 [1,0x01]:");
            Serial.write(inBuffer[0x20]);
            Serial.print(" (0x21) Destination ID 1 [1,0x01]:");
            Serial.write(inBuffer[0x21]);
            Serial.print(" (0x22) Destination ID 2 [1,0x01]:");
            Serial.write(inBuffer[0x22]);
            Serial.print(" (0x23) Destination ID 3 [1,0x01]:");
            Serial.write(inBuffer[0x23]);
            Serial.print(" (0x24) Destination ID 4 [1,0x01]:");
            Serial.write(inBuffer[0x24]);
            Serial.print(" (0x28) Broadcast address [255,0xFF]:");
            Serial.write(inBuffer[0x28]);
            Serial.print(" (0x30) UART baud rate [5,0x05]:");
            Serial.write(inBuffer[0x30]);
            Serial.print(" (0x31) UART number of bits [8,0x08]:");
            Serial.write(inBuffer[0x31]);
            Serial.print(" (0x32) UART parity [0,0x00]:");
            Serial.write(inBuffer[0x32]);
            Serial.print(" (0x33) UART stop bits [1,0x01]:");
            Serial.write(inBuffer[0x33]);
            Serial.print(" (0x35) UART flow control [0,0x00]:");
            Serial.write(inBuffer[0x35]);
            Serial.print(" (0x3C - 0x49) Part number:");
            for(int i = 0x3C; i <= 0x49; i++)
            {
              Serial.write(inBuffer[i]);
            }
            Serial.print(" (0x4B - 0x4E) Hardware revision number:");
            for(int i = 0x4B; i <= 0x4E; i++)
            {
              Serial.write(inBuffer[i]);
            }
            Serial.print(" (0x50 - 0x53) Software revision number:");
            for(int i = 0x50; i <= 0x53; i++)
            {
              Serial.write(inBuffer[i]);
            }
            Serial.print("Finished. Returning to normal operation mode. | ");
          }
          else
          {
            Serial.print("Error 2392 (received no response from module). Entering normal operation mode. | ");
          }
        }
        else
        {
            Serial.print("Error 2391 (received either no or irregular response from module). Entering normal operation mode. | ");
        }
        Serial2.write('X');
      }

      // [230x] Wrong input
      else
      {
        Serial.print("Error 2301 (received irregular input from user). Entering normal operation mode. | ");
      }
    }

    // [20xx] Wrong input
    else
    {
      Serial.print("Error 2001 (received irregular input from user). Entering normal operation mode. | ");
    }
  }

}

//Waiting whether serial.available() == true in given time
int serial_wait(int delay_microsec)
{
  for(int i = 0; i < (delay_microsec / 10) && Serial.available() == 0; i++)
  {
    delayMicroseconds(10);
  }
  return Serial.available();
}

//Waiting whether serial2.available() == true in given time
int serial2_wait(int delay_microsec)
{
  for(int i = 0; i < (delay_microsec / 10) && Serial2.available() == 0; i++)
  {
    delayMicroseconds(10);
  }
  return Serial2.available();
}
