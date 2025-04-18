"""


email atmak için test komutlari:

curl -X POST  https://127.0.0.1:8443/rapor \
     -H "Content-Type: application/json" \
     -d '{"token": "SECRET_TOKEN", "message": "Selam!"}' \
     --insecure
     

     
curl -X POST  https://vahsikelebekler.pythonanywhere.com/rapor \
     -H "Content-Type: application/json" \
     -d '{"token": "SECRET_TOKEN", "message": "Selam!"}'



"""


from flask import Flask, request, jsonify
import smtplib
from email.mime.text import MIMEText

from config import EMAIL, PASSWORD, TOKEN


app = Flask(__name__)


# SMTP ayarları
SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 465



# maillerin gonderileceği adreslerin listesi
email_list = [
	"farukcelikclk@gmail.com",
	
]


@app.route("/")
def test():
	return "hi"


# gunluk rapor gonderme şeysi
@app.route("/rapor", methods=["POST"])
def send_email2():
	data = request.get_json()
	if not data or "message" not in data or "token" not in data:
		return jsonify({"error": "message gerekli"}), 400
	
	if (TOKEN !=  data["token"]):
		return ""

	try:		
		send_email("günlük rapor", data["message"])

		return jsonify({"status": "email gönderildi"}), 200
	except Exception as e:
		return jsonify({"error": str(e)}), 500




def send_email(subject, message):

	body = message
	msg = MIMEText(body)
	msg["Subject"] = subject
	msg["From"] = EMAIL
	msg["To"] = ", ".join(email_list)  # SMTP header için

	with smtplib.SMTP_SSL(SMTP_SERVER, SMTP_PORT) as server:
		server.login(EMAIL, PASSWORD)	
		server.send_message(msg)
		


if __name__ == "__main__":

	app.run(
		ssl_context=('cert.pem', 'key.pem'),
		host='0.0.0.0',
        port=8443,   # 443 için sudo gerekebilir, o yüzden 8443 kullanıyoruz
        debug=True
    )

