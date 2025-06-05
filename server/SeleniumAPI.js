/*
	selenium kullanarak tweet bilgilerini çeker
	free x apisine gore daha hızlı


*/

const { Builder, By, until } = require('selenium-webdriver');
const chrome = require('selenium-webdriver/chrome');
const fs = require('fs');
const path = require('path');

class SeleniumScraper {
    static #driver = null;
    static #cacheDir = path.join(__dirname, 'cache');
    
    /**
     * Önbellek klasörünü oluştur
     */
    static #createCacheDir() {
        if (!fs.existsSync(this.#cacheDir)) {
            fs.mkdirSync(this.#cacheDir, { recursive: true });
            console.log('📁 Önbellek klasörü oluşturuldu:', this.#cacheDir);
        }
    }
    
    /**
     * Chrome options'ı yapılandırır (önbellek destekli)
     * @param {boolean} headless - Headless mode (default: true)
     * @returns {chrome.Options} Chrome options
     */
    static #getChromeOptions(headless = true) {
        // Önbellek klasörünü oluştur
        this.#createCacheDir();
        
        const options = new chrome.Options();
        
        // Önbellek ayarları
        const userDataDir = path.join(this.#cacheDir, 'user-data');
        const diskCacheDir = path.join(this.#cacheDir, 'disk-cache');
        
        // Klasörleri oluştur
        if (!fs.existsSync(userDataDir)) {
            fs.mkdirSync(userDataDir, { recursive: true });
        }
        if (!fs.existsSync(diskCacheDir)) {
            fs.mkdirSync(diskCacheDir, { recursive: true });
        }
        
        // Önbellek seçenekleri
        options.addArguments(`--user-data-dir=${userDataDir}`);
        options.addArguments(`--disk-cache-dir=${diskCacheDir}`);
        options.addArguments('--disk-cache-size=104857600'); // 100MB cache
        options.addArguments('--media-cache-size=52428800'); // 50MB media cache
        options.addArguments('--aggressive-cache-discard=false');
        
        // Temel seçenekler
        options.addArguments('--disable-blink-features=AutomationControlled');
        options.addArguments('--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36');
        options.addArguments('--no-sandbox');
        options.addArguments('--disable-dev-shm-usage');
        options.addArguments('--disable-extensions');
        options.addArguments('--disable-plugins');
        
        // Performans optimizasyonları
        options.addArguments('--disable-web-security');
        options.addArguments('--allow-running-insecure-content');
        options.addArguments('--disable-features=TranslateUI');
        options.addArguments('--disable-ipc-flooding-protection');
        
        // Önbellek politikası
        options.addArguments('--enable-features=NetworkService');
        options.addArguments('--force-fieldtrials=NetworkService/Enabled');
        
        if (headless) {
            options.addArguments('--headless');
            options.addArguments('--disable-gpu');
            options.addArguments('--window-size=1920,1080');
        } else {
            options.addArguments('--start-maximized');
        }
        
        // Prefs ayarları (önbellek için)
        const prefs = {
            'profile.default_content_setting_values': {
                'notifications': 2,
                'media_stream_mic': 2,
                'media_stream_camera': 2
            },
            'profile.default_content_settings': {
                'popups': 0
            },
            'profile.managed_default_content_settings': {
                'images': 1
            }
        };
        
        options.setUserPreferences(prefs);
        
        return options;
    }
    
    /**
     * WebDriver'ı başlatır
     * @param {boolean} headless - Headless mode
     * @param {boolean} useCache - Önbellek kullanılsın mı
     */
    static async #initDriver(headless = true, useCache = true) {
        if (this.#driver) {
            await this.#driver.quit();
        }
        
        const options = useCache ? 
            this.#getChromeOptions(headless) : 
            this.#getBasicChromeOptions(headless);
            
        this.#driver = await new Builder()
            .forBrowser('chrome')
            .setChromeOptions(options)
            .build();
            
        if (useCache) {
            // Önbellek ayarlarını driver seviyesinde aktifleştir
            await this.#driver.executeScript(`
                // Service Worker için önbellek stratejisi
                if ('serviceWorker' in navigator) {
                    navigator.serviceWorker.register('/sw.js').catch(() => {});
                }
                
                // Local Storage temizleme (isteğe bağlı)
                // localStorage.clear();
            `);
        }
    }
    
    /**
     * Temel Chrome options (önbellek olmadan)
     */
    static #getBasicChromeOptions(headless = true) {
        const options = new chrome.Options();
        
        options.addArguments('--disable-blink-features=AutomationControlled');
        options.addArguments('--user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36');
        options.addArguments('--no-sandbox');
        options.addArguments('--disable-dev-shm-usage');
        options.addArguments('--disable-extensions');
        options.addArguments('--disable-plugins');
        options.addArguments('--disable-images');
        
        if (headless) {
            options.addArguments('--headless');
            options.addArguments('--disable-gpu');
            options.addArguments('--window-size=1920,1080');
        }
        
        return options;
    }
    
    /**
     * WebDriver'ı kapatır
     */
    static async #closeDriver() {
        if (this.#driver) {
            await this.#driver.quit();
            this.#driver = null;
        }
    }
    
    /**
     * Önbellek boyutunu kontrol eder
     * @returns {Object} Önbellek istatistikleri
     */
    static getCacheStats() {
        try {
            const stats = {
                cacheExists: fs.existsSync(this.#cacheDir),
                userDataSize: 0,
                diskCacheSize: 0,
                totalSize: 0
            };
            
            if (stats.cacheExists) {
                const userDataDir = path.join(this.#cacheDir, 'user-data');
                const diskCacheDir = path.join(this.#cacheDir, 'disk-cache');
                
                if (fs.existsSync(userDataDir)) {
                    stats.userDataSize = this.#getFolderSize(userDataDir);
                }
                
                if (fs.existsSync(diskCacheDir)) {
                    stats.diskCacheSize = this.#getFolderSize(diskCacheDir);
                }
                
                stats.totalSize = stats.userDataSize + stats.diskCacheSize;
            }
            
            return stats;
        } catch (error) {
            console.error('Önbellek istatistikleri alınırken hata:', error);
            return null;
        }
    }
    
    /**
     * Klasör boyutunu hesaplar (MB cinsinden)
     */
    static #getFolderSize(folderPath) {
        let totalSize = 0;
        
        try {
            const files = fs.readdirSync(folderPath);
            
            for (const file of files) {
                const filePath = path.join(folderPath, file);
                const stats = fs.statSync(filePath);
                
                if (stats.isDirectory()) {
                    totalSize += this.#getFolderSize(filePath);
                } else {
                    totalSize += stats.size;
                }
            }
        } catch (error) {
            // Hata durumunda 0 döndür
        }
        
        return Math.round((totalSize / 1024 / 1024) * 100) / 100; // MB cinsinden
    }
    
    /**
     * Önbelleği temizler
     */
    static clearCache() {
        try {
            if (fs.existsSync(this.#cacheDir)) {
                fs.rmSync(this.#cacheDir, { recursive: true, force: true });
                console.log('🗑️ Önbellek temizlendi');
                return true;
            }
            return false;
        } catch (error) {
            console.error('Önbellek temizlenirken hata:', error);
            return false;
        }
    }
    
    /**
     * Tweet URL'sinden tweet ID'sini ayıklar
     */
    static extractTweetId(url) {
        const match = url.match(/status\/(\d+)/);
        return match ? match[1] : null;
    }
    
    /**
     * Tweet metnini scrape eder
     */
    static async #scrapeTweetText() {
        try {
            const textElement = await this.#driver.wait(
                until.elementLocated(By.css('[data-testid="tweetText"]')), 
                10000
            );
            return await textElement.getText();
        } catch (error) {
            try {
                const altTextElement = await this.#driver.findElement(By.css('div[lang] span'));
                return await altTextElement.getText();
            } catch (altError) {
                return 'Tweet metni bulunamadı';
            }
        }
    }
    
    /**
     * Tweet yazarını scrape eder
     */
    static async #scrapeAuthor() {
        try {
            const authorElement = await this.#driver.wait(
                until.elementLocated(By.css('[data-testid="User-Name"] span')), 
                5000
            );
            return await authorElement.getText();
        } catch (error) {
            try {
                const altAuthorElement = await this.#driver.findElement(By.css('div[data-testid="User-Name"] a span'));
                return await altAuthorElement.getText();
            } catch (altError) {
                return 'Yazar bulunamadı';
            }
        }
    }
    
    /**
     * Tweet zamanını scrape eder
     */
    static async #scrapeTime() {
        try {
            const timeElement = await this.#driver.wait(
                until.elementLocated(By.css('time')), 
                5000
            );
            const datetime = await timeElement.getAttribute('datetime');
            
            if (datetime) {
                const date = new Date(datetime);
                return date.toLocaleString('tr-TR');
            }
            return 'Zaman bulunamadı';
        } catch (error) {
            return 'Zaman bulunamadı';
        }
    }
    
    /**
     * Tweet'i scrape eder (önbellek destekli)
     * @param {string} tweetUrl - Tweet URL'si
     * @param {boolean} headless - Headless mode (default: true)
     * @param {boolean} useCache - Önbellek kullanılsın mı (default: true)
     * @returns {Promise<Object>} Tweet verisi objesi
     */
    static async scrapeTweet(tweetUrl, headless = true, useCache = true) {
        try {
            console.log('🚀 Önbellekli Twitter Scraper başlatılıyor...');
            console.log('📍 URL:', tweetUrl);
            console.log('💾 Önbellek:', useCache ? 'Aktif' : 'Pasif');
            
            if (useCache) {
                const cacheStats = this.getCacheStats();
                if (cacheStats && cacheStats.cacheExists) {
                    console.log(`📊 Mevcut önbellek boyutu: ${cacheStats.totalSize} MB`);
                }
            }
            
            // Driver'ı başlat
            await this.#initDriver(headless, useCache);
            
            // Tweet sayfasını aç
            console.log('📄 Tweet sayfası açılıyor...');
            await this.#driver.get(tweetUrl);
            
            // Sayfanın yüklenmesini bekle
            await this.#driver.sleep(useCache ? 2000 : 3000); // Önbellekli daha hızlı
            
            // Tweet verilerini scrape et
            console.log('📊 Tweet verileri alınıyor...');
            const [text, author, createdTime] = await Promise.all([
                this.#scrapeTweetText(),
                this.#scrapeAuthor(),
                this.#scrapeTime()
            ]);
            
            const tweetData = {
                success: true,
                data: {
                    id: this.extractTweetId(tweetUrl),
                    text: text,
                    author: author,
                    created_at: createdTime,
                    url: tweetUrl,
                    cached: useCache
                }
            };
            
            console.log('✅ Tweet başarıyla alındı!');
            
            if (useCache) {
                const finalCacheStats = this.getCacheStats();
                if (finalCacheStats) {
                    console.log(`💾 Yeni önbellek boyutu: ${finalCacheStats.totalSize} MB`);
                }
            }
            
            return tweetData;
            
        } catch (error) {
            console.error('❌ Hata oluştu:', error.message);
            return {
                success: false,
                error: error.message,
                url: tweetUrl
            };
        } finally {
            await this.#closeDriver();
        }
    }
}

// Export
module.exports = SeleniumScraper;