const API_BASE_URL = 'http://localhost:8080';
const WS_URL = 'ws://localhost:8080/weather/live';
let weatherTable;
let ws;

// Funktion til at initialisere Tabulator-tabellen
function initializeTable() {
    if (!weatherTable) {
        weatherTable = new Tabulator("#weatherDataTable", {
            layout: "fitDataFill",
            height: "auto",
            columns: [
                { title: "ID", field: "ID", width: 60 },
                { title: "Dato", field: "Tidspunkt (dato og klokkeslæt).Dato", width: 120 },
                { title: "Klokkeslæt", field: "Tidspunkt (dato og klokkeslæt).Klokkeslæt", width: 100 },
                { title: "Sted", field: "Sted.Navn", width: 150 },
                { title: "Lat", field: "Sted.Lat", width: 80 },
                { title: "Lon", field: "Sted.Lon", width: 80 },
                { title: "Temperatur", field: "Temperatur", width: 120 },
                { title: "Luftfugtighed", field: "Luftfugtighed", width: 120 },
            ],
            placeholder: "Ingen vejrdata at vise",
        });
    }
}

// Opsætter WebSocket-forbindelsen
function connectWebSocket() {
    console.log('Forsøger at oprette WebSocket-forbindelse til:', WS_URL);
    ws = new WebSocket(WS_URL);

    ws.onopen = () => {
        console.log('WebSocket-forbindelse oprettet.');
    };

    ws.onmessage = (event) => {
        console.log('Modtaget besked fra WebSocket:', event.data);
        try {
            const message = JSON.parse(event.data);
            if (message && (message.ID || message.type === 'weather_updated' || message.type === 'weather_created')) {
                console.log('Vejrdata ændret via WebSocket. Opdaterer tabel...');
                getAllWeatherData(); 
            }
        } catch (e) {
            console.warn('Kunne ikke parse WebSocket-besked som JSON, men genopfrisker alligevel:', event.data);
            getAllWeatherData();
        }
    };

    ws.onclose = (event) => {
        console.log('Forbindelse til WebSocket lukket. Kode:', event.code, 'Grund:', event.reason || 'Ingen grund');
        setTimeout(connectWebSocket, 3000); // Prøver hver X sekunder at genoprette forbindelsen
    };

    ws.onerror = (error) => {
        console.error('WebSocket Fejl:', error);
        alert('Der opstod en fejl med WebSocket-forbindelsen. Se konsollen.');
    };
}


document.addEventListener('DOMContentLoaded', () => {
    initializeTable();
    connectWebSocket();
    // Hent alle data ved start
    getAllWeatherData();
});


// HTTP GET funktionalitet
async function getAllWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/weather`);
        weatherTable.setData(response.data);
    } catch (error) {
        console.error('Fejl ved hentning af alle vejrdata:', error);
        alert('Kunne ikke hente alle vejrdata. Tjek serveren og konsollen for fejl.');
        weatherTable.setData([]);
    }
}

async function getWeatherDataById() {
    const id = document.getElementById('get_id').value;
    if (!id) {
        alert('Indtast venligst et ID for at hente data.');
        return;
    }
    try {
        const response = await axios.get(`${API_BASE_URL}/weather/${id}`);
        weatherTable.setData([response.data]);
    } catch (error) {
        console.error(`Fejl ved hentning af vejrdata for ID '${id}':`, error);
        if (error.response && error.response.status === 404) {
            alert(`Vejrdata med ID '${id}' blev ikke fundet.`);
        } else {
            alert(`Kunne ikke hente vejrdata for ID '${id}'. Tjek konsollen for fejl.`);
        }
        weatherTable.setData([]);
    }
}

async function getWeatherDataByDate() {
    const date = document.getElementById('get_date').value;
    if (!date) {
        alert('Indtast venligst en dato.');
        return;
    }
    const normalizedDate = date.replace(/-/g, '');
    try {
        const response = await axios.get(`${API_BASE_URL}/weather/date/${normalizedDate}`);
        weatherTable.setData(response.data);
    } catch (error) {
        console.error(`Fejl ved hentning af vejrdata for dato '${date}':`, error);
        alert(`Kunne ikke hente vejrdata for dato '${date}'. Tjek konsollen for fejl.`);
        weatherTable.setData([]);
    }
}

async function getLatestThreeWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/weather/latest_three`);
        console.log('Data modtaget for latest_three:', response.data);
        weatherTable.setData(response.data);
    } catch (error) {
        console.error('Fejl ved hentning af de 3 seneste vejrdata:', error);
        alert('Kunne ikke hente de 3 seneste vejrdata. Tjek konsollen for fejl.');
        weatherTable.setData([]);
    }
}

