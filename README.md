#  Tweet Analiz ve Özetleme Uygulaması

Bu proje, bir Tweet URL'si üzerinden:

1. **Tweet metnini selenium ile yada x api ile çeker**
2. **Gemini API ile özet çıkarır**
3. **Hugging Face ile duygu analizini yapar (pozitif / negatif / nötr)**
4. Sonuçları sade bir React arayüzünde gösterir

---

## Kurulum

```bash
git clone https://github.com/kullaniciadi/proje-adi.git
cd proje-adi
bash setup.sh
```


## TOKEN yapılandırması

**"server/config.js"** dosyasını açıp aşağıdaki değişkenleri kendi API anahtarlarınızla doldurun:

```js
const GEMINI_TOKEN = "YOUR_GEMINI_API_KEY";
const HF_API_TOKEN = "YOUR_HUGGING_FACE_API_KEY";
```
NOT: 
Huggingface tokenı için, token ayarlarında  ***"Make calls to Inference Providers"*** seçneğini aktif etmelisniz.

### X API
Eğer selenium yerine x api kullanıcaksanız bunu eklemelisiniz. Uygulama varsayılan olarak selenium kullanır.
```js
const X_API_TOKEN = "YOUR_X_API_KEY";
const useXAPI = true;
```


## Uygulamayı Başlat
```sh
bash run.sh 5000
```

## LOGS

Analiz logları ***server/logs.xlsx***  dosyasına kaydedilir.

