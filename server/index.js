// server/index.js
const express = require('express');
const cors = require('cors');
const path = require('path');
const Tweet = require('./Tweet');
const config = require('./config');

const ExcelSaver = require('./ExcelSaver');
const saver = new ExcelSaver('logs.xlsx');

const app = express();

app.use(express.static(path.join(__dirname, '../client/build')));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '../client/build/index.html'));
});

app.use(cors()); // React'ten gelen istekleri kabul et
app.use(express.json());


app.post('/api',async(req, res) => {
	const { url } = req.body;          // POST gövdesinden 'url' al
	console.log('Gelen URL:', url);    // Konsola yazdır
  

	try {

  		// Tweet objesi oluştur
        const tweet = new Tweet(url);          
		
		
		if (config.useXAPI) {
			//x api ile tweet bilgilerini çek
			await tweet.fetchTweetWithXAPI();    
		}
        else {
			// Selenium ile tweet bilgilerini çek
        	await tweet.fetchTweetWithSelenium();
		}
				

		await Promise.all([
			tweet.summurizeTweet(),     // gemini ile tweet metninii özetle
			tweet.sentimentAnalysis()   // haggingface ile tweeti olumlu olumsuz analizini yap
		]);


		
		await saver.saveTweet(tweet.toJSON());
		tweet.display();
        console.log('\n📋 İşlem tamamlandı!');
		res.json(tweet.toJSON());


		
        
    } catch (error) {
        console.error('❌ Hata:', error.message);
	
    }

});


const PORT = process.argv[2] || 5000;
app.listen(PORT, () => console.log(`API sunucusu http://localhost:${PORT}`));
