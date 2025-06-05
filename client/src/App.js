import React, { useState } from 'react';
import './App.css';




function App() {
  const [url, setUrl] = useState('https://x.com/ferhundere/status/1929805780590477722');
  const [loading, setLoading] = useState(false);
  const [result, setResult] = useState(null); // Başlangıçta null, obje bekliyoruz

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    setResult(null);

    try {
      const res = await fetch('/api', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url })
      });

      const data = await res.json();
      console.log(data);
      setResult(data);  
    } catch (err) {
      console.error(err);
      setResult({ error: 'Hata oluştu.' });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="App">
      <header className="App-header">
        <form onSubmit={handleSubmit} style={{ display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
          <input
            type="text"
            placeholder="Tweet URL'sini girin"
            value={url}
            onChange={(e) => setUrl(e.target.value)}
            style={{ padding: '10px', width: '300px', fontSize: '16px' }}
          />
          <button type="submit" style={{ marginTop: '10px', padding: '8px 16px' }}>
            Gönder
          </button>
        </form>

        {loading && <p style={{ marginTop: '20px' }}>Yükleniyor...</p>}

        {result && !result.error && (
          <div style={{ marginTop: '20px', textAlign: 'left', width: '600px' }}>
           
            <p style={{ wordBreak: 'break-word', whiteSpace: 'normal' }}>
              <strong>Metin:</strong> {result.text}
            </p>

            <p style={{ wordBreak: 'break-word', whiteSpace: 'normal' }}>
              <strong>Özet:</strong> {result.summary}
            </p>

       
            <p><strong>Duygu:</strong> {result.emotion}</p>
            <p><strong>Yazar:</strong> @{result.author}</p>
            <p><strong>Tarih:</strong> {result.created_at}</p>
          
          </div>

        )}

        {result && result.error && <p style={{ marginTop: '20px', color: 'red' }}>{result.error}</p>}
      </header>
    </div>
  );
}


export default App;