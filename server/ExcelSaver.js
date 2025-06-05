/*
	toplanan tweet verilerini xlsx dosyasına kaydeder
*/

const ExcelJS = require('exceljs');
const fs = require('fs');
const path = require('path');

/**
 * Tweet verilerini Excel dosyasına kaydeder
 */
class ExcelSaver {
    constructor(filePath = 'logs.xlsx') {
        this.filePath = filePath;
    }

    /**
     * Tweet verisini Excel dosyasına ekler
     * @param {Object} tweetData - Tweet verisi
     * @param {string} tweetData.url - Tweet URL'si
     * @param {string} tweetData.text - Tweet metni
     * @param {string} tweetData.summary - Tweet özeti
     * @param {string} tweetData.author - Tweet yazarı
     * @param {string} tweetData.emotion - Duygu durumu
     * @param {number} tweetData.score - Duygu skoru (0-100)
     * @param {string} tweetData.created_at - Oluşturulma tarihi
     */
    async saveTweet(tweetData) {
        try {
            let workbook = new ExcelJS.Workbook();
            let worksheet;

            // Dosya var mı kontrol et
            if (fs.existsSync(this.filePath)) {
                // Varsa aç
                await workbook.xlsx.readFile(this.filePath);
                worksheet = workbook.getWorksheet('Tweets');
            } else {
                // Yoksa yeni oluştur
                worksheet = workbook.addWorksheet('Tweets');
                this.createHeaders(worksheet);
            }

            // Tweet ID'sini çıkar
            const tweetId = this.extractTweetId(tweetData.url);
            
            // Aynı tweet var mı kontrol et
            const existingRow = this.findExistingTweet(worksheet, tweetId);
            
            if (existingRow) {
                // Varsa güncelle
                this.updateRow(worksheet, existingRow, tweetData, tweetId);
                console.log(`✅ Tweet güncellendi: ${tweetId}`);
            } else {
                // Yoksa yeni satır ekle
                this.addNewRow(worksheet, tweetData, tweetId);
                console.log(`➕ Yeni tweet eklendi: ${tweetId}`);
            }

            // Dosyayı kaydet
            await workbook.xlsx.writeFile(this.filePath);
            console.log(`💾 Excel dosyası kaydedildi: ${this.filePath}`);

            return true;
        } catch (error) {
            console.error('❌ Excel kaydetme hatası:', error.message);
            return false;
        }
    }

    /**
     * Excel başlıklarını oluşturur
     */
    createHeaders(worksheet) {
        const headers = [
            'Tweet ID',
            'URL',
            'Metin',
            'Özet',
            'Yazar',
            'Duygu',
            'Skor (%)',
            'Paylaşım Tarihi',
            'Kayıt Tarihi'
        ];

        // Başlık satırını ekle
        worksheet.addRow(headers);

        // Başlık stilini ayarla
        const headerRow = worksheet.getRow(1);
        headerRow.font = { bold: true, color: { argb: 'FFFFFF' } };
        headerRow.fill = {
            type: 'pattern',
            pattern: 'solid',
            fgColor: { argb: '366092' }
        };

        // Sütun genişliklerini ayarla
        worksheet.columns = [
            { width: 20 }, // Tweet ID
            { width: 50 }, // URL
            { width: 80 }, // Metin
            { width: 60 }, // Özet
            { width: 25 }, // Yazar
            { width: 15 }, // Duygu
            { width: 12 }, // Skor
            { width: 20 }, // Paylaşım Tarihi
            { width: 20 }  // Kayıt Tarihi
        ];
    }

    /**
     * Tweet ID'sini URL'den ayıklar
     */
    extractTweetId(url) {
        const match = url.match(/status\/(\d+)/);
        return match ? match[1] : null;
    }

    /**
     * Mevcut tweet'i bulur
     */
    findExistingTweet(worksheet, tweetId) {
        if (!tweetId) return null;

        let existingRow = null;
        worksheet.eachRow((row, rowNumber) => {
            if (rowNumber === 1) return; // Başlık satırını atla
            
            const cellValue = row.getCell(1).value;
            if (cellValue && cellValue.toString() === tweetId) {
                existingRow = rowNumber;
                return false; // Loop'u durdur
            }
        });

        return existingRow;
    }

    /**
     * Mevcut satırı günceller
     */
    updateRow(worksheet, rowNumber, tweetData, tweetId) {
        const row = worksheet.getRow(rowNumber);
        
        row.getCell(1).value = tweetId;
        row.getCell(2).value = tweetData.url;
        row.getCell(3).value = tweetData.text;
        row.getCell(4).value = tweetData.summary;
        row.getCell(5).value = tweetData.author;
        row.getCell(6).value = tweetData.emotion;
        row.getCell(7).value = tweetData.score;
        row.getCell(8).value = tweetData.created_at;
        row.getCell(9).value = new Date().toISOString();

        // Güncellenen satırı vurgula
        row.fill = {
            type: 'pattern',
            pattern: 'solid',
            fgColor: { argb: 'FFF2CC' }
        };
    }

    /**
     * Yeni satır ekler
     */
    addNewRow(worksheet, tweetData, tweetId) {
        const newRow = worksheet.addRow([
            tweetId,
            tweetData.url,
            tweetData.text,
            tweetData.summary,
            tweetData.author,
            tweetData.emotion,
            tweetData.score,
            tweetData.created_at,
            new Date().toISOString()
        ]);

        // Duygu durumuna göre renklendirme
        this.colorRowByEmotion(newRow, tweetData.emotion);
    }

    /**
     * Duygu durumuna göre satırı renklendirir
     */
    colorRowByEmotion(row, emotion) {
        let color = 'FFFFFF'; // Varsayılan beyaz

        switch (emotion?.toLowerCase()) {
            case 'positive':
                color = 'E8F5E8'; // Açık yeşil
                break;
            case 'negative':
                color = 'FFEBEE'; // Açık kırmızı
                break;
            case 'neutral':
                color = 'F5F5F5'; // Açık gri
                break;
        }

        row.fill = {
            type: 'pattern',
            pattern: 'solid',
            fgColor: { argb: color }
        };
    }
}

// Export
module.exports = ExcelSaver;

// Kullanım örneği
/*
const saver = new TweetExcelSaver('tweets.xlsx');

const tweetData = {
    url: 'https://twitter.com/user/status/123456789',
    text: 'Bu bir örnek tweet metnidir...',
    summary: 'Örnek tweet özeti.',
    author: 'kullanici_adi',
    emotion: 'positive',
    score: 85,
    created_at: '2024-01-15T10:30:00Z'
};

await saver.saveTweet(tweetData);
*/