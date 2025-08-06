#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#define OLED_RESET 4
Adafruit_SH1106 display(OLED_RESET);

#define ECG_PIN A0

int ecgData[64];
unsigned long lastBeatTime = 0;
int beatTimes[10];
int beatIndex = 0;
bool peakDetected = false;
int bpm = 0;
int lastBPM = 0;

void setup() {
    Serial.begin(115200);
    display.begin(SH1106_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
}

void loop() {
    int sumECG = 0;

    // **อ่านค่า ECG 20 ครั้งแล้วเฉลี่ยเพื่อลด Noise**
    for (int i = 0; i < 20; i++) {
        sumECG += analogRead(ECG_PIN);
        delay(1);
    }
    int ecgValue = sumECG / 20;

    // **อัปเดต BPM**
    int newBPM = calculateBPM(ecgValue);
    
    if (newBPM > 40 && newBPM < 180) {  
        bpm = newBPM;
        lastBPM = bpm;
    } else {
        bpm = lastBPM;
    }

    if (ecgValue > 300 && ecgValue < 900) {  
        Serial.println(ecgValue);

        for (int i = 0; i < 63; i++) {
            ecgData[i] = ecgData[i + 1];
        }

        int mappedECG = map(constrain(ecgValue, 300, 900), 300, 900, 5, 58);
        ecgData[63] = mappedECG;

        display.fillRect(0, 10, 128, 54, BLACK);

        // **แสดงข้อความ ECG Monitoring ❤️**
        display.setCursor(0, 5);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.print("ECG Monitoring ❤️");

        // **แสดงค่า BPM ในบรรทัดเดียวกัน**
        display.fillRect(80, 0, 50, 16, BLACK); // เคลียร์เฉพาะ BPM
        display.setCursor(150, 5);
        display.setTextSize(2);
        display.print("BPM:");
        display.print(bpm);  // ติดกับ BPM:

        // **วาดกราฟ ECG กว้างขึ้น**
        for (int i = 0; i < 62; i++) {
            display.drawLine(i * 2, 64 - ecgData[i], (i + 1) * 2, 64 - ecgData[i + 1], WHITE);
        }

        display.display();
    }

    delay(10);
}

// **ฟังก์ชันคำนวณ BPM**
int calculateBPM(int ecgValue) {
    static int lastPeak = 0;
    int threshold = 400;

    if (ecgValue > threshold && !peakDetected) {
        peakDetected = true;
        unsigned long currentTime = millis();
        if (lastBeatTime > 0) {
            int beatInterval = currentTime - lastBeatTime;
            beatTimes[beatIndex] = beatInterval;
            beatIndex = (beatIndex + 1) % 10;

            int sum = 0, validCount = 0;
            for (int i = 0; i < 10; i++) {
                if (beatTimes[i] > 300 && beatTimes[i] < 1500) {
                    sum += beatTimes[i];
                    validCount++;
                }
            }

            int avgInterval = validCount > 0 ? sum / validCount : 600;
            lastBeatTime = currentTime;
            return 60000 / avgInterval;
        }
        lastBeatTime = currentTime;
    }

    if (ecgValue < threshold - 50) {
        peakDetected = false;
    }

    return lastBPM;
}
