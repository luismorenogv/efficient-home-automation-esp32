// Establishes and manages a WebSocket connection with the server
let socket;

document.addEventListener("DOMContentLoaded", () => {
    initializeWebSocket();
});

function initializeWebSocket() {
    const protocol = (window.location.protocol === 'https:') ? 'wss://' : 'ws://';
    socket = new WebSocket(protocol + window.location.host + '/ws');

    socket.onopen = () => {
        console.log("WebSocket connection established");
    };

    socket.onmessage = (event) => {
        let data;
        try {
            data = JSON.parse(event.data);
        } catch (error) {
            console.error('Error parsing JSON:', error);
            return;
        }

        // Handle message types
        if (data.type === 'history') {
            try {
                displayHistoryModal(data);
            } catch (error) {
                console.error('Error in displayHistoryModal:', error);
            }
        } else if (data.type === 'update') {
            try {
                updateSensorData(data);
            } catch (error) {
                console.error('Error in updateSensorData:', error);
            }
        } else {
            console.warn('Unknown message type received:', data);
        }
    };

    socket.onclose = () => {
        console.log("WebSocket connection closed, retrying in 5 seconds...");
        setTimeout(initializeWebSocket, 5000);
    };

    socket.onerror = (error) => {
        console.error("WebSocket error:", error);
        socket.close();
    };
}

// Requests historical data for a specific room
function showHistoryModal(roomId) {
    if (socket && socket.readyState === WebSocket.OPEN) {
        const message = { action: "getHistory", room_id: roomId };
        socket.send(JSON.stringify(message));
    } else {
        alert('WebSocket not connected');
    }
}

// Displays historical data in a modal with a chart
function displayHistoryModal(data) {
    // Remove existing modal if any
    const existingModal = document.querySelector('.modal');
    if (existingModal) document.body.removeChild(existingModal);

    // Create modal elements
    const modal = document.createElement('div');
    modal.className = 'modal';

    const modalContent = document.createElement('div');
    modalContent.className = 'modal-content';

    const closeButton = document.createElement('span');
    closeButton.className = 'close-button';
    closeButton.innerHTML = '&times;';
    closeButton.onclick = () => document.body.removeChild(modal);

    const canvas = document.createElement('canvas');
    canvas.id = 'historyChart';

    modalContent.appendChild(closeButton);
    modalContent.appendChild(canvas);
    modal.appendChild(modalContent);
    document.body.appendChild(modal);

    // Prepare data for chart
    const timestamps = data.timestamps.map(ts => new Date(ts * 1000));
    const temperatures = data.temperature;
    const humidities = data.humidity;

    if (timestamps.length !== temperatures.length || temperatures.length !== humidities.length) {
        console.error('Inconsistent data lengths');
        return;
    }

    // Determine chart ranges
    const minTemp = Math.min(...temperatures);
    const maxTemp = Math.max(...temperatures);
    const tempPadding = maxTemp * 0.4;
    const tempMin = minTemp - tempPadding;
    const tempMax = maxTemp + tempPadding;

    const minHumid = Math.min(...humidities);
    const maxHumid = Math.max(...humidities);
    const humidPadding = maxHumid * 0.4;
    const humidMin = minHumid - humidPadding;
    const humidMax = maxHumid + humidPadding;

    // Create chart
    const ctx = canvas.getContext('2d');
    new Chart(ctx, {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: [
                {
                    label: 'Temperature (°C)',
                    data: temperatures,
                    borderColor: 'rgba(255, 99, 132, 1)',
                    backgroundColor: 'rgba(255, 99, 132, 0.2)',
                    fill: false,
                    yAxisID: 'y',
                    tension: 0.1,
                    pointRadius: 0
                },
                {
                    label: 'Humidity (%)',
                    data: humidities,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    backgroundColor: 'rgba(54, 162, 235, 0.2)',
                    fill: false,
                    yAxisID: 'y1',
                    tension: 0.1,
                    pointRadius: 0
                }
            ]
        },
        options: {
            normalized: true,
            scales: {
                x: {
                    type: 'time',
                    time: { unit: 'minute', stepSize: 10, tooltipFormat: 'HH:mm:ss', displayFormats: { minute: 'HH:mm', hour: 'HH:mm' } },
                    title: { display: true, text: 'Time' },
                    ticks: { maxRotation: 0, autoSkip: true }
                },
                y: {
                    beginAtZero: false,
                    min: tempMin,
                    max: tempMax,
                    position: 'left',
                    title: { display: true, text: 'Temperature (°C)' }
                },
                y1: {
                    beginAtZero: false,
                    min: humidMin,
                    max: humidMax,
                    position: 'right',
                    grid: { drawOnChartArea: false },
                    title: { display: true, text: 'Humidity (%)' }
                }
            },
            plugins: {
                legend: { display: true, position: 'top' },
                tooltip: { mode: 'index', intersect: false }
            },
            interaction: { mode: 'index', intersect: false },
            maintainAspectRatio: false,
            responsive: true,
        }
    });
}