// HTTP POST funktionalitet
async function postWeatherData() {
    const date = document.getElementById('post_date').value;
    const time = document.getElementById('post_time').value;
    const placeName = document.getElementById('post_place_name').value;
    const latValue = document.getElementById('post_lat').value;
    const lonValue = document.getElementById('post_lon').value;
    const temperatureValue = document.getElementById('post_temperature').value;
    const humidityValue = document.getElementById('post_humidity').value;

    // Validering
    if (!date || !time || !placeName || !latValue || !lonValue || !temperatureValue || !humidityValue) {
        alert("Alle felter skal udfyldes.");
        return;
    }
    if (isNaN(parseFloat(latValue))) { alert("Latitude skal være et gyldigt tal."); return; }
    if (isNaN(parseFloat(lonValue))) { alert("Longitude skal være et gyldigt tal."); return; }
    if (isNaN(parseFloat(temperatureValue))) { alert("Temperatur skal være et gyldigt tal."); return; }
    if (isNaN(parseInt(humidityValue))) { alert("Luftfugtighed skal være et gyldigt heltal."); return; }


    const data = {
        "Tidspunkt (dato og klokkeslæt)": {
            "Dato": date,
            "Klokkeslæt": time
        },
        "Sted": {
            "Navn": placeName,
            "Lat": parseFloat(latValue),
            "Lon": parseFloat(lonValue)
        },
        "Temperatur": parseFloat(temperatureValue),
        "Luftfugtighed": parseInt(humidityValue)
    };

    try {
        const response = await axios.post(`${API_BASE_URL}/weather`, data);
        alert(`Vejrdata oprettet med ID: ${response.data.ID}.`);
    } catch (error) {
        console.error('Fejl ved oprettelse af vejrdata (POST):', error.response ? error.response.data : error.message);
        alert('Kunne ikke oprette vejrdata. Tjek input og serverens log for fejl.');
    }
}

// HTTP PUT funktionalitet
async function putWeatherData() {
    const id = document.getElementById('put_id').value;
    if (!id) {
        alert('Indtast venligst et ID for den post, der skal opdateres.');
        return;
    }

    // Hent værdier fra felter
    const date = document.getElementById('put_date').value;
    const time = document.getElementById('put_time').value;
    const placeName = document.getElementById('put_place_name').value;
    const latValue = document.getElementById('put_lat').value;
    const lonValue = document.getElementById('put_lon').value;
    const temperatureValue = document.getElementById('put_temperature').value;
    const humidityValue = document.getElementById('put_humidity').value;

    // Validering
    if (!date || !time || !placeName || !latValue || !lonValue || !temperatureValue || !humidityValue) {
        alert("Alle felter skal udfyldes for at opdatere.");
        return;
    }
    if (isNaN(parseFloat(latValue))) { alert("Latitude skal være et gyldigt tal."); return; }
    if (isNaN(parseFloat(lonValue))) { alert("Longitude skal være et gyldigt tal."); return; }
    if (isNaN(parseFloat(temperatureValue))) { alert("Temperatur skal være et gyldigt tal."); return; }
    if (isNaN(parseInt(humidityValue))) { alert("Luftfugtighed skal være et gyldigt heltal."); return; }


    const data = {
        "Tidspunkt (dato og klokkeslæt)": {
            "Dato": date,
            "Klokkeslæt": time
        },
        "Sted": {
            "Navn": placeName,
            "Lat": parseFloat(latValue),
            "Lon": parseFloat(lonValue)
        },
        "Temperatur": parseFloat(temperatureValue),
        "Luftfugtighed": parseInt(humidityValue)
    };

    try {
        const response = await axios.put(`${API_BASE_URL}/weather/${id}`, data);
        alert(`Vejrdata opdateret for ID: ${response.data.ID}.`);
    } catch (error) {
        console.error(`Fejl ved opdatering af vejrdata med ID ${id} (PUT):`, error.response ? error.response.data : error.message);
        if (error.response && error.response.status === 404) {
            alert(`Vejrdata med ID '${id}' blev ikke fundet til opdatering.`);
        } else {
            alert(`Kunne ikke opdatere vejrdata for ID '${id}'. Tjek input og serverens log for fejl.`);
        }
    }
}