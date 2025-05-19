// client.js

const API_BASE_URL = 'http://localhost:8080'; // Sørg for at dette matcher jeres server-port

// Variabel til Tabulator-tabelinstansen
let weatherTable;

// Funktion til at initialisere Tabulator-tabellen
function initializeTable() {
    if (!weatherTable) {
        weatherTable = new Tabulator("#weatherDataTable", {
            layout: "fitDataFill", // Fyld tilgængelig bredde ud
            height: "auto", // Dynamisk højde baseret på indhold
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
            placeholder: "Ingen vejrdata at vise", // Besked når tabellen er tom
        });
    }
}

// Sørg for at Tabulator-tabellen initialiseres når DOM er fuldt indlæst
document.addEventListener('DOMContentLoaded', initializeTable);


// --- HTTP GET Funktioner ---

/**
 * Henter og viser alle vejrdata fra serveren.
 */
async function getAllWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/weather`);
        console.log('Alle vejrdata:', response.data);
        weatherTable.setData(response.data); // Opdater Tabulator tabellen med alle data
    } catch (error) {
        console.error('Fejl ved hentning af alle vejrdata:', error);
        alert('Kunne ikke hente alle vejrdata. Tjek serveren og konsollen for fejl.');
        weatherTable.setData([]); // Ryd tabellen ved fejl
    }
}

/**
 * Henter og viser vejrdata for et specifikt ID.
 */
async function getWeatherDataById() {
    const id = document.getElementById('get_id').value;
    if (!id) {
        alert('Indtast venligst et ID for at hente data.');
        return;
    }
    try {
        const response = await axios.get(`${API_BASE_URL}/weather/${id}`);
        console.log(`Vejrdata for ID ${id}:`, response.data);
        weatherTable.setData([response.data]); // Vis kun den ene post i tabellen
    } catch (error) {
        console.error(`Fejl ved hentning af vejrdata for ID ${id}:`, error);
        if (error.response && error.response.status === 404) {
            alert(`Vejrdata med ID '${id}' blev ikke fundet.`);
        } else {
            alert(`Kunne ikke hente vejrdata for ID '${id}'. Tjek konsollen for fejl.`);
        }
        weatherTable.setData([]);
    }
}

/**
 * Henter og viser vejrdata for en specifik dato.
 */
async function getWeatherDataByDate() {
    const date = document.getElementById('get_date').value; // Forventer YYYYMMDD
    if (!date) {
        alert('Indtast venligst en dato (YYYYMMDD).');
        return;
    }
    try {
        const response = await axios.get(`${API_BASE_URL}/weather/date/${date}`);
        console.log(`Vejrdata for dato ${date}:`, response.data);
        weatherTable.setData(response.data); // Vis alle poster for datoen
    } catch (error) {
        console.error(`Fejl ved hentning af vejrdata for dato ${date}:`, error);
        alert(`Kunne ikke hente vejrdata for dato '${date}'. Tjek konsollen for fejl.`);
        weatherTable.setData([]);
    }
}

/**
 * Henter og viser de tre seneste vejrdata-poster.
 */
async function getLatestThreeWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/latest_three`);
        console.log('De 3 seneste vejrdata:', response.data);
        weatherTable.setData(response.data); // Vis de 3 seneste poster
    } catch (error) {
        console.error('Fejl ved hentning af de 3 seneste vejrdata:', error);
        alert('Kunne ikke hente de 3 seneste vejrdata. Tjek konsollen for fejl.');
        weatherTable.setData([]);
    }
}


// --- HTTP POST Funktion ---

/**
 * Opretter en ny vejrdata-post på serveren via POST.
 */