// Updates sensor data display for a room
function updateSensorData(data) {
    console.log(data);
    const container = document.getElementById('sensor-data');
    let roomDiv = document.getElementById(`room-${data.room_id}`);

    if (!roomDiv) {
        roomDiv = document.createElement('div');
        roomDiv.className = 'room-box';
        roomDiv.id = `room-${data.room_id}`;

        const title = document.createElement('h2');
        title.textContent = data.room_name;
        roomDiv.appendChild(title);

        const tempPara = document.createElement('p');
        tempPara.id = `temp-${data.room_id}`;
        roomDiv.appendChild(tempPara);

        const humidPara = document.createElement('p');
        humidPara.id = `humid-${data.room_id}`;
        roomDiv.appendChild(humidPara);

        const timePara = document.createElement('p');
        timePara.id = `time-${data.room_id}`;
        roomDiv.appendChild(timePara);

        const historyButton = document.createElement('button');
        historyButton.id = `history-${data.room_id}`;
        historyButton.textContent = 'Show History';
        historyButton.onclick = () => showHistoryModal(data.room_id);
        historyButton.style.marginRight = '10px';
        historyButton.style.display = 'none';
        roomDiv.appendChild(historyButton);

        // Add sleep period dropdown
        const sleepLabel = document.createElement('label');
        sleepLabel.id = `sleep-label-${data.room_id}`;
        sleepLabel.setAttribute('for', `sleep-${data.room_id}`);
        sleepLabel.textContent = `Sleep Period: `;
        sleepLabel.style.display = 'none';
        roomDiv.appendChild(sleepLabel);

        const sleepSelect = document.createElement('select');
        sleepSelect.id = `sleep-${data.room_id}`;
        sleepSelect.style.display = 'none';

        sleepSelect.id = `sleep-${data.room_id}`;
        const options = [
            { text: '5 min', value: '5min' },
            { text: '15 min', value: '15min' },
            { text: '30 min', value: '30min' },
            { text: '1 h', value: '1h' },
            { text: '3 h', value: '3h' },
            { text: '6 h', value: '6h' },
        ];

        options.forEach(opt => {
            const optionElement = document.createElement('option');
            optionElement.value = opt.value;
            optionElement.textContent = opt.text;
            sleepSelect.appendChild(optionElement);
        });

        sleepSelect.onchange = () => {
            const selected = sleepSelect.value;
            const message = {
                action: "setSleepPeriod",
                room_id: data.room_id,
                sleep_period: selected
            };
            socket.send(JSON.stringify(message));
            console.log(`Sent sleep period update: ${JSON.stringify(message)}`);
        };

        roomDiv.appendChild(sleepSelect);

        const warmPara = document.createElement('p');
        warmPara.id = `warm-${data.room_id}`;
        roomDiv.appendChild(warmPara);

        const coldPara = document.createElement('p');
        coldPara.id = `cold-${data.room_id}`;
        roomDiv.appendChild(coldPara);

        container.appendChild(roomDiv);
    }

    // Show the getHistory button when temperature and humidity data become available
    const historyButton = document.getElementById(`history-${data.room_id}`);
    if (historyButton) {
        if (data.registered) {
            historyButton.style.display = 'inline-block';

            const sleepLabel = document.getElementById(`sleep-label-${data.room_id}`);
            const sleepSelect = document.getElementById(`sleep-${data.room_id}`);
            if (sleepLabel) {
                sleepLabel.style.display = 'inline-block';
            }
            if (sleepSelect) {
                sleepSelect.style.display = 'inline-block';
            }
        } else {
            console.log(`Room ${data.room_id} is not registered or data value is undefined.`);
        }
    }


    const sleepSelect = document.getElementById(`sleep-${data.room_id}`);
    if (sleepSelect && typeof data.sleep_period_ms !== 'undefined'){
        sleepSelect.value = sleepMsToStr(data.sleep_period_ms);
    }

    // Update displayed sensor values
    if (typeof data.temperature !== 'undefined'){
        document.getElementById(`temp-${data.room_id}`).textContent = `Temperature: ${data.temperature.toFixed(2)} °C`; 
    }
    if (typeof data.humidity !== 'undefined'){
        document.getElementById(`humid-${data.room_id}`).textContent = `Humidity: ${data.humidity.toFixed(2)} %`;
    }

    if (typeof data.timestamp !== 'undefined' && data.timestamp > 0) {
        const date = new Date(data.timestamp * 1000);
        document.getElementById(`time-${data.room_id}`).textContent = `Last Updated: ${date.toLocaleString()}`;
    } else {
        document.getElementById(`time-${data.room_id}`).textContent = '';
    }

    // If room control data is available, create schedule form if not present
    if (typeof data.warm_time !== 'undefined' && typeof data.cold_time !== 'undefined') {
        let scheduleForm = document.getElementById(`schedule-form-${data.room_id}`);
        if (!scheduleForm) {
            scheduleForm = document.createElement('div');
            scheduleForm.id = `schedule-form-${data.room_id}`;

            const warmInput = document.createElement('input');
            warmInput.type = 'time';
            warmInput.id = `warm-input-${data.room_id}`;
            warmInput.value = data.warm_time;

            const coldInput = document.createElement('input');
            coldInput.type = 'time';
            coldInput.id = `cold-input-${data.room_id}`;
            coldInput.value = data.cold_time;

            const setScheduleButton = document.createElement('button');
            setScheduleButton.textContent = 'Set Schedule';
            setScheduleButton.onclick = () => {
                const warmVal = document.getElementById(`warm-input-${data.room_id}`).value; 
                const coldVal = document.getElementById(`cold-input-${data.room_id}`).value;

                const message = {
                    action: "setSchedule",
                    room_id: data.room_id,
                    warm_time: warmVal,
                    cold_time: coldVal
                };
                socket.send(JSON.stringify(message));
                console.log(`Sent setSchedule: ${JSON.stringify(message)}`);
            };

            scheduleForm.appendChild(document.createTextNode("Warm Mode Time: "));
            scheduleForm.appendChild(warmInput);
            scheduleForm.appendChild(document.createElement('br'));

            scheduleForm.appendChild(document.createTextNode("Cold Mode Time: "));
            scheduleForm.appendChild(coldInput);
            scheduleForm.appendChild(document.createElement('br'));

            scheduleForm.appendChild(setScheduleButton);
            roomDiv.appendChild(scheduleForm);
        }
    }
}

// Converts sleep period in ms to a string label
function sleepMsToStr(ms) {
    switch(ms) {
        case 300000: return '5min';
        case 900000: return '15min';
        case 1800000: return '30min';
        case 3600000: return '1h';
        case 10800000: return '3h';
        case 21600000: return '6h';
        default: return '5min';
    }
}
