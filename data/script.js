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
        } else {
            try {
                updateSensorData(data);
            } catch (error) {
                console.error('Error in updateSensorData:', error);
            }
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
        socket.send(`getHistory:${roomId}`);
    } else {
        alert('WebSocket not connected');
    }
}

function displayHistoryModal(data) {
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
    const timestamps = data.timestamps.map(ts => new Date(ts*1000)); // Timestamps in milliseconds
    const temperatures = data.temperature;
    const humidities = data.humidity;

    // Log data lengths
    console.log('Data lengths:', {
        timestamps: timestamps.length,
        temperatures: temperatures.length,
        humidities: humidities.length
    });

    // Log first few entries
    console.log('Sample data:', {
        timestamps: timestamps.slice(0, 5),
        temperatures: temperatures.slice(0, 5),
        humidities: humidities.slice(0, 5)
    });

    // Check for data consistency
    if (timestamps.length !== temperatures.length || temperatures.length !== humidities.length) {
        console.error('Data arrays have inconsistent lengths');
        return;
    }

    // Log first few entries
    console.log('Sample data:', {
        timestamps: timestamps.slice(0, 5).map(ts => ts.toLocaleString()),
        temperatures: temperatures.slice(0, 5),
        humidities: humidities.slice(0, 5)
    });


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
                    fill: false,
                    yAxisID: 'y',
                    tension: 0.1
                },
                {
                    label: 'Humidity (%)',
                    data: humidities,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    fill: false,
                    yAxisID: 'y1',
                    tension: 0.1
                }
            ]
        },
        options: {
            // parsing: false,
            normalized: true,
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'minute',
                        tooltipFormat: 'HH:mm:ss',
                        displayFormats: {
                            minute: 'HH:mm',
                            hour: 'HH:mm',
                        }
                    },
                    title: {
                        display: true,
                        text: 'Time'
                    }
                },
                y: {
                    beginAtZero: true,
                    suggestedMin: 15, // Adjust based on expected temperature range
                    suggestedMax: 35,
                    position: 'left',
                    title: {
                        display: true,
                        text: 'Temperature (°C)'
                    }
                },
                y1: {
                    beginAtZero: true,
                    suggestedMin: 20, // Adjust based on expected humidity range
                    suggestedMax: 100,
                    position: 'right',
                    grid: {
                        drawOnChartArea: false,
                    },
                    title: {
                        display: true,
                        text: 'Humidity (%)'
                    }
                }
            }
        }
    });
}

function updateSensorData(data) {
    const container = document.getElementById('sensor-data');

    // Check if the room element already exists
    let roomDiv = document.getElementById(`room-${data.room_id}`);
    if (!roomDiv) {
        // Create new room element
        roomDiv = document.createElement('div');
        roomDiv.className = 'room-box';
        roomDiv.id = `room-${data.room_id}`;

        const title = document.createElement('h2');
        title.textContent = `${data.room_name}`;  // Correctly set room_name
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

        // Add button to show historical data
        const historyButton = document.createElement('button');
        historyButton.textContent = 'Show History';
        historyButton.onclick = () => {
            showHistoryModal(data.room_id);
        };
        roomDiv.appendChild(historyButton);

        container.appendChild(roomDiv);
    }

    // Update the data
    document.getElementById(`temp-${data.room_id}`).textContent = `Temperature: ${data.temperature.toFixed(2)} °C`;
    document.getElementById(`humid-${data.room_id}`).textContent = `Humidity: ${data.humidity.toFixed(2)} %`;
    const date = new Date(data.timestamp * 1000);
    document.getElementById(`time-${data.room_id}`).textContent = `Last Updated: ${date.toLocaleString()}`;
}

