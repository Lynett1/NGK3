// client.js

const API_BASE_URL = 'http://localhost:8080'; 

// Viser tabel
let weatherTable;
function initializeTable() {
    if (!weatherTable) {
        weatherTable = new Tabulator("#weatherDataTable", {
            layout: "fitDataFill",
            height: "auto",
            columns: [
                { title: "ID", field: "ID" },
                { title: "Dato", field: "Tidspunkt (dato og klokkeslæt).Dato" },
                { title: "Klokkeslæt", field: "Tidspunkt (dato og klokkeslæt).Klokkeslæt" },
                { title: "Sted", field: "Sted.Navn" },
                { title: "Lat", field: "Sted.Lat" },
                { title: "Lon", field: "Sted.Lon" },
                { title: "Temperatur", field: "Temperatur" },
                { title: "Luftfugtighed", field: "Luftfugtighed" },
            ],
        });
    }
}


document.addEventListener('DOMContentLoaded', initializeTable);


// GET all
async function getAllWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/weather`);
        weatherTable.setData(response.data);
    } catch (error) {
        alert('Kunne ikke hente alle vejrdata.');
        weatherTable.setData([]);
    }
}
// GET ID
async function getWeatherDataById() {
    const id = document.getElementById('get_id').value;
    if (!id) {
        alert('Indtast ID.');
        return;
    }
    try {
        const response = await axios.get(`<span class="math-inline">\{API\_BASE\_URL\}/weather/</span>{id}`);
        weatherTable.setData([response.data]);
    } catch (error) {
        alert('Kunne ikke finde vejrdata for det angivne ID.');
        weatherTable.setData([]);
    }
}
// GET dato
async function getWeatherDataByDate() {
    const date = document.getElementById('get_date').value;
    if (!date) {
        alert('Indtast venligst en dato (YYYYMMDD).');
        return;
    }
    try {
        const response = await axios.get(`<span class="math-inline">\{API\_BASE\_URL\}/weather/date/</span>{date}`);
        weatherTable.setData(response.data); 
    } catch (error) {
        alert('Kunne ikke finde vejrdata for den angivne dato.');
        weatherTable.setData([]);
    }
}
// GET Tre seneste
async function getLatestThreeWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/latest_three`);
        weatherTable.setData(response.data); 
    } catch (error) {
        alert('Kunne ikke hente de 3 seneste vejrdata.');
        weatherTable.setData([]);
    }
}


// HTTP POST funktionalitet

async function postWeatherData() {
    const data = {
        "Dato": document.getElementById('post_date').value,
        "Klokkeslæt": document.getElementById('post_time').value,
        "Navn": document.getElementById('post_place_name').value,
        "Lat": parseFloat(document.getElementById('post_lat').value),
        "Lon": parseFloat(document.getElementById('post_lon').value),
        "Temperatur": parseFloat(document.getElementById('post_temperature').value),
        "Luftfugtighed": parseInt(document.getElementById('post_humidity').value)
    };

    try {
        const response = await axios.post(`${API_BASE_URL}/weather`, data);
        alert('Vejrdata oprettet med ID: ' + response.data.ID);
        getAllWeatherData();
    } catch (error) {
        alert('Kunne ikke oprette vejrdata. Tjek input og server.');
    }
}


// HTTP PUT funktionalitet

async function putWeatherData() {
    const id = document.getElementById('put_id').value;
    if (!id) {
        alert('Indtast venligst et ID for den post der skal opdateres.');
        return;
    }

    const data = {
        "Tidspunkt (dato og klokkeslæt)": {
            "Dato": document.getElementById('put_date').value,
            "Klokkeslæt": document.getElementById('put_time').value
        },
        "Sted": {
            "Navn": document.getElementById('put_place_name').value,
            "Lat": parseFloat(document.getElementById('put_lat').value),
            "Lon": parseFloat(document.getElementById('put_lon').value)
        },
        "Temperatur": parseFloat(document.getElementById('put_temperature').value),
        "Luftfugtighed": parseInt(document.getElementById('put_humidity').value)
    };

    try {
        const response = await axios.put(`<span class="math-inline">\{API\_BASE\_URL\}/weather/</span>{id}`, data);
        alert('Vejrdata opdateret for ID: ' + response.data.ID);
        getAllWeatherData();
    } catch (error) {
        alert('Kunne ikke opdatere vejrdata. Tjek ID, input og server.');
    }
}

// --- WebSocket Logik (til Delopgave 3 / Lektion 12) ---
// Dette er en grundlæggende skabelon. Mere detaljeret implementering
// kræver vejledning fra "SWNGK Week13 Lab Guide.pdf" og Lektion 13 slides.

// const ws = new WebSocket('ws://localhost:8080/ws'); // Antag at WebSocket-endpoint er /ws

// ws.onopen = () => {
//     console.log('Forbundet til WebSocket server.');
// };

// ws.onmessage = (event) => {
//     console.log('Modtaget besked fra WebSocket:', event.data);
//     // Opdater klientens UI baseret på den modtagne besked (f.eks. ved at kalde getAllWeatherData())
//     // Det er vigtigt at validere dataformatet her
//     // F.eks.: if (event.data === 'new_weather_data') { getAllWeatherData(); }
// };

// ws.onclose = () => {
//     console.log('Forbindelse til WebSocket lukket.');
// };

// ws.onerror = (error) => {
//     console.error('WebSocket Fejl:', error);
// };
