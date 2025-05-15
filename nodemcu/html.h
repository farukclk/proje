
const char html[] PROGMEM = R"EOF(

<!DOCTYPE html>
<html lang="tr">
<head>
  <meta charset="UTF-8">
  <title>Kullanıcı Düzenleme Paneli</title>
  <style>
body {
	font-family: Arial, sans-serif;
	max-width: 800px;
	margin: 0 auto;
	padding: 20px;
	background-color: #000;
	color: #3ff59a;
	position: relative;
  }
  
table {
	width: 100%;
	border-collapse: collapse;
	margin-top: 20px;
  }
  
  th, td {
	padding: 10px;
	text-align: left;
  }
  
  tr {
	border-bottom: 1px solid #555;
  }
  
  .green {
	color: #00e676;
	font-weight: bold;
  }
  
  .red {
	color: #ff1744;
	font-weight: bold;
  }
  
  input[type="checkbox"] {
	margin-right: 5px;
	transform: scale(1.2);
  }
  
  label {
	cursor: pointer;
	user-select: none;
  }
  

  .charging {
	color: #76ff03;
  }
  
  .battery {
	color: #ffeb3b;
  }
  
  .unknown {
	color: #888;
  }
  

  #addUserForm {
	margin: 20px 0;
  }
  #addUserForm input {
	padding: 5px;
	font-size: 16px;
  }
  #addUserForm button {
	padding: 5px 10px;
	font-size: 16px;
	cursor: pointer;
  }
  </style>
</head>
<body>
  <h1>Kullanıcı Özellikleri</h1>

<div style="position: absolute; top: 20px; right: 20px; display: flex; align-items: center; gap: 6px;">
  <span id="batteryStatus" class="unknown" title="Şarj Durumu" style="font-size: 22px; line-height: 1;">❔</span>
  <button onclick="fetchUsers()" title="Sayfayı Yenile" style="
    background: none;
    border: none;
    padding: 0;
    margin: 0;
    font-size: 22px;
    line-height: 1;
    cursor: pointer;
    color: #3ff59a;
  ">🔄</button>
</div>

  <!-- İsim ekleme alanı -->
  <div id="addUserForm">
    <form id="addUserForm" onsubmit="event.preventDefault(); addName();">
      <input type="text" id="nameInput" placeholder="İsim girin">
      <button type="submit">Yeni Kayıt Ekle</button>
    </form>

  </div>

  <table id="userTable">
    <thead>
      <tr>
        <th>ID</th>
        <th>İsim</th>
        <th>İçeride mi?</th>
        <th>Admin</th>
        <th>Kapıyı Açabilir mi?</th>
        <th>Sil</th>
      </tr>
    </thead>
    <tbody></tbody>
  </table>

  <script>
let url = "";


window.onload = () => {
 
  url = prompt("bağlantı urlsi:");
  //url = "http://10.42.0.242";
 
  console.log(url);
  let users = [];
  fetchUsers();
  checkBatteryStatusLoop(); ;
  
};




// Yeni isim ekle
function addName() {
  const input = document.getElementById("nameInput");
  const name = input.value.trim();

  if (name === "") {
    alert("Lütfen bir isim giriniz!");
    return;
  }

  fetch(`${url}/kayit?name=${encodeURIComponent(name)}`, {
    method: "GET",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded"
    }
  })
  .then(res => res.text())
  .then(data => {
    console.log("Cevap:", data);
    input.value = "";
    // Opsiyonel: tabloyu güncelle
    fetchUsers();
  });
}



// İsim sil
function deleteName(index) {
  if (confirm("Bu kaydı silmek istediğinize emin misiniz?")) {
    const userId = users[index][0]; // gerçek ID (veri kaynağından gelen)

    fetch(`${url}/delete?id=${encodeURIComponent(userId)}`, {
      method: "GET",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      }
    })
    .then(res => res.text())
    .then(data => {
      console.log("Cevap:", data);
      if (data.trim().toUpperCase() === "OK") {
        users.splice(index, 1); // diziden sil
        updateTable();          // tabloyu güncelle
      } else {
        alert("Silme işlemi başarısız: " + data);
      }
    })
    .catch(err => {
      alert("Sunucu hatası: " + err);
    });
  }
}



function fetchUsers() {
  fetch(url + "/users")
    .then(res => res.text())
    .then(data => {
      users = JSON.parse(data);
      console.log(users);
      updateTable();
    })
    .catch(() => {
      users = [
        // id, name, is_admin, can_open_door, is_in : 1
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [4, "Zeynep", 0, 0, 1],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
        [1, "Ali", 1, 1, 1],
        [2, "Ayşe", 0, 1, 0],
        [3, "Mehmet", 1, 0, 0],
      ];
      updateTable();
    });
}


function updateTable() {
  const tbody = document.querySelector("#userTable tbody");
  tbody.innerHTML = "";

  users.forEach((user, id) => {
    const row = tbody.insertRow();

    row.insertCell().textContent = id;
    row.insertCell().textContent = user[1];

    const isInCell = row.insertCell();
    isInCell.textContent = user[4] ? "Evet" : "Hayır";
    isInCell.className = user[4] ? "green" : "red";

    const adminCell = row.insertCell();
    adminCell.innerHTML = `
      <label>
        <input type="checkbox" ${user[2] ? "checked" : ""} onchange="toggleUser(${id}, 'admin', this.checked)">
        Admin
      </label>
    `;

    const doorCell = row.insertCell();
    doorCell.innerHTML = `
      <label>
        <input type="checkbox" ${user[3] ? "checked" : ""} onchange="toggleUser(${id}, 'door', this.checked)">
        Kapıyı Açabilir
      </label>
    `;

    // Sil butonu hücresi
    const actionCell = row.insertCell();
        //  actionCell.className = "action-cell";
    
          const deleteBtn = document.createElement("button");
          deleteBtn.textContent = "Sil";
          deleteBtn.className = "delete";
          deleteBtn.onclick = function() {
            deleteName(id);
          };
    
          actionCell.appendChild(deleteBtn);

  });
}

function toggleUser(id, type, value) {
  if (type === 'admin') users[id][2] = value ? 1 : 0;
  if (type === 'door')  users[id][3] = value ? 1 : 0;

  const user = users[id];
  fetch(`${url}/updateUser?id=${id}&&is_admin=${user[2]}&can_open_door=${user[3]}`, {
    method: "GET"
  }).then(res => res.text())
    .then(data => console.log("Sunucu cevabı:", data))
    .catch(err => console.warn("Sunucuya gönderilemedi:", err));
}

function checkBatteryStatusLoop() {
  fetch(`${url}/sarj`)
    .then(res => res.text())
    .then(data => {
      const status = document.getElementById("batteryStatus");
      const clean = data.trim().toUpperCase();

      if (clean === "YES") {
        status.textContent = "🔌";
        status.className = "charging";
        status.title = "Şarjda";
      } else if (clean === "NO") {
        status.textContent = "🔋";
        status.className = "battery";
        status.title = "Pille Çalışıyor";
      } else {
        status.textContent = "❔";
        status.className = "unknown";
        status.title = "Bilinmiyor";
      }
    })
    .catch(() => {
      const status = document.getElementById("batteryStatus");
      status.textContent = "❔";
      status.className = "unknown";
      status.title = "Sunucuya ulaşılamıyor";
    })
    .finally(() => {
      // ✅ Her şey bittikten 4 saniye sonra tekrar çağır
      setTimeout(checkBatteryStatusLoop, 4000);
    });
}


  </script>
</body>
</html>

)EOF";