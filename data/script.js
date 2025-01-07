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
    const tempRange = maxTemp - minTemp || 1; // Avoid division by zero
    const tempPadding = tempRange * 0.5;
    const tempMin = minTemp - tempPadding;
    const tempMax = maxTemp + tempPadding;

    const minHumid = Math.min(...humidities);
    const maxHumid = Math.max(...humidities);
    const humidRange = maxHumid - minHumid || 1; // Avoid division by zero
    const humidPadding = humidRange * 0.5;
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

// Updates sensor and control data display for a room
function updateSensorData(data) {
    console.log(data);
    const container = document.getElementById('home-data');
    let roomDiv = document.getElementById(`room-${data.room_id}`);

    // Remove roomDiv if both sensor and control are unregistered
    if (!data.control_registered && !data.sensor_registered) {
        if (roomDiv) {
            roomDiv.remove();
            console.log(`Room ${data.room_id} removed from UI (both nodes unregistered).`);
        }
        return;
    }

    // Create roomDiv if it doesn't exist
    if (!roomDiv) {
        roomDiv = document.createElement('div');
        roomDiv.className = 'room-box';
        roomDiv.id = `room-${data.room_id}`;

        // Room title
        const title = document.createElement('h2');
        title.textContent = data.room_name;
        roomDiv.appendChild(title);

        // Sensor container
        const sensorContainer = document.createElement('div');
        sensorContainer.className = 'sensor-data';
        sensorContainer.style.display = 'none'; // Hidden by default
        roomDiv.appendChild(sensorContainer);

        // Control container
        const controlContainer = document.createElement('div');
        controlContainer.className = 'control-data';
        controlContainer.style.display = 'none'; // Hidden by default
        roomDiv.appendChild(controlContainer);

        container.appendChild(roomDiv);
    }

    // Get sensor and control containers
    let sensorContainer = roomDiv.querySelector('.sensor-data');
    let controlContainer = roomDiv.querySelector('.control-data');

    // Handle SensorNode
    if (data.sensor_registered) {
        sensorContainer.style.display = 'block';

        // Create sensor elements if not present
        if (!sensorContainer.hasChildNodes()) {
            // Temperature
            const tempPara = document.createElement('p');
            tempPara.id = `temp-${data.room_id}`;
            sensorContainer.appendChild(tempPara);

            // Humidity
            const humidPara = document.createElement('p');
            humidPara.id = `humid-${data.room_id}`;
            sensorContainer.appendChild(humidPara);

            // Last Updated
            const timePara = document.createElement('p');
            timePara.id = `time-${data.room_id}`;
            sensorContainer.appendChild(timePara);

            // Show History Button
            const historyButton = document.createElement('button');
            historyButton.id = `history-${data.room_id}`;
            historyButton.textContent = 'Show History';
            historyButton.onclick = () => showHistoryModal(data.room_id);
            historyButton.style.marginRight = '10px';
            historyButton.style.display = 'inline-block';
            sensorContainer.appendChild(historyButton);

            // Sleep Period Dropdown
            const sleepLabel = document.createElement('label');
            sleepLabel.id = `sleep-label-${data.room_id}`;
            sleepLabel.setAttribute('for', `sleep-${data.room_id}`);
            sleepLabel.textContent = `Sleep Period: `;
            sleepLabel.style.display = 'inline-block';
            sensorContainer.appendChild(sleepLabel);

            const sleepSelect = document.createElement('select');
            sleepSelect.id = `sleep-${data.room_id}`;
            sleepSelect.style.display = 'inline-block';

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
            };

            sensorContainer.appendChild(sleepSelect);
        }

        // Update sleep period selection
        const sleepSelect = document.getElementById(`sleep-${data.room_id}`);
        if (sleepSelect && typeof data.sleep_period_ms !== 'undefined') {
            sleepSelect.value = sleepMsToStr(data.sleep_period_ms);
        }

        // Update sensor values
        if (typeof data.temperature === 'number') {
            const tempPara = document.getElementById(`temp-${data.room_id}`);
            if (tempPara) {
                tempPara.textContent = `Temperature: ${data.temperature.toFixed(2)} °C`;
            }
        }
        if (typeof data.humidity === 'number') {
            const humidPara = document.getElementById(`humid-${data.room_id}`);
            if (humidPara) {
                humidPara.textContent = `Humidity: ${data.humidity.toFixed(2)} %`;
            }
        }
        if (typeof data.timestamp === 'number' && data.timestamp > 0) {
            const timePara = document.getElementById(`time-${data.room_id}`);
            if (timePara) {
                const date = new Date(data.timestamp * 1000);
                timePara.textContent = `Last Updated: ${date.toLocaleString()}`;
            }
        } else {
            const timePara = document.getElementById(`time-${data.room_id}`);
            if (timePara) {
                timePara.textContent = '';
            }
        }
    } else {
        // Hide and clear sensor data if SensorNode is unregistered
        if (sensorContainer) {
            sensorContainer.style.display = 'none';
            sensorContainer.innerHTML = '';
        }
    }

    // Handle RoomNode (Control)
    if (data.control_registered) {
        controlContainer.style.display = 'block';

        // Create control elements if not present
        if (!controlContainer.hasChildNodes()) {
            // Schedule Form
            const scheduleForm = document.createElement('div');
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
                const warmVal = warmInput.value;
                const coldVal = coldInput.value;
                const message = {
                    action: "setSchedule",
                    room_id: data.room_id,
                    warm_time: warmVal,
                    cold_time: coldVal
                };
                socket.send(JSON.stringify(message));
            };

            scheduleForm.appendChild(document.createTextNode("Warm Mode Time: "));
            scheduleForm.appendChild(warmInput);
            scheduleForm.appendChild(document.createElement('br'));

            scheduleForm.appendChild(document.createTextNode("Cold Mode Time: "));
            scheduleForm.appendChild(coldInput);
            scheduleForm.appendChild(document.createElement('br'));

            scheduleForm.appendChild(setScheduleButton);
            controlContainer.appendChild(scheduleForm);

            // Lights Toggle
            const toggleContainer = document.createElement('div');
            toggleContainer.id = `lights-toggle-${data.room_id}`;
            toggleContainer.style.marginTop = '10px';

            const toggleLabel = document.createElement('label');
            toggleLabel.className = 'toggle-button';

            const toggleInput = document.createElement('input');
            toggleInput.type = 'checkbox';
            toggleInput.checked = data.lights_on;

            toggleInput.onchange = () => {
                const message = {
                    action: "toggleLights",
                    room_id: data.room_id,
                    turn_on: toggleInput.checked
                };
                socket.send(JSON.stringify(message));
            };

            const toggleSlider = document.createElement('span');
            toggleSlider.className = 'toggle-slider';

            toggleLabel.appendChild(toggleInput);
            toggleLabel.appendChild(toggleSlider);

            const toggleText = document.createElement('span');
            toggleText.style.marginLeft = '10px';
            toggleText.textContent = 'Lights';

            toggleContainer.appendChild(toggleLabel);
            toggleContainer.appendChild(toggleText);
            controlContainer.appendChild(toggleContainer);
        } else {
            // Update warm and cold times
            const warmInput = document.getElementById(`warm-input-${data.room_id}`);
            const coldInput = document.getElementById(`cold-input-${data.room_id}`);
            if (warmInput) warmInput.value = data.warm_time;
            if (coldInput) coldInput.value = data.cold_time;

            // Update lights toggle
            const toggleContainer = document.getElementById(`lights-toggle-${data.room_id}`);
            if (toggleContainer) {
                const toggleInput = toggleContainer.querySelector('input');
                if (toggleInput) {
                    toggleInput.checked = data.lights_on;
                }
            }
        }
    } else {
        // Hide and clear control data if RoomNode is unregistered
        if (controlContainer) {
            controlContainer.style.display = 'none';
            controlContainer.innerHTML = '';
        }
    }
}


// Converts sleep period from ms to string
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
