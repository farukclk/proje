

const char* ssid = "";
const char* password = "";

// cert.pem içeriğini buraya yapıştır:
const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

// key.pem içeriğini buraya yapıştır:
const char serverKey[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
-----END PRIVATE KEY-----
)EOF";
