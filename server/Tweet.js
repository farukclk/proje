
const axios = require('axios');
const SeleniumScraper = require("./SeleniumAPI");
const config = require('./config');
const TwitterAPI = require('./TwitterAPI');

class Tweet {


	constructor( url ) {
		this.url = url;           
		this.text = null;           // tvitin içerik metni
		this.summary = null;        // 1-2 cumlelik ozet
    	this.author = null;         // yazan kişi  
		this.emotion = null;        // duygu durumu
		this.score = null;          // duygu tahmini yüdelik oranı
    	this.created_at = null;     // paylaşılma tarihi
  	}


  	// tweet bilgilerini X api si ile çeker
  	// free kullanımda 15dk da 1 istek
  	async fetchTweetWithXAPI() {
		if (!this.url) {
            throw new Error('Tweet URL\'si belirtilmemiş!');
        }
        
		const result = await TwitterAPI.scrapeTweet(this.url);

		if (result.success === true) {
			this.text = result.data.text;
			this.author = result.data.author;
			this.created_at = result.data.created_at;
		}

  	}


	// tweet bilgilerini selenium ile çeker
	// free x apisine göre 100 kat daha iyi :)
    async fetchTweetWithSelenium() {
        if (!this.url) {
            throw new Error('Tweet URL\'si belirtilmemiş!');
        }
        
        const result = await SeleniumScraper.scrapeTweet(this.url);


		if (result.success === true) {
			this.text = result.data.text;
			this.author = result.data.author;
			this.created_at = result.data.created_at;
		}

		return result.success;
    }


	// tweeti 1-2 cümle ile özetler gemini api sini kullanarak
	async summurizeTweet() {
		if (this.text !== null) {
			// gemini api url
			const API_URL = 'https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=' + config.GEMINI_TOKEN;
			const headers = {
				"Content-Type": "application/json"
			};
				
	
			const data = {
				contents: [
					{
						role: "user",
						parts: [
							{ text: `Aşağıdaki tweeti 1-2 cümle ile özetle:\n\n${this.text}` }
						]
					}
				]
			};

			
			try {
				const response = await axios.post(API_URL, data, { headers });
			
				const summary = response.data.candidates?.[0]?.content?.parts?.[0]?.text;
				this.summary = summary;
				return summary
			
			}
			catch(error) {
				console.error('Error: tewwt özetlenemedi [!]\n',  error.message);
			}
		}
		else {
			console.error("tweet metni null [!]")
		}
	}







	// olumlu, olumsuz, notr analizi
	// tweet metnini huggingfaace AI apisine gonderir ve sonucu getirir
	async sentimentAnalysis() {
		if (this.text !== null) {
			const API_URL = "https://api-inference.huggingface.co/models/savasy/bert-base-turkish-sentiment-cased";
			const headers = {
				"Authorization": "Bearer " + config.HF_API_TOKEN,
				"Content-Type": "application/json"
			};
				
			const data = {
				inputs: this.text
			};
			
			try {
				const response = await axios.post(API_URL, data, { headers });
				var result =  response.data;


				if (result) {
					
					// Sonuçları daha okunabilir şekilde göster
					const predictions = result[0];
					//console.log('Sentiment Analizi Sonucu:');
					predictions.forEach(pred => {
						const percentage = (pred.score * 100).toFixed(2);
						//console.log(`${pred.label.toUpperCase()}: %${percentage}`);
					});
					
					// En yüksek skoru bul
					const topPrediction = predictions.reduce((prev, current) => 
						(prev.score > current.score) ? prev : current
					);
					

					this.emotion = topPrediction.label.toUpperCase();
					this.score = (topPrediction.score * 100).toFixed(2);
					return this.emotion;
				}	


			} catch (error) {
				console.error('Error:', error.response?.data || error.message);
				return null;
			}
		}
		else {
			console.error("url null olamaz [!]");
		}
		return null;
	}


	// Tweet bilgilerini göster
    display() {
        console.log('\n=== TWEET BİLGİLERİ ===');
        console.log(`📝 Metin: ${this.text}`);
        console.log(`👤 Yazar: ${this.author}`);
        console.log(`🕒 Tarih: ${this.created_at}`);
        console.log(`🔗 URL: ${this.url}`);
		console.log(`✂️ Özet: ${this.summary || 'Henüz oluşturulmadı'}`);
        console.log(`😊 Duygu: ${this.emotion || 'Henüz analiz edilmedi'}`);
        console.log(`📊 Skor: ${this.score || 'Henüz analiz edilmedi'}`);
        console.log('====================\n');
    }


	toJSON() {
		return {
			url: this.url,
			text: this.text,
			summary: this.summary,
			author: this.author,
			emotion: this.emotion,
			score: this.score,
			created_at: this.created_at,	
		};
	}
}

module.exports = Tweet;
