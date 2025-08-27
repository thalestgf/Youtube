
void setup() {
    Serial.begin(9600);
    Serial0.begin(9600);
}

void loop() {
    if (Serial.available()) {
        Serial0.write(Serial.read());
    }
    if (Serial0.available()) {
        Serial.write(Serial0.read());
    }
}
