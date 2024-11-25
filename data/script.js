let socket;

document.addEventListener("DOMContentLoaded", function() {
    initializeWebSocket();
});

function initializeWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
    socket = new WebSocket(protocol + window.location.host + '/ws');

    socket.onopen = function() {
        console.log("WebSocket connection established");
    };

    socket.onmessage = function(event) {
        let data;
        try {
            data = JSON.parse(event.data);
            console.log("Received message:", data);
        } catch (error) {
            console.error('Error parsing JSON:', error);
            return;
        }

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


    socket.onclose = function() {
        console.log("WebSocket connection closed, retrying in 5 seconds...");
        setTimeout(initializeWebSocket, 5000);
    };

    socket.onerror = function(error) {
        console.error("WebSocket error:", error);
        socket.close();
    };
}

function showHistoryModal(roomId) {
    if (socket && socket.readyState === WebSocket.OPEN) {
        const message = {
            action: "getHistory",
            room_id: roomId
        };
        socket.send(JSON.stringify(message));
        console.log(`Sent getHistory request: ${JSON.stringify(message)}`);
    } else {
        alert('WebSocket not connected');
    }
}



function displayHistoryModal(data) {
    // Remove existing modal if any
    const existingModal = document.querySelector('.modal');
    if (existingModal) {
        document.body.removeChild(existingModal);
    }

    // Create modal elements
    const modal = document.createElement('div');
    modal.className = 'modal';

    const modalContent = document.createElement('div');
    modalContent.className = 'modal-content';

    const closeButton = document.createElement('span');
    closeButton.className = 'close-button';
    closeButton.innerHTML = '&times;';
    closeButton.onclick = () => {
        document.body.removeChild(modal);
    };

    const canvas = document.createElement('canvas');
    canvas.id = 'historyChart';

    modalContent.appendChild(closeButton);
    modalContent.appendChild(canvas);
    modal.appendChild(modalContent);
    document.body.appendChild(modal);

    // Prepare data for chart
    const timestamps = data.timestamps.map(ts => new Date(ts * 1000)); // Timestamps in milliseconds
    const temperatures = data.temperature;
    const humidities = data.humidity;

    // Check for data consistency
    if (timestamps.length !== temperatures.length || temperatures.length !== humidities.length) {
        console.error('Data arrays have inconsistent lengths');
        return;
    }

    // Calculate min and max values for temperature
    const minTemp = Math.min(...temperatures);
    const maxTemp = Math.max(...temperatures);
    const tempRange = (maxTemp || 1) - (minTemp || 1); // Avoid division by zero
    const tempPadding = 5.0;

    const tempMin = minTemp - tempPadding;
    const tempMax = maxTemp + tempPadding;

    // Calculate min and max values for humidity
    const minHumid = Math.min(...humidities);
    const maxHumid = Math.max(...humidities);
    const humidRange = maxHumid - minHumid || 1; // Avoid division by zero
    const humidPadding = 5.0;

    const humidMin = minHumid - humidPadding;
    const humidMax = maxHumid + humidPadding;

    // Create chart using Chart.js
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
                    time: {
                        unit: 'minute',
                        stepSize: 10,
                        tooltipFormat: 'HH:mm:ss',
                        displayFormats: {
                            minute: 'HH:mm',
                            hour: 'HH:mm',
                        }
                    },
                    title: {
                        display: true,
                        text: 'Time'
                    },
                    ticks: {
                        maxRotation: 0,
                        autoSkip: true,
                    }
                },
                y: {
                    beginAtZero: false,
                    min: tempMin,
                    max: tempMax,
                    position: 'left',
                    title: {
                        display: true,
                        text: 'Temperature (°C)'
                    }
                },
                y1: {
                    beginAtZero: false,
                    min: humidMin,
                    max: humidMax,
                    position: 'right',
                    grid: {
                        drawOnChartArea: false,
                    },
                    title: {
                        display: true,
                        text: 'Humidity (%)'
                    }
                }
            },
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                },
                tooltip: {
                    mode: 'index',
                    intersect: false,
                }
            },
            interaction: {
                mode: 'index',
                intersect: false,
            },
            maintainAspectRatio: false,
            responsive: true,
        }
    });
}

function updateSensorData(data) {
    // Defensive checks
    if (typeof data.room_id === 'undefined') {
        console.error('updateSensorData called with undefined room_id:', data);
        return;
    }
    if (typeof data.temperature === 'undefined') {
        console.error(`Temperature data missing for room ${data.room_id}:`, data);
        return;
    }
    if (typeof data.humidity === 'undefined') {
        console.error(`Humidity data missing for room ${data.room_id}:`, data);
        return;
    }
    if (typeof data.sleep_period_ms === 'undefined') {
        console.error(`Sleep period data missing for room ${data.room_id}:`, data);
        return;
    }
    if (typeof data.room_name === 'undefined') {
        console.error(`Room name missing for room ${data.room_id}:`, data);
        return;
    }
    if (typeof data.timestamp === 'undefined') {
        console.error(`Timestamp missing for room ${data.room_id}:`, data);
        return;
    }

    const container = document.getElementById('sensor-data');

    // Check if the room element already exists
    let roomDiv = document.getElementById(`room-${data.room_id}`);
    if (!roomDiv) {
        // Create new room element
        roomDiv = document.createElement('div');
        roomDiv.className = 'room-box';
        roomDiv.id = `room-${data.room_id}`;

        const title = document.createElement('h2');
        title.textContent = `${data.room_name}`;
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

        // Add sleep period dropdown
        const sleepLabel = document.createElement('label');
        sleepLabel.setAttribute('for', `sleep-${data.room_id}`);
        sleepLabel.textContent = `Sleep Period: `;
        roomDiv.appendChild(sleepLabel);

        const sleepSelect = document.createElement('select');
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

        // Add "Show History" button
        const historyButton = document.createElement('button');
        historyButton.textContent = 'Show History';
        historyButton.onclick = () => {
            showHistoryModal(data.room_id);
        };
        historyButton.style.marginLeft = '10px'; // Add some spacing
        roomDiv.appendChild(historyButton);

        container.appendChild(roomDiv);
    }

    // Update the data
    try {
        document.getElementById(`temp-${data.room_id}`).textContent = `Temperature: ${data.temperature.toFixed(2)} °C`;
    } catch (e) {
        console.error(`Error setting temperature for room ${data.room_id}:`, e);
    }

    try {
        document.getElementById(`humid-${data.room_id}`).textContent = `Humidity: ${data.humidity.toFixed(2)} %`;
    } catch (e) {
        console.error(`Error setting humidity for room ${data.room_id}:`, e);
    }

    try {
        const date = new Date(data.timestamp * 1000);
        document.getElementById(`time-${data.room_id}`).textContent = `Last Updated: ${date.toLocaleString()}`;
    } catch (e) {
        console.error(`Error setting timestamp for room ${data.room_id}:`, e);
    }

    // Update sleep period dropdown
    if (document.getElementById(`sleep-${data.room_id}`)) {
        document.getElementById(`sleep-${data.room_id}`).value = sleepMsToStr(data.sleep_period_ms);
    }
}


function sleepMsToStr(ms) {
    switch(ms) {
        case 300000:
            return '5min';
        case 900000:
            return '15min';
        case 1800000:
            return '30min';
        case 3600000:
            return '1h';
        case 10800000:
            return '3h';
        case 21600000:
            return '6h';
        default:
            return '5min';
    }
}
