// client.js

const API_BASE_URL = 'http://localhost:8080'; // Sørg for at dette matcher jeres server-port

// Funktion til at vise data i en tabel (ved brug af Tabulator.js)
let weatherTable;
function initializeTable() {
    if (!weatherTable) {
        weatherTable = new Tabulator("#weatherDataTable", {
            layout: "fitDataFill",
            height: "auto", // Eller en fast højde
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

// Kald funktionen når DOM er indlæst
document.addEventListener('DOMContentLoaded', initializeTable);


// --- HTTP GET Funktioner ---

async function getAllWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/weather`);
        console.log('Alle vejrdata:', response.data);
        weatherTable.setData(response.data); // Opdater Tabulator tabellen
    } catch (error) {
        console.error('Fejl ved hentning af alle vejrdata:', error);
        alert('Kunne ikke hente alle vejrdata. Tjek serveren.');
        weatherTable.setData([]); // Ryd tabellen ved fejl
    }
}

async function getWeatherDataById() {
    const id = document.getElementById('get_id').value;
    if (!id) {
        alert('Indtast venligst et ID.');
        return;
    }
    try {
        const response = await axios.get(`<span class="math-inline">\{API\_BASE\_URL\}/weather/</span>{id}`);
        console.log(`Vejrdata for ID ${id}:`, response.data);
        weatherTable.setData([response.data]); // Vis kun den ene post
    } catch (error) {
        console.error(`Fejl ved hentning af vejrdata for ID ${id}:`, error);
        alert('Kunne ikke finde vejrdata for det angivne ID.');
        weatherTable.setData([]);
    }
}

async function getWeatherDataByDate() {
    const date = document.getElementById('get_date').value;
    if (!date) {
        alert('Indtast venligst en dato (YYYYMMDD).');
        return;
    }
    try {
        const response = await axios.get(`<span class="math-inline">\{API\_BASE\_URL\}/weather/date/</span>{date}`);
        console.log(`Vejrdata for dato ${date}:`, response.data);
        weatherTable.setData(response.data); // Vis alle poster for datoen
    } catch (error) {
        console.error(`Fejl ved hentning af vejrdata for dato ${date}:`, error);
        alert('Kunne ikke finde vejrdata for den angivne dato.');
        weatherTable.setData([]);
    }
}

async function getLatestThreeWeatherData() {
    try {
        const response = await axios.get(`${API_BASE_URL}/latest_three`);
        console.log('De 3 seneste vejrdata:', response.data);
        weatherTable.setData(response.data); // Vis de 3 seneste poster
    } catch (error) {
        console.error('Fejl ved hentning af de 3 seneste vejrdata:', error);
        alert('Kunne ikke hente de 3 seneste vejrdata.');
        weatherTable.setData([]);
    }
}


// --- HTTP POST Funktion ---

async function postWeatherData() {
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
        // ID er ikke inkluderet her, da serveren genererer det
    };

    try {
        const response = await axios.post(`${API_BASE_URL}/weather`, data);
        console.log('Vejrdata oprettet:', response.data);
        alert('Vejrdata oprettet med ID: ' + response.data.ID);
        getAllWeatherData(); // Opdater visningen
    } catch (error) {
        console.error('Fejl ved oprettelse af vejrdata:', error);
        alert('Kunne ikke oprette vejrdata. Tjek input og server.');
    }
}


// --- HTTP PUT Funktion ---

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
        // ID er ikke inkluderet her, da det kommer fra URL'en
    };

    try {
        const response = await axios.put(`<span class="math-inline">\{API\_BASE\_URL\}/weather/</span>{id}`, data);
        console.log('Vejrdata opdateret:', response.data);
        alert('Vejrdata opdateret for ID: ' + response.data.ID);
        getAllWeatherData(); // Opdater visningen
    } catch (error) {
        console.error('Fejl ved opdatering af vejrdata:', error);
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
