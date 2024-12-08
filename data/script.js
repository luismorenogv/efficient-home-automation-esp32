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
    const tempPadding = maxTemp * 0.4;

    const tempMin = minTemp - tempPadding;
    const tempMax = maxTemp + tempPadding;

    // Calculate min and max values for humidity
    const minHumid = Math.min(...humidities);
    const maxHumid = Math.max(...humidities);
    const humidRange = maxHumid - minHumid || 1; // Avoid division by zero
    const humidPadding = maxHumid * 0.4;

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
    const container = document.getElementById('sensor-data');

    let roomDiv = document.getElementById(`room-${data.room_id}`);
    if (!roomDiv) {
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

        const historyButton = document.createElement('button');
        historyButton.textContent = 'Show History';
        historyButton.onclick = () => {
            showHistoryModal(data.room_id);
        };
        historyButton.style.marginRight = '10px';
        roomDiv.appendChild(historyButton);

        const warmPara = document.createElement('p');
        warmPara.id = `warm-${data.room_id}`;
        roomDiv.appendChild(warmPara);

        const coldPara = document.createElement('p');
        coldPara.id = `cold-${data.room_id}`;
        roomDiv.appendChild(coldPara);

        container.appendChild(roomDiv);
    }

    if (typeof data.temperature !== 'undefined') {
        document.getElementById(`temp-${data.room_id}`).textContent = `Temperature: ${data.temperature.toFixed(2)} °C`;
    } else {
        document.getElementById(`temp-${data.room_id}`).textContent = `No SensorNode registered`;
    }

    if (typeof data.humidity !== 'undefined') {
        document.getElementById(`humid-${data.room_id}`).textContent = `Humidity: ${data.humidity.toFixed(2)} %`;
    } else {
        document.getElementById(`humid-${data.room_id}`).textContent = '';
    }

    if (typeof data.timestamp !== 'undefined' && data.timestamp > 0) {
        const date = new Date(data.timestamp * 1000);
        document.getElementById(`time-${data.room_id}`).textContent = `Last Updated: ${date.toLocaleString()}`;
    } else {
        document.getElementById(`time-${data.room_id}`).textContent = ``;
    }

    // Update warm/cold times
    if (typeof data.warm_time !== 'undefined') {
        document.getElementById(`warm-${data.room_id}`).textContent = `Warm Mode: ${data.warm_time}`;
    } else {
        document.getElementById(`warm-${data.room_id}`).textContent = ``;
    }

    if (typeof data.cold_time !== 'undefined') {
        document.getElementById(`cold-${data.room_id}`).textContent = `Cold Mode: ${data.cold_time}`;
    } else {
        document.getElementById(`cold-${data.room_id}`).textContent = ``;
    }

    // If RoomNode registered (control data present), allow schedule changes
    // data includes room.control info only if registered:
    if (typeof data.warm_time !== 'undefined' && typeof data.cold_time !== 'undefined') {
        // Add schedule controls if not already present
        let scheduleForm = document.getElementById(`schedule-form-${data.room_id}`);
        if (!scheduleForm) {
            scheduleForm = document.createElement('div');
            scheduleForm.id = `schedule-form-${data.room_id}`;

            // Warm Input
            const warmInput = document.createElement('input');
            warmInput.type = 'time';
            warmInput.id = `warm-input-${data.room_id}`;
            warmInput.value = data.warm_time; // "HH:MM"

            // Cold Input
            const coldInput = document.createElement('input');
            coldInput.type = 'time';
            coldInput.id = `cold-input-${data.room_id}`;
            coldInput.value = data.cold_time; // "HH:MM"

            const setScheduleButton = document.createElement('button');
            setScheduleButton.textContent = 'Set Schedule';
            setScheduleButton.onclick = () => {
                const warmVal = document.getElementById(`warm-input-${data.room_id}`).value; 
                const coldVal = document.getElementById(`cold-input-${data.room_id}`).value;

                // Send setSchedule action
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
