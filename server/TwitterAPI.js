/*

	x apisi kullanarak tweet in bilgilerini çeker
	free kullanımda 15 dk da 1 istek

*/

const axios = require('axios');
const config = require('./config');

class TwitterAPI {
    
    /**
     * Tweet URL'sinden ID ayıklar
     * @param {string} url - Tweet URL'si
     * @returns {string|null} - Tweet ID veya null
     */
    static extractTweetId(url) {
        const match = url.match(/status\/(\d+)/);
        return match ? match[1] : null;
    }
    
    
    /**
     * Tweet ID ile tweet verilerini getirir
     * @param {string} tweetId - Tweet ID
     * @returns {Promise<Object>} - Tweet verisi
     */
    static async fetchTweet(tweetId) {
        if (!config.X_API_TOKEN) {
            throw new Error('Bearer token ayarlanmamış!');
        }
        
        const url = `https://api.twitter.com/2/tweets/${tweetId}`
            + `?expansions=author_id`
            + `&tweet.fields=text,created_at,public_metrics`
            + `&user.fields=username,name,verified`;

        try {
            const response = await axios.get(url, {
                headers: {
                    'Authorization': `Bearer ${config.X_API_TOKEN}`,
                    'Content-Type': 'application/json'
                },
            });

            const tweet = response.data.data;
            const user = response.data.includes?.users?.[0];

            return {
                success: true,
         /*       data: {
                    id: tweet.id,
                    text: tweet.text,
                    createdAt: tweet.created_at,
                    metrics: tweet.public_metrics,
                    author: {
                        id: tweet.author_id,
                        username: user?.username,
                        name: user?.name,
                        verified: user?.verified
                    }
                }
                */
                data: {
                    text: tweet.text,
                    created_at: tweet.created_at,
                    author:  user?.username,
                }
                
            };
        } catch (error) {
            return {
                success: false,
                error: error.response?.data || error.message
            };
        }
    }
    
    /**
     * Tweet URL'si ile tweet verilerini getirir
     * @param {string} tweetUrl - Tweet URL'si
     * @returns {Promise<Object>} - Tweet verisi
     */
    static async scrapeTweet(tweetUrl) {
        const tweetId = this.extractTweetId(tweetUrl);
        
        if (!tweetId) {
            return {
                success: false,
                error: 'Geçersiz tweet URL\'si'
            };
        }
        
        return await this.fetchTweet(tweetId);
    }
    
}



module.exports = TwitterAPI;
