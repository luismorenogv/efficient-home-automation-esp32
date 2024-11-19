document.addEventListener("DOMContentLoaded", function() {
    fetchDataAndRenderCharts();

    setInterval(fetchDataAndRenderCharts, 60000); // refresh every 60 seconds
});

function fetchDataAndRenderCharts() {
    fetch('/data')
    .then(response => response.json())
    .then(data => {
        renderCharts(data.rooms);
    })
    .catch(error => {
        console.error('Error fetching data:', error);
    });
}

function renderCharts(rooms) {
    const container = document.getElementById('charts-container');
    container.innerHTML = ''; // Clear previous charts

    rooms.forEach(room => {
        const chartBox = document.createElement('div');
        chartBox.className = 'chart-box';

        const title = document.createElement('h2');
        title.textContent = `Room ${room.room_id}`;
        chartBox.appendChild(title);

        const tempCanvas = document.createElement('canvas');
        tempCanvas.id = `temp-chart-${room.room_id}`;
        chartBox.appendChild(tempCanvas);

        const humidCanvas = document.createElement('canvas');
        humidCanvas.id = `humid-chart-${room.room_id}`;
        chartBox.appendChild(humidCanvas);

        container.appendChild(chartBox);

        // Render temperature chart
        new Chart(tempCanvas, {
            type: 'line',
            data: {
                labels: getLabels(room.temperature.length),
                datasets: [{
                    label: 'Temperature (°C)',
                    data: room.temperature,
                    borderColor: 'rgba(255, 99, 132, 1)',
                    borderWidth: 2,
                    fill: false,
                }]
            },
            options: {
                responsive: true,
                scales: {
                    x: {
                        display: false // Hide x-axis labels
                    },
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });

        // Render humidity chart
        new Chart(humidCanvas, {
            type: 'line',
            data: {
                labels: getLabels(room.humidity.length),
                datasets: [{
                    label: 'Humidity (%)',
                    data: room.humidity,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    borderWidth: 2,
                    fill: false,
                }]
            },
            options: {
                responsive: true,
                scales: {
                    x: {
                        display: false // Hide x-axis labels
                    },
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });
    });
}

function getLabels(length) {
    // Generate labels like "Data Point 1", "Data Point 2", etc.
    let labels = [];
    for (let i = 0; i < length; i++) {
        labels.push(`Point ${i+1}`);
    }
    return labels;
}