async function postWeatherData() {
    // Saml data fra inputfelterne. Navne matcher C++ json_dto felter.
    const data = {
        "Tidspunkt (dato og klokkeslæt)": {
            "Dato": document.getElementById('post_date').value,
            "Klokkeslæt": document.getElementById('post_time').value
        },
        "Sted": {
            "Navn": document.getElementById('post_place_name').value,
            "Lat": parseFloat(document.getElementById('post_lat').value),
            "Lon": parseFloat(document.getElementById('post_lon').value)
        },
        "Temperatur": parseFloat(document.getElementById('post_temperature').value),
        "Luftfugtighed": parseInt(document.getElementById('post_humidity').value)
        // ID er ikke inkluderet her, da serveren genererer det automatisk
    };

    try {
        const response = await axios.post(`${API_BASE_URL}/weather`, data);
        console.log('Vejrdata oprettet:', response.data);
        alert(`Vejrdata oprettet med ID: ${response.data.ID}.`);
        getAllWeatherData(); // Opdater visningen for at inkludere den nye post
    } catch (error) {
        console.error('Fejl ved oprettelse af vejrdata:', error);
        alert('Kunne ikke oprette vejrdata. Tjek input og serverens log for fejl.');
    }
}


// --- HTTP PUT Funktion ---

/**
 * Opdaterer en eksisterende vejrdata-post på serveren via PUT.
 */
async function putWeatherData() {
    const id = document.getElementById('put_id').value;
    if (!id) {
        alert('Indtast venligst et ID for den post, der skal opdateres.');
        return;
    }

    // Saml opdaterede data fra inputfelterne. Navne matcher C++ json_dto felter.
    const data = {
        // ID sendes ikke i body, men som en del af URL'en
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
        const response = await axios.put(`${API_BASE_URL}/weather/${id}`, data);
        console.log('Vejrdata opdateret:', response.data);
        alert(`Vejrdata opdateret for ID: ${response.data.ID}.`);
        getAllWeatherData(); // Opdater visningen for at vise de opdaterede data
    } catch (error) {
        console.error('Fejl ved opdatering af vejrdata:', error);
        if (error.response && error.response.status === 404) {
            alert(`Vejrdata med ID '${id}' blev ikke fundet til opdatering.`);
        } else {
            alert(`Kunne ikke opdatere vejrdata for ID '${id}'. Tjek input og serverens log for fejl.`);
        }
    }
}

// --- WebSocket Logik (Til fremtidig implementering i Delopgave 3 / Lektion 12) ---
// Denne del er kommenteret ud, da den kræver WebSocket-serverfunktionalitet
// og er en del af en senere del af opgaven.
// Når I implementerer WebSocket på serveren, skal I fjerne kommentarerne herunder
// og tilpasse WebSocket URL'en, hvis den er anderledes end '/ws'.

/*
let ws;

function connectWebSocket() {
    // Erstat med den korrekte WebSocket URL, hvis den er anderledes
    ws = new WebSocket('ws://localhost:8080/ws'); 

    ws.onopen = () => {
        console.log('Forbundet til WebSocket server.');
        // Måske send en besked til serveren for at abonnere på opdateringer
        // ws.send('subscribe');
    };

    ws.onmessage = (event) => {
        console.log('Modtaget besked fra WebSocket:', event.data);
        // Antag at serveren sender en besked, når data ændres.
        // I kan f.eks. parse JSON her, hvis serveren sender strukturerede beskeder.
        // Eller blot kalde getAllWeatherData() for at genindlæse alle data.
        try {
            const message = JSON.parse(event.data);
            if (message.type === 'weather_updated' || message.type === 'weather_created') {
                console.log('Vejrdata ændret via WebSocket. Opdaterer tabel...');
                getAllWeatherData(); // Opdater visningen når serveren notificerer
            }
        } catch (e) {
            console.warn('Modtog ikke JSON fra WebSocket:', event.data);
            // Hvis det er en simpel string besked (f.eks. "refresh"), så håndter det her
            if (event.data === 'refresh_weather_data') {
                getAllWeatherData();
            }
        }
    };

    ws.onclose = () => {
        console.log('Forbindelse til WebSocket lukket.');
        // Genopret forbindelse efter et stykke tid ved utilsigtet lukning
        setTimeout(connectWebSocket, 5000); 
    };

    ws.onerror = (error) => {
        console.error('WebSocket Fejl:', error);
        alert('Der opstod en fejl med WebSocket-forbindelsen. Se konsollen.');
    };
}

// Kald connectWebSocket() når klienten er klar, f.eks. efter DOMContentLoaded
// document.addEventListener('DOMContentLoaded', connectWebSocket);
*/
